// Copyright Epic Games, Inc. All Rights Reserved.

#include "IndexModule.h"

#include "Editor.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "EditorActorFolders.h"
#include "Selection.h"
#include "Framework/Application/SlateApplication.h"
#include "IndexColumn.h"
#include "IndexInteraction.h"
#include "IndexSequenceState.h"
#include "SceneOutlinerModule.h"
#include "SceneOutlinerPublicTypes.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"

#define LOCTEXT_NAMESPACE "FIndexModule"

namespace
{
    TArray<FIndexSequenceItem> GIndexDuplicateSourceItems;
    FTSTicker::FDelegateHandle GIndexDuplicateFinalizeTicker;
    int32 GIndexDuplicateFinalizeAttempts = 0;
}

void FIndexModule::StartupModule()
{
    FSceneOutlinerModule& SceneOutlinerModule =
        FModuleManager::LoadModuleChecked<FSceneOutlinerModule>("SceneOutliner");

    ActorBrowserColumnsHandle = SceneOutlinerModule.OnCreateActorBrowserColumns().AddLambda(
        [](FSceneOutlinerInitializationOptions& Options, UWorld*)
        {
            Options.ColumnMap.Add(
                FIndexColumn::GetID(),
                FSceneOutlinerColumnInfo(
                    ESceneOutlinerColumnVisibility::Visible,
                    250,
                    FCreateSceneOutlinerColumn::CreateLambda(
                        [](ISceneOutliner& Outliner)
                        {
                            return MakeShared<FIndexColumn>(Outliner);
                        }),
                    false,
                    TOptional<float>(),
                    LOCTEXT("IndexColumnLabel", "Index")));
        });

    if (FSlateApplication::IsInitialized())
    {
        InputProcessor = IndexInteraction::CreateInputProcessor();
        FSlateApplication::Get().RegisterInputPreProcessor(InputProcessor);
    }

    PostUndoRedoHandle = FEditorDelegates::PostUndoRedo.AddLambda([]
    {
        FIndexSequenceState::PrepareForPostUndoRedo();
        IndexInteraction::RefreshAllOutliners();
    });

    MapChangeHandle = FEditorDelegates::MapChange.AddLambda([](uint32)
    {
        FIndexSequenceState::InvalidateCache();
        IndexInteraction::ResetTransientState();
    });

    ObjectsReplacedHandle = FCoreUObjectDelegates::OnObjectsReplaced.AddLambda(
        [](const TMap<UObject*, UObject*>&)
        {
            // Actor replacement during Blueprint/editor reconstruction normally retains
            // the actor GUID used by Index. Drop the cache so the next Outliner sort
            // resolves the current objects against the same saved hierarchy IDs.
            FIndexSequenceState::InvalidateCache();
            IndexInteraction::RefreshAllOutliners();
        });

    NewActorsPlacedHandle = FEditorDelegates::OnNewActorsPlaced.AddLambda(
        [this](UObject*, const TArray<AActor*>& PlacedActors)
        {
            if (!bDuplicateInProgress)
            {
                IndexInteraction::IntegrateNewActorsAtTop(PlacedActors);
            }
        });

    DuplicateActorsBeginHandle = FEditorDelegates::OnDuplicateActorsBegin.AddLambda([this]()
    {
        bDuplicateInProgress = true;
        GIndexDuplicateFinalizeAttempts = 0;
        GIndexDuplicateSourceItems.Reset();
        IndexInteraction::GetSelectedSequenceRoots(GIndexDuplicateSourceItems);
    });

    DuplicateActorsEndHandle = FEditorDelegates::OnDuplicateActorsEnd.AddLambda([this]()
    {
        // Folder duplication finishes in the stock Outliner's own duplicate-end
        // callback. Defer one tick so its new folder rows exist and selection has
        // been rebuilt before Index resolves the duplicate block. Keep the
        // duplicate guard active until then so creation callbacks do not treat the
        // duplicates as generic new content at the top of the tier.
        if (GIndexDuplicateFinalizeTicker.IsValid())
        {
            FTSTicker::GetCoreTicker().RemoveTicker(GIndexDuplicateFinalizeTicker);
            GIndexDuplicateFinalizeTicker.Reset();
        }

        GIndexDuplicateFinalizeTicker = FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateLambda([this](float)
            {
                ++GIndexDuplicateFinalizeAttempts;

                const TArray<FIndexSequenceItem> Sources = GIndexDuplicateSourceItems;
                TArray<FIndexSequenceItem> SelectedAfterDuplicate;
                IndexInteraction::GetSelectedSequenceRoots(SelectedAfterDuplicate);

                if (Sources.IsEmpty())
                {
                    bDuplicateInProgress = false;
                    GIndexDuplicateSourceItems.Reset();
                    GIndexDuplicateFinalizeAttempts = 0;
                    GIndexDuplicateFinalizeTicker.Reset();
                    return false;
                }

                // The stock folder duplicate path can leave the original folders
                // selected and add duplicate folders after its refresh has populated
                // the new rows. Remove the original roots explicitly. If the rows are
                // not present yet, keep this ticker alive for a few frames rather than
                // misclassifying the duplicate as generic new content.
                TArray<FIndexSequenceItem> Duplicates;
                for (const FIndexSequenceItem& Candidate : SelectedAfterDuplicate)
                {
                    bool bIsSource = false;
                    for (const FIndexSequenceItem& Source : Sources)
                    {
                        if (Candidate == Source)
                        {
                            bIsSource = true;
                            break;
                        }
                    }
                    if (!bIsSource)
                    {
                        Duplicates.AddUnique(Candidate);
                    }
                }

                if (Duplicates.IsEmpty() && GIndexDuplicateFinalizeAttempts < 8)
                {
                    return true;
                }

                bDuplicateInProgress = false;
                GIndexDuplicateSourceItems.Reset();
                GIndexDuplicateFinalizeAttempts = 0;
                GIndexDuplicateFinalizeTicker.Reset();

                TArray<FIndexSequenceItem> OrderedSources = Sources;
                FIndexSequenceState::CollapseToTopLevelRoots(OrderedSources);
                FIndexSequenceState::SortItemsForMove(OrderedSources);
                FIndexSequenceState::CollapseToTopLevelRoots(Duplicates);
                FIndexSequenceState::SortItemsForMove(Duplicates);
                if (Duplicates.IsEmpty())
                {
                    return false;
                }

                bool bSameSourceTier = !OrderedSources.IsEmpty();
                FIndexSequenceGroupKey SourceGroup;
                if (bSameSourceTier)
                {
                    SourceGroup = FIndexSequenceState::GetGroupKey(OrderedSources[0]);
                    for (const FIndexSequenceItem& Source : OrderedSources)
                    {
                        bSameSourceTier &= FIndexSequenceState::GetGroupKey(Source) == SourceGroup;
                    }
                    for (const FIndexSequenceItem& Duplicate : Duplicates)
                    {
                        bSameSourceTier &= FIndexSequenceState::GetGroupKey(Duplicate) == SourceGroup;
                    }
                }

                const bool bApplied = bSameSourceTier
                    ? FIndexSequenceState::InsertItemsRelative(Duplicates, OrderedSources.Last(), false)
                    : FIndexSequenceState::IntegrateItemsAtTop(Duplicates);
                if (bApplied)
                {
                    IndexInteraction::RefreshAllOutliners();
                }
                return false;
            }));
    });

    FolderMovedHandle = FActorFolders::Get().OnFolderMoved.AddLambda(
        [](UWorld& World, const FFolder& OldFolder, const FFolder& NewFolder)
        {
            IndexInteraction::HandleFolderMoved(World, OldFolder, NewFolder);
        });

    FolderDeletedHandle = FActorFolders::Get().OnFolderDeleted.AddLambda(
        [](UWorld&, const FFolder& Folder)
        {
            FIndexSequenceState::RemoveItem(FIndexSequenceItem::FromFolder(Folder));
            IndexInteraction::RefreshAllOutliners();
        });

    if (GEngine)
    {
        LevelActorAttachedHandle = GEngine->OnLevelActorAttached().AddLambda(
            [](AActor* Actor, const AActor*)
            {
                IndexInteraction::HandleNativeHierarchyChanged(Actor);
            });

        LevelActorDetachedHandle = GEngine->OnLevelActorDetached().AddLambda(
            [](AActor* Actor, const AActor*)
            {
                IndexInteraction::HandleNativeHierarchyChanged(Actor);
            });

        LevelActorDeletedHandle = GEngine->OnLevelActorDeleted().AddLambda(
            [](AActor* Actor)
            {
                IndexInteraction::HandleActorDeleted(Actor);
            });
    }
}

void FIndexModule::ShutdownModule()
{
    if (GIndexDuplicateFinalizeTicker.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(GIndexDuplicateFinalizeTicker);
        GIndexDuplicateFinalizeTicker.Reset();
    }
    GIndexDuplicateSourceItems.Reset();
    GIndexDuplicateFinalizeAttempts = 0;

    if (DuplicateActorsEndHandle.IsValid())
    {
        FEditorDelegates::OnDuplicateActorsEnd.Remove(DuplicateActorsEndHandle);
        DuplicateActorsEndHandle.Reset();
    }
    if (DuplicateActorsBeginHandle.IsValid())
    {
        FEditorDelegates::OnDuplicateActorsBegin.Remove(DuplicateActorsBeginHandle);
        DuplicateActorsBeginHandle.Reset();
    }
    bDuplicateInProgress = false;

    if (FolderDeletedHandle.IsValid())
    {
        FActorFolders::Get().OnFolderDeleted.Remove(FolderDeletedHandle);
        FolderDeletedHandle.Reset();
    }
    if (FolderMovedHandle.IsValid())
    {
        FActorFolders::Get().OnFolderMoved.Remove(FolderMovedHandle);
        FolderMovedHandle.Reset();
    }

    if (GEngine)
    {
        if (LevelActorDeletedHandle.IsValid())
        {
            GEngine->OnLevelActorDeleted().Remove(LevelActorDeletedHandle);
            LevelActorDeletedHandle.Reset();
        }
        if (LevelActorAttachedHandle.IsValid())
        {
            GEngine->OnLevelActorAttached().Remove(LevelActorAttachedHandle);
            LevelActorAttachedHandle.Reset();
        }
        if (LevelActorDetachedHandle.IsValid())
        {
            GEngine->OnLevelActorDetached().Remove(LevelActorDetachedHandle);
            LevelActorDetachedHandle.Reset();
        }
    }

    if (NewActorsPlacedHandle.IsValid())
    {
        FEditorDelegates::OnNewActorsPlaced.Remove(NewActorsPlacedHandle);
        NewActorsPlacedHandle.Reset();
    }
    if (ObjectsReplacedHandle.IsValid())
    {
        FCoreUObjectDelegates::OnObjectsReplaced.Remove(ObjectsReplacedHandle);
        ObjectsReplacedHandle.Reset();
    }
    if (MapChangeHandle.IsValid())
    {
        FEditorDelegates::MapChange.Remove(MapChangeHandle);
        MapChangeHandle.Reset();
    }
    if (PostUndoRedoHandle.IsValid())
    {
        FEditorDelegates::PostUndoRedo.Remove(PostUndoRedoHandle);
        PostUndoRedoHandle.Reset();
    }

    if (InputProcessor.IsValid() && FSlateApplication::IsInitialized())
    {
        FSlateApplication::Get().UnregisterInputPreProcessor(InputProcessor);
    }
    InputProcessor.Reset();
    IndexInteraction::Shutdown();
    FIndexSequenceState::InvalidateCache();

    if (FModuleManager::Get().IsModuleLoaded("SceneOutliner"))
    {
        FSceneOutlinerModule& SceneOutlinerModule =
            FModuleManager::GetModuleChecked<FSceneOutlinerModule>("SceneOutliner");
        if (ActorBrowserColumnsHandle.IsValid())
        {
            SceneOutlinerModule.OnCreateActorBrowserColumns().Remove(ActorBrowserColumnsHandle);
            ActorBrowserColumnsHandle.Reset();
        }
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FIndexModule, Index)
