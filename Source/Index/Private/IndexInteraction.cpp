// Copyright Epic Games, Inc. All Rights Reserved.

#include "IndexInteraction.h"

#include "Editor.h"
#include "ActorTreeItem.h"
#include "ActorFolderTreeItem.h"
#include "Containers/Ticker.h"
#include "Components/SceneComponent.h"
#include "Engine/Level.h"
#include "GameFramework/Actor.h"
#include "DragAndDrop/DecoratedDragDropOp.h"
#include "EditorActorFolders.h"
#include "Framework/Application/SlateApplication.h"
#include "IndexColumn.h"
#include "IndexLineOverlay.h"
#include "IndexSettings.h"
#include "IndexSequenceState.h"
#include "Input/DragAndDrop.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "ISceneOutliner.h"
#include "ISceneOutlinerMode.h"
#include "ISceneOutlinerTreeItem.h"
#include "Layout/WidgetPath.h"
#include "SceneOutlinerDragDrop.h"
#include "SceneOutlinerStandaloneTypes.h"
#include "ScopedTransaction.h"
#include "Selection.h"
#include "Styling/AppStyle.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/ITableRow.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/STreeView.h"
#include "SOutlinerTreeView.h"
#include "SSceneOutliner.h"

namespace IndexInteraction
{
    static TArray<TWeakPtr<ISceneOutliner>> Outliners;
    static TArray<FTSTicker::FDelegateHandle> PendingSortTickers;
    static TArray<FTSTicker::FDelegateHandle> PendingNativeTickers;

    struct FNativeDragSnapshot
    {
        TArray<FIndexSequenceItem> OrderedItems;
        TArray<FIndexSequenceGroupKey> SourceGroups;

        bool IsValid() const
        {
            return !OrderedItems.IsEmpty() && OrderedItems.Num() == SourceGroups.Num();
        }

        void Reset()
        {
            OrderedItems.Reset();
            SourceGroups.Reset();
        }
    };

    struct FPendingNativeMove
    {
        FNativeDragSnapshot Snapshot;
        bool bActive = false;
        bool bPlaceAfterTarget = false;
        bool bPlacementApplied = false;
        FIndexSequenceItem PlacementTarget;

        void Reset()
        {
            Snapshot.Reset();
            bActive = false;
            bPlaceAfterTarget = false;
            bPlacementApplied = false;
            PlacementTarget = FIndexSequenceItem();
        }
    };

    static FPendingNativeMove PendingNativeMove;
    static bool bApplyingFolderDetach = false;
    static bool bApplyingActorFolderDetach = false;
    static bool bFolderRefreshScheduled = false;

    static void ActivateIndexSort(ISceneOutliner& Outliner);
    static bool FinalizePendingNativeMove(float);

    void RegisterOutliner(ISceneOutliner& Outliner)
    {
        const TWeakPtr<ISceneOutliner> WeakOutliner =
            StaticCastSharedRef<ISceneOutliner>(Outliner.AsShared());
        Outliners.AddUnique(WeakOutliner);

        // Column factories run while the Outliner header is being assembled, so
        // defer the first sort activation until the widget has finished building.
        PendingSortTickers.Add(FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateLambda([WeakOutliner](float)
            {
                if (const TSharedPtr<ISceneOutliner> PinnedOutliner = WeakOutliner.Pin())
                {
                    ActivateIndexSort(*PinnedOutliner);
                    PinnedOutliner->RequestSort();
                    PinnedOutliner->Refresh();
                }
                return false;
            })));
    }

    void UnregisterDeadOutliners()
    {
        Outliners.RemoveAll([](const TWeakPtr<ISceneOutliner>& WeakOutliner)
        {
            return !WeakOutliner.IsValid();
        });
    }

    bool GetSelectedSequenceRoots(TArray<FIndexSequenceItem>& OutItems)
    {
        OutItems.Reset();
        UnregisterDeadOutliners();

        // Use the first live actor-browser Outliner that currently owns a selection.
        // This captures folders as well as actors, unlike GEditor actor selection.
        for (const TWeakPtr<ISceneOutliner>& WeakOutliner : Outliners)
        {
            const TSharedPtr<ISceneOutliner> Outliner = WeakOutliner.Pin();
            if (!Outliner)
            {
                continue;
            }

            const TArray<FSceneOutlinerTreeItemPtr> Selected = Outliner->GetTree().GetSelectedItems();
            if (Selected.IsEmpty())
            {
                continue;
            }

            for (const FSceneOutlinerTreeItemPtr& TreeItem : Selected)
            {
                if (!TreeItem.IsValid())
                {
                    continue;
                }
                if (const FActorTreeItem* ActorItem = TreeItem->CastTo<FActorTreeItem>();
                    ActorItem && ActorItem->IsValid())
                {
                    if (AActor* Actor = ActorItem->Actor.Get())
                    {
                        OutItems.AddUnique(FIndexSequenceItem::FromActor(Actor));
                    }
                    continue;
                }
                if (const FActorFolderTreeItem* FolderItem = TreeItem->CastTo<FActorFolderTreeItem>();
                    FolderItem && FolderItem->IsValid())
                {
                    const FFolder Folder = FolderItem->GetFolder();
                    if (Folder.IsValid())
                    {
                        OutItems.AddUnique(FIndexSequenceItem::FromFolder(Folder));
                    }
                }
            }

            if (!OutItems.IsEmpty())
            {
                FIndexSequenceState::CollapseToTopLevelRoots(OutItems);
                FIndexSequenceState::SortItemsForMove(OutItems);
                return true;
            }
        }

        // Viewport-driven actor duplication can begin without a useful folder-row
        // selection. Actor selection is synchronized through GEditor, so use it as a
        // fallback. Folder selection is Outliner-only and is never guessed.
        if (GEditor && GEditor->GetSelectedActors())
        {
            for (FSelectionIterator It(*GEditor->GetSelectedActors()); It; ++It)
            {
                if (AActor* Actor = Cast<AActor>(*It))
                {
                    OutItems.AddUnique(FIndexSequenceItem::FromActor(Actor));
                }
            }
        }
        if (!OutItems.IsEmpty())
        {
            FIndexSequenceState::CollapseToTopLevelRoots(OutItems);
            FIndexSequenceState::SortItemsForMove(OutItems);
            return true;
        }
        return false;
    }

    static void ActivateIndexSort(ISceneOutliner& Outliner)
    {
        // The Index column is intentionally hidden, so the user cannot click its
        // header to make it the active sort column. Execute the delegate Unreal
        // already bound to that header column instead of reaching into
        // SSceneOutliner's protected SortByColumn/SortMode state.
        const TSharedPtr<SHeaderRow> HeaderRow = Outliner.GetTree().GetHeaderRow();
        if (!HeaderRow)
        {
            return;
        }

        const int32 ColumnIndex = HeaderRow->FindColumnIndex(FIndexColumn::GetID());
        if (ColumnIndex == INDEX_NONE)
        {
            return;
        }

        const auto& Columns = HeaderRow->GetColumns();
        if (ColumnIndex < 0 || ColumnIndex >= Columns.Num())
        {
            return;
        }

        Columns[ColumnIndex].OnSortModeChanged.ExecuteIfBound(
            EColumnSortPriority::Primary,
            FIndexColumn::GetID(),
            EColumnSortMode::Ascending);
    }

    void RefreshAllOutliners()
    {
        UnregisterDeadOutliners();
        for (const TWeakPtr<ISceneOutliner>& WeakOutliner : Outliners)
        {
            if (const TSharedPtr<ISceneOutliner> Outliner = WeakOutliner.Pin())
            {
                ActivateIndexSort(*Outliner);
                Outliner->RequestSort();
                Outliner->Refresh();
            }
        }
    }

    static void ShowRenameFlashNow(const FIndexSequenceItem& InItem)
    {
        const FIndexSequenceItem Item = FIndexSequenceState::ResolveCurrentItem(InItem);
        if (!Item.IsValid())
        {
            return;
        }

        // Use the Scene Outliner's own changed-item highlight instead of tinting the
        // table-row style. The native highlight is painted as a dedicated row effect,
        // so selection colors do not multiply with Index's rename color and turn
        // selected folders/new items green. This also makes folder rename feedback
        // match the actor rename feedback Unreal already provides.
        UnregisterDeadOutliners();
        for (const TWeakPtr<ISceneOutliner>& WeakOutliner : Outliners)
        {
            const TSharedPtr<ISceneOutliner> Outliner = WeakOutliner.Pin();
            if (!Outliner)
            {
                continue;
            }

            const FSceneOutlinerTreeItemID ItemId = Item.IsActor()
                ? FSceneOutlinerTreeItemID(Item.Actor.Get())
                : FSceneOutlinerTreeItemID(Item.Folder);
            const FSceneOutlinerTreeItemPtr TreeItem = Outliner->GetTreeItem(ItemId, true);
            if (!TreeItem)
            {
                continue;
            }

            const TSharedPtr<SSceneOutliner> SceneOutliner = StaticCastSharedPtr<SSceneOutliner>(Outliner);
            if (SceneOutliner)
            {
                // OnItemLabelChanged is exported by the SceneOutliner module. Calling
                // it here lets the module invoke its own non-exported tree-view flash
                // implementation internally, avoiding an unresolved external symbol
                // while still using Unreal's native changed-item animation.
                SceneOutliner->OnItemLabelChanged(TreeItem, true);
            }
        }
    }

    void FlashRenameItem(const FIndexSequenceItem& Item)
    {
        if (!Item.IsValid())
        {
            return;
        }

        // Folder rename rebuilds its tree row as part of the same editor operation.
        // Defer the flash one Slate tick so the current row exists and has final geometry.
        PendingSortTickers.Add(FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateLambda([Item](float)
            {
                ShowRenameFlashNow(Item);
                return false;
            })));
    }

    static void ScheduleFolderRefresh()
    {
        if (bFolderRefreshScheduled)
        {
            return;
        }

        bFolderRefreshScheduled = true;
        PendingSortTickers.Add(FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateLambda([](float)
            {
                bFolderRefreshScheduled = false;
                RefreshAllOutliners();
                return false;
            })));
    }

    enum class EDropZone : uint8
    {
        Native,
        Before,
        After
    };

    struct FDropTarget
    {
        TSharedPtr<ISceneOutliner> Outliner;
        TSharedPtr<STableRow<FSceneOutlinerTreeItemPtr>> Row;
        FSceneOutlinerTreeItemPtr Item;
        EDropZone VisualZone = EDropZone::Native;
        bool bInsertBefore = false;
        TArray<FIndexSequenceItem> MovedItems;
        FIndexSequenceItem TargetItem;

        bool IsCustomReorder() const
        {
            return VisualZone != EDropZone::Native && TargetItem.IsValid() && !MovedItems.IsEmpty();
        }

        void Reset()
        {
            Outliner.Reset();
            Row.Reset();
            Item.Reset();
            VisualZone = EDropZone::Native;
            bInsertBefore = false;
            MovedItems.Reset();
            TargetItem = FIndexSequenceItem();
        }
    };

    struct FNativeDropPreview
    {
        TSharedPtr<STableRow<FSceneOutlinerTreeItemPtr>> Row;
        FText Message;
        bool bValid = false;

        bool IsSet() const
        {
            return Row.IsValid() && !Message.IsEmpty();
        }

        void Reset()
        {
            Row.Reset();
            Message = FText::GetEmpty();
            bValid = false;
        }
    };

    enum class ESpecialDropAction : uint8
    {
        None,
        ParentToActor,
        MoveIntoFolder,
        ActorDetach,
        ActorDetachToFolder,
        ActorMoveOutOfFolder,
        FolderDetach
    };

    struct FSpecialDropTarget
    {
        TSharedPtr<ISceneOutliner> Outliner;
        TSharedPtr<STableRow<FSceneOutlinerTreeItemPtr>> Row;
        ESpecialDropAction Action = ESpecialDropAction::None;
        TArray<FIndexSequenceItem> MovedItems;
        FIndexSequenceItem TargetItem;
        FIndexSequenceItem PlacementTargetItem;

        bool IsValid() const
        {
            return Action != ESpecialDropAction::None
                && Row.IsValid()
                && TargetItem.IsValid()
                && !MovedItems.IsEmpty();
        }

        void Reset()
        {
            Outliner.Reset();
            Row.Reset();
            Action = ESpecialDropAction::None;
            MovedItems.Reset();
            TargetItem = FIndexSequenceItem();
            PlacementTargetItem = FIndexSequenceItem();
        }
    };

    struct FRootDropTarget
    {
        TSharedPtr<ISceneOutliner> Outliner;
        TArray<FIndexSequenceItem> MovedItems;
        TWeakObjectPtr<ULevel> Level;
        FFolder::FRootObject RootObject = FFolder::GetInvalidRootObject();
        FGeometry TreeGeometry;

        bool IsValid() const
        {
            return Outliner.IsValid() && Level.IsValid() && !MovedItems.IsEmpty();
        }

        void Reset()
        {
            Outliner.Reset();
            MovedItems.Reset();
            Level.Reset();
            RootObject = FFolder::GetInvalidRootObject();
            TreeGeometry = FGeometry();
        }
    };

    static int32 GetItemHierarchyDepth(const FIndexSequenceItem& Item)
    {
        if (!Item.IsValid())
        {
            return 0;
        }

        int32 Depth = 0;
        FIndexSequenceItem Current = Item;
        for (int32 Guard = 0; Guard < 64 && Current.IsValid(); ++Guard)
        {
            if (Current.IsActor())
            {
                AActor* Actor = Current.Actor.Get();
                if (!IsValid(Actor))
                {
                    break;
                }
                if (AActor* ParentActor = Actor->GetAttachParentActor())
                {
                    ++Depth;
                    Current = FIndexSequenceItem::FromActor(ParentActor);
                    continue;
                }
                const FFolder Folder = Actor->GetFolder();
                if (Folder.IsValid() && !Folder.IsNone())
                {
                    ++Depth;
                    Current = FIndexSequenceItem::FromFolder(Folder);
                    continue;
                }
                break;
            }

            const FFolder Parent = Current.Folder.GetParent();
            if (Parent.IsValid() && !Parent.IsNone())
            {
                ++Depth;
                Current = FIndexSequenceItem::FromFolder(Parent);
                continue;
            }
            break;
        }
        return Depth;
    }

    static float GetInsertionLineInset(const FIndexSequenceItem& Item)
    {
        // SOutlinerTreeView uses a 12 px expander indent per hierarchy level. Match
        // that step so the insertion line communicates the destination tier.
        constexpr float OutlinerIndentPerLevel = 12.0f;
        return static_cast<float>(GetItemHierarchyDepth(Item)) * OutlinerIndentPerLevel;
    }

    static FText GetItemDisplayText(const FIndexSequenceItem& Item)
    {
        if (Item.IsActor())
        {
            if (const AActor* Actor = Item.Actor.Get())
            {
                return FText::FromString(Actor->GetActorLabel());
            }
        }
        else if (Item.IsFolder() && Item.Folder.IsValid())
        {
            return FText::FromName(Item.Folder.GetLeafName());
        }
        return FText::GetEmpty();
    }

    static void SetOperationToolTip(
        const TSharedPtr<FDragDropOperation>& Operation,
        const FText& Text,
        bool bValid = true)
    {
        if (!Operation || Text.IsEmpty())
        {
            return;
        }

        const FSlateBrush* Icon = FAppStyle::GetBrush(
            bValid ? TEXT("Graph.ConnectorFeedback.OK") : TEXT("Graph.ConnectorFeedback.Error"));

        if (Operation->IsOfType<FSceneOutlinerDragDropOp>())
        {
            static_cast<FSceneOutlinerDragDropOp*>(Operation.Get())->SetTooltip(Text, Icon);
        }
        else if (Operation->IsOfType<FDecoratedDragDropOp>())
        {
            static_cast<FDecoratedDragDropOp*>(Operation.Get())->SetToolTip(Text, Icon);
        }
    }

    static void ResetOperationToolTip(const TSharedPtr<FDragDropOperation>& Operation)
    {
        if (!Operation)
        {
            return;
        }

        if (Operation->IsOfType<FSceneOutlinerDragDropOp>())
        {
            static_cast<FSceneOutlinerDragDropOp*>(Operation.Get())->ResetTooltip();
        }
        else if (Operation->IsOfType<FDecoratedDragDropOp>())
        {
            static_cast<FDecoratedDragDropOp*>(Operation.Get())->ResetToDefaultToolTip();
        }
    }

    static bool PathContains(const FWidgetPath& Path, const SWidget* Widget)
    {
        if (!Widget)
        {
            return false;
        }

        for (int32 Index = 0; Index < Path.Widgets.Num(); ++Index)
        {
            if (&Path.Widgets[Index].Widget.Get() == Widget)
            {
                return true;
            }
        }
        return false;
    }

    static bool PathContains(const FWidgetPath& Path, const TSharedPtr<SWidget>& Widget)
    {
        return PathContains(Path, Widget.Get());
    }

    static bool IsModifierDown(const FPointerEvent& Event)
    {
        switch (UIndexSettings::Get()->ReorderModifierKey)
        {
        case EIndexModifierKey::Alt:
            return Event.IsAltDown();
        case EIndexModifierKey::Shift:
            return Event.IsShiftDown();
        case EIndexModifierKey::Ctrl:
            return Event.IsControlDown();
        case EIndexModifierKey::None:
        default:
            return false;
        }
    }

    static EDropZone ResolveZone(const FGeometry& Geometry, const FPointerEvent& Event)
    {
        const FVector2D LocalPosition = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
        const float Height = Geometry.GetLocalSize().Y;
        if (Height <= KINDA_SMALL_NUMBER)
        {
            return EDropZone::Native;
        }

        const float Fraction = FMath::Clamp(LocalPosition.Y / Height, 0.0f, 1.0f);
        if (IsModifierDown(Event))
        {
            return Fraction < 0.5f ? EDropZone::Before : EDropZone::After;
        }

        const float EdgeFraction = FMath::Clamp(
            UIndexSettings::Get()->EdgeThresholdPercent / 100.0f,
            0.01f,
            0.45f);

        if (Fraction <= EdgeFraction)
        {
            return EDropZone::Before;
        }
        if (Fraction >= 1.0f - EdgeFraction)
        {
            return EDropZone::After;
        }
        return EDropZone::Native;
    }

    static bool BuildActorPayload(
        ISceneOutliner& Outliner,
        const TSharedPtr<FDragDropOperation>& Operation,
        TArray<AActor*>& OutActors)
    {
        OutActors.Reset();
        if (!Operation || !Outliner.GetMode())
        {
            return false;
        }

        FSceneOutlinerDragDropPayload Payload(*Operation);
        if (!Outliner.GetMode()->ParseDragDrop(Payload, *Operation))
        {
            return false;
        }

        for (const TWeakPtr<ISceneOutlinerTreeItem>& WeakItem : Payload.DraggedItems)
        {
            const FSceneOutlinerTreeItemPtr DraggedItem = WeakItem.Pin();
            if (!DraggedItem)
            {
                continue;
            }

            const FActorTreeItem* ActorItem = DraggedItem->CastTo<FActorTreeItem>();
            if (!ActorItem || !ActorItem->IsValid())
            {
                // This native hierarchy snapshot path is intentionally actor-only.
                OutActors.Reset();
                return false;
            }

            if (AActor* Actor = ActorItem->Actor.Get())
            {
                OutActors.AddUnique(Actor);
            }
        }

        return !OutActors.IsEmpty();
    }

    static bool BuildSequencePayload(
        ISceneOutliner& Outliner,
        const TSharedPtr<FDragDropOperation>& Operation,
        TArray<FIndexSequenceItem>& OutItems)
    {
        OutItems.Reset();
        if (!Operation || !Outliner.GetMode())
        {
            return false;
        }

        FSceneOutlinerDragDropPayload Payload(*Operation);
        if (!Outliner.GetMode()->ParseDragDrop(Payload, *Operation))
        {
            return false;
        }

        for (const TWeakPtr<ISceneOutlinerTreeItem>& WeakItem : Payload.DraggedItems)
        {
            const FSceneOutlinerTreeItemPtr DraggedItem = WeakItem.Pin();
            if (!DraggedItem)
            {
                continue;
            }

            if (const FActorTreeItem* ActorItem = DraggedItem->CastTo<FActorTreeItem>();
                ActorItem && ActorItem->IsValid())
            {
                if (AActor* Actor = ActorItem->Actor.Get())
                {
                    OutItems.AddUnique(FIndexSequenceItem::FromActor(Actor));
                }
                continue;
            }

            if (const FActorFolderTreeItem* FolderItem = DraggedItem->CastTo<FActorFolderTreeItem>();
                FolderItem && FolderItem->IsValid())
            {
                const FFolder Folder = FolderItem->GetFolder();
                if (Folder.IsValid())
                {
                    OutItems.AddUnique(FIndexSequenceItem::FromFolder(Folder));
                }
                continue;
            }

            // Components and other Scene Outliner item types remain entirely native.
            OutItems.Reset();
            return false;
        }

        FIndexSequenceState::CollapseToTopLevelRoots(OutItems);
        FIndexSequenceState::SortItemsForMove(OutItems);
        return !OutItems.IsEmpty();
    }

    static bool CaptureNativeDragSnapshot(
        FSlateApplication& App,
        const FPointerEvent& Event,
        const TSharedPtr<FDragDropOperation>& Operation,
        FNativeDragSnapshot& OutSnapshot)
    {
        OutSnapshot.Reset();
        if (!Operation)
        {
            return false;
        }

        UnregisterDeadOutliners();
        const FWidgetPath Path = App.LocateWindowUnderMouse(
            Event.GetScreenSpacePosition(),
            App.GetInteractiveTopLevelWindows(),
            false,
            Event.GetUserIndex());

        for (const TWeakPtr<ISceneOutliner>& WeakOutliner : Outliners)
        {
            const TSharedPtr<ISceneOutliner> Outliner = WeakOutliner.Pin();
            if (!Outliner || !PathContains(Path, Outliner))
            {
                continue;
            }

            TArray<FIndexSequenceItem> Items;
            if (!BuildSequencePayload(*Outliner, Operation, Items))
            {
                continue;
            }
            FIndexSequenceState::CollapseToTopLevelRoots(Items);
            FIndexSequenceState::SortItemsForMove(Items);
            for (const FIndexSequenceItem& Item : Items)
            {
                OutSnapshot.OrderedItems.Add(Item);
                OutSnapshot.SourceGroups.Add(FIndexSequenceState::GetGroupKey(Item));
            }
            return OutSnapshot.IsValid();
        }
        return false;
    }

    static bool ResolveFolderByPath(
        UWorld& World,
        const FFolder::FRootObject& RootObject,
        const FName& Path,
        FFolder& OutFolder)
    {
        bool bFound = false;
        FActorFolders::Get().ForEachFolder(World, [&](const FFolder& Folder)
        {
            if (Folder.IsValid()
                && Folder.GetRootObject() == RootObject
                && Folder.GetPath() == Path)
            {
                OutFolder = Folder;
                bFound = true;
                return false;
            }
            return true;
        });
        return bFound;
    }

    static FName BuildDetachedFolderPath(const FFolder& ChildFolder, const FFolder& ParentFolder)
    {
        const FFolder GrandParent = ParentFolder.GetParent();
        const FString Leaf = ChildFolder.GetLeafName().ToString();
        if (!GrandParent.IsValid() || GrandParent.GetPath().IsNone())
        {
            return FName(*Leaf);
        }

        return FName(*(GrandParent.GetPath().ToString() / Leaf));
    }

    static bool SameFolderPath(const FFolder& A, const FFolder& B)
    {
        if (!A.IsValid() || !B.IsValid())
        {
            return !A.IsValid() && !B.IsValid();
        }
        return A.GetRootObject() == B.GetRootObject() && A.GetPath() == B.GetPath();
    }

    static FRootDropTarget FindRootDropTarget(
        FSlateApplication& App,
        const FPointerEvent& Event,
        const TSharedPtr<FDragDropOperation>& Operation)
    {
        FRootDropTarget Result;
        if (!UIndexSettings::Get()->bEnableCustomOrdering || !Operation)
        {
            return Result;
        }

        UnregisterDeadOutliners();
        const FWidgetPath Path = App.LocateWindowUnderMouse(
            Event.GetScreenSpacePosition(),
            App.GetInteractiveTopLevelWindows(),
            false,
            Event.GetUserIndex());

        for (const TWeakPtr<ISceneOutliner>& WeakOutliner : Outliners)
        {
            const TSharedPtr<ISceneOutliner> Outliner = WeakOutliner.Pin();
            if (!Outliner || !Outliner->GetMode())
            {
                continue;
            }

            const SWidget* TreeWidget = static_cast<const SWidget*>(&Outliner->GetTree());
            if (!PathContains(Path, TreeWidget))
            {
                continue;
            }

            bool bOverRow = false;
            for (int32 WidgetIndex = 0; WidgetIndex < Path.Widgets.Num(); ++WidgetIndex)
            {
                const FArrangedWidget& Arranged = Path.Widgets[WidgetIndex];
                if (Arranged.Widget->GetType() == FName(TEXT("SSceneOutlinerTreeRow")))
                {
                    bOverRow = true;
                    break;
                }
            }
            if (bOverRow)
            {
                continue;
            }

            TArray<FIndexSequenceItem> MovedItems;
            if (!BuildSequencePayload(*Outliner, Operation, MovedItems) || MovedItems.IsEmpty())
            {
                continue;
            }

            ULevel* Level = nullptr;
            bool bSameLevel = true;
            for (const FIndexSequenceItem& Item : MovedItems)
            {
                ULevel* ItemLevel = Item.IsActor()
                    ? (Item.Actor.IsValid() ? Item.Actor->GetLevel() : nullptr)
                    : Item.Folder.GetRootObjectAssociatedLevel();
                if (!Level)
                {
                    Level = ItemLevel;
                }
                else if (Level != ItemLevel)
                {
                    bSameLevel = false;
                    break;
                }
            }
            if (!bSameLevel || !Level)
            {
                continue;
            }

            Result.Outliner = Outliner;
            Result.MovedItems = MoveTemp(MovedItems);
            Result.Level = Level;
            Result.RootObject = Outliner->GetMode()->GetRootObject();
            Result.TreeGeometry = Outliner->GetTree().GetCachedGeometry();
            return Result;
        }
        return Result;
    }

    static FSpecialDropTarget FindSpecialDropTarget(
        FSlateApplication& App,
        const FPointerEvent& Event,
        const TSharedPtr<FDragDropOperation>& Operation)
    {
        FSpecialDropTarget Result;
        if (!UIndexSettings::Get()->bEnableCustomOrdering || !Operation)
        {
            return Result;
        }

        UnregisterDeadOutliners();
        const FWidgetPath Path = App.LocateWindowUnderMouse(
            Event.GetScreenSpacePosition(),
            App.GetInteractiveTopLevelWindows(),
            false,
            Event.GetUserIndex());

        for (const TWeakPtr<ISceneOutliner>& WeakOutliner : Outliners)
        {
            const TSharedPtr<ISceneOutliner> Outliner = WeakOutliner.Pin();
            if (!Outliner || !PathContains(Path, Outliner) || !Outliner->GetMode())
            {
                continue;
            }

            TSharedPtr<STableRow<FSceneOutlinerTreeItemPtr>> Row;
            for (int32 Index = Path.Widgets.Num() - 1; Index >= 0; --Index)
            {
                const TSharedRef<SWidget> Widget = Path.Widgets[Index].Widget;
                if (Widget->GetType() == FName(TEXT("SSceneOutlinerTreeRow")))
                {
                    Row = StaticCastSharedRef<STableRow<FSceneOutlinerTreeItemPtr>>(Widget);
                    break;
                }
            }
            if (!Row)
            {
                continue;
            }

            const FSceneOutlinerTreeItemPtr* ItemPtr = Outliner->GetTree().ItemFromWidget(Row.Get());
            if (!ItemPtr || !*ItemPtr)
            {
                continue;
            }

            // Structural actions belong exclusively to the center of a row. The
            // top/bottom edge zones are always Index insertion slots, even when
            // Unreal says the same payload could attach/detach on this row. This is
            // what allows an item to move directly between parent, grandparent and
            // sibling tiers at an exact position instead of the native parenting
            // preview stealing nearly every row. Alt/forced-reorder therefore also
            // suppresses structural drops across the whole row.
            if (ResolveZone(Row->GetCachedGeometry(), Event) != EDropZone::Native)
            {
                continue;
            }

            TArray<FIndexSequenceItem> MovedItems;
            if (!BuildSequencePayload(*Outliner, Operation, MovedItems))
            {
                continue;
            }

            // Actor detach is a real native Unreal operation. Index only augments
            // its final manual-order position, so the actual hierarchy change still
            // goes through FActorMode::OnDrop.
            FSceneOutlinerDragDropPayload NativePayload(*Operation);
            if (Outliner->GetMode()->ParseDragDrop(NativePayload, *Operation))
            {
                const FSceneOutlinerDragValidationInfo NativeValidation =
                    Outliner->GetMode()->ValidateDrop(**ItemPtr, NativePayload);
                if (NativeValidation.CompatibilityType == ESceneOutlinerDropCompatibility::CompatibleDetach
                    || NativeValidation.CompatibilityType == ESceneOutlinerDropCompatibility::CompatibleMultipleDetach)
                {
                    FActorTreeItem* ActorItem = (*ItemPtr)->CastTo<FActorTreeItem>();
                    if (ActorItem && ActorItem->IsValid())
                    {
                        bool bActorsOnly = true;
                        for (const FIndexSequenceItem& MovedItem : MovedItems)
                        {
                            bActorsOnly &= MovedItem.IsActor();
                        }
                        if (bActorsOnly)
                        {
                            Result.Outliner = Outliner;
                            Result.Row = Row;
                            Result.Action = ESpecialDropAction::ActorDetach;
                            Result.MovedItems = MoveTemp(MovedItems);
                            Result.TargetItem = FIndexSequenceItem::FromActor(ActorItem->Actor.Get());
                            return Result;
                        }
                    }
                }
                if (NativeValidation.CompatibilityType == ESceneOutlinerDropCompatibility::CompatibleAttach
                    || NativeValidation.CompatibilityType == ESceneOutlinerDropCompatibility::CompatibleMultipleAttach)
                {
                    FActorTreeItem* ActorItem = (*ItemPtr)->CastTo<FActorTreeItem>();
                    bool bActorsOnly = ActorItem && ActorItem->IsValid() && !MovedItems.IsEmpty();
                    for (const FIndexSequenceItem& MovedItem : MovedItems)
                    {
                        bActorsOnly &= MovedItem.IsActor();
                    }
                    if (bActorsOnly)
                    {
                        Result.Outliner = Outliner;
                        Result.Row = Row;
                        Result.Action = ESpecialDropAction::ParentToActor;
                        Result.MovedItems = MoveTemp(MovedItems);
                        Result.TargetItem = FIndexSequenceItem::FromActor(ActorItem->Actor.Get());
                        return Result;
                    }
                }
            }

            FActorFolderTreeItem* FolderItem = (*ItemPtr)->CastTo<FActorFolderTreeItem>();
            if (FolderItem && FolderItem->IsValid())
            {
                const FFolder TargetFolder = FolderItem->GetFolder();
                if (TargetFolder.IsValid()
                    && ResolveZone(Row->GetCachedGeometry(), Event) == EDropZone::Native)
                {
                    // Dropping an attached actor onto a folder is a native Unreal
                    // move-to-folder operation that should also detach it from its
                    // current actor parent. Own this center-drop explicitly so Index
                    // cannot accidentally leave the attachment behind.
                    bool bActorsOnly = !MovedItems.IsEmpty();
                    bool bHasExternalAttachment = false;
                    TSet<AActor*> MovedActors;
                    for (const FIndexSequenceItem& MovedItem : MovedItems)
                    {
                        if (!MovedItem.IsActor() || !MovedItem.Actor.IsValid())
                        {
                            bActorsOnly = false;
                            break;
                        }
                        AActor* MovedActor = MovedItem.Actor.Get();
                        if (MovedActor->GetLevel() != TargetFolder.GetRootObjectAssociatedLevel()
                            || MovedActor->GetFolderRootObject() != TargetFolder.GetRootObject())
                        {
                            bActorsOnly = false;
                            break;
                        }
                        MovedActors.Add(MovedActor);
                    }

                    AActor* CommonFormerParent = nullptr;
                    bool bCommonFormerParent = true;
                    if (bActorsOnly)
                    {
                        for (const FIndexSequenceItem& MovedItem : MovedItems)
                        {
                            AActor* Actor = MovedItem.Actor.Get();
                            AActor* Parent = Actor ? Actor->GetAttachParentActor() : nullptr;
                            if (Parent && !MovedActors.Contains(Parent))
                            {
                                bHasExternalAttachment = true;
                                if (!CommonFormerParent)
                                {
                                    CommonFormerParent = Parent;
                                }
                                else if (CommonFormerParent != Parent)
                                {
                                    bCommonFormerParent = false;
                                }
                            }
                        }
                    }

                    // Stock Unreal can report an attached actor as "already in this
                    // folder" even though the meaningful user gesture is to release
                    // it from its actor parent. Index owns this one center-drop case
                    // explicitly, so do not gate it on the native folder validation.
                    if (bActorsOnly && bHasExternalAttachment)
                    {
                        Result.Outliner = Outliner;
                        Result.Row = Row;
                        Result.Action = ESpecialDropAction::ActorDetachToFolder;
                        Result.MovedItems = MovedItems;
                        Result.TargetItem = FIndexSequenceItem::FromFolder(TargetFolder);

                        if (bCommonFormerParent && IsValid(CommonFormerParent))
                        {
                            const FIndexSequenceItem ParentItem = FIndexSequenceItem::FromActor(CommonFormerParent);
                            const FIndexSequenceGroupKey TargetGroup = FIndexSequenceState::GetGroupKey(Result.TargetItem);
                            if (FIndexSequenceState::GetGroupKey(ParentItem) == TargetGroup)
                            {
                                Result.PlacementTargetItem = ParentItem;
                            }
                        }
                        return Result;
                    }
                }
            }

            // An un-attached actor dropped onto the folder it already belongs to is
            // the folder equivalent of dropping an actor child onto its actor parent:
            // release it one folder level and place it immediately after that folder.
            if (FolderItem && FolderItem->IsValid()
                && ResolveZone(Row->GetCachedGeometry(), Event) == EDropZone::Native)
            {
                const FFolder CurrentFolderTarget = FolderItem->GetFolder();
                bool bDirectActorChildren = !MovedItems.IsEmpty() && CurrentFolderTarget.IsValid();
                for (const FIndexSequenceItem& MovedItem : MovedItems)
                {
                    AActor* Actor = MovedItem.IsActor() ? MovedItem.Actor.Get() : nullptr;
                    if (!IsValid(Actor)
                        || Actor->GetAttachParentActor()
                        || !SameFolderPath(Actor->GetFolder(), CurrentFolderTarget))
                    {
                        bDirectActorChildren = false;
                        break;
                    }
                }

                if (bDirectActorChildren)
                {
                    Result.Outliner = Outliner;
                    Result.Row = Row;
                    Result.Action = ESpecialDropAction::ActorMoveOutOfFolder;
                    Result.MovedItems = MoveTemp(MovedItems);
                    Result.TargetItem = FIndexSequenceItem::FromFolder(CurrentFolderTarget);
                    return Result;
                }
            }

            // Stock Unreal considers dropping a child folder onto its own parent an
            // invalid no-op. Index makes this symmetrical with actor detach: move the
            // child folder up one level and place it immediately after the old parent.
            if (!FolderItem || !FolderItem->IsValid())
            {
                continue;
            }

            const FFolder TargetFolder = FolderItem->GetFolder();
            if (!TargetFolder.IsValid())
            {
                continue;
            }

            bool bFoldersOnly = !MovedItems.IsEmpty();
            ULevel* TargetLevel = TargetFolder.GetRootObjectAssociatedLevel();
            UWorld* World = TargetLevel ? TargetLevel->GetWorld() : nullptr;
            if (!World)
            {
                continue;
            }

            for (const FIndexSequenceItem& MovedItem : MovedItems)
            {
                if (!MovedItem.IsFolder()
                    || !MovedItem.Folder.IsValid()
                    || !SameFolderPath(MovedItem.Folder.GetParent(), TargetFolder))
                {
                    bFoldersOnly = false;
                    break;
                }

                const FName NewPath = BuildDetachedFolderPath(MovedItem.Folder, TargetFolder);
                const FFolder Candidate(TargetFolder.GetRootObject(), NewPath);
                if (FActorFolders::Get().ContainsFolder(*World, Candidate))
                {
                    bFoldersOnly = false;
                    break;
                }
            }

            if (bFoldersOnly)
            {
                Result.Outliner = Outliner;
                Result.Row = Row;
                Result.Action = ESpecialDropAction::FolderDetach;
                Result.MovedItems = MoveTemp(MovedItems);
                Result.TargetItem = FIndexSequenceItem::FromFolder(TargetFolder);
                return Result;
            }

            // All remaining valid center-drops onto a folder are structural moves.
            // Index owns the mutation using only the collapsed highest selected roots,
            // so selected descendants cannot be flattened or absorbed accidentally.
            FSceneOutlinerDragDropPayload FolderPayload(*Operation);
            if (Outliner->GetMode()->ParseDragDrop(FolderPayload, *Operation))
            {
                const FSceneOutlinerDragValidationInfo FolderValidation =
                    Outliner->GetMode()->ValidateDrop(**ItemPtr, FolderPayload);
                if (FolderValidation.IsValid())
                {
                    Result.Outliner = Outliner;
                    Result.Row = Row;
                    Result.Action = ESpecialDropAction::MoveIntoFolder;
                    Result.MovedItems = MoveTemp(MovedItems);
                    Result.TargetItem = FIndexSequenceItem::FromFolder(TargetFolder);
                    return Result;
                }
            }
        }

        return Result;
    }

    static bool ApplyParentToTarget(const FSpecialDropTarget& Target)
    {
        if (!Target.IsValid()
            || (Target.Action != ESpecialDropAction::ParentToActor
                && Target.Action != ESpecialDropAction::MoveIntoFolder))
        {
            return false;
        }

        const bool bApplied = FIndexSequenceState::MoveItemsToParentAtTop(
            Target.MovedItems,
            Target.TargetItem);
        if (bApplied)
        {
            RefreshAllOutliners();
        }
        return bApplied;
    }

    static bool ApplyActorDetach(const FSpecialDropTarget& Target)
    {
        if (!Target.IsValid() || Target.Action != ESpecialDropAction::ActorDetach)
        {
            return false;
        }

        // The target parent itself belongs to the destination sibling tier. Inserting
        // after it both detaches the selected highest roots and gives the exact
        // placement the user saw in the preview. Descendant attachments are untouched.
        const bool bApplied = FIndexSequenceState::InsertItemsRelative(
            Target.MovedItems,
            Target.TargetItem,
            false);
        if (bApplied)
        {
            RefreshAllOutliners();
        }
        return bApplied;
    }

    static bool ApplyActorDetachToFolder(const FSpecialDropTarget& Target)
    {
        if (!Target.IsValid()
            || Target.Action != ESpecialDropAction::ActorDetachToFolder
            || !Target.TargetItem.IsFolder())
        {
            return false;
        }

        const FFolder DestinationFolder = Target.TargetItem.Folder;
        if (!DestinationFolder.IsValid())
        {
            return false;
        }

        TArray<FIndexSequenceItem> OrderedMoved = Target.MovedItems;
        FIndexSequenceState::SortItemsForMove(OrderedMoved);

        TSet<AActor*> MovedActors;
        for (const FIndexSequenceItem& Item : OrderedMoved)
        {
            if (!Item.IsActor() || !Item.Actor.IsValid())
            {
                return false;
            }
            MovedActors.Add(Item.Actor.Get());
        }

        const FScopedTransaction Transaction(
            FText::FromString(TEXT("Index: Move Actors to Folder")));

        struct FActorFolderDetachGuard
        {
            FActorFolderDetachGuard() { bApplyingActorFolderDetach = true; }
            ~FActorFolderDetachGuard() { bApplyingActorFolderDetach = false; }
        } Guard;

        TArray<FIndexSequenceItem> DetachedTopLevelItems;
        for (const FIndexSequenceItem& Item : OrderedMoved)
        {
            AActor* Actor = Item.Actor.Get();
            if (!IsValid(Actor))
            {
                continue;
            }

            AActor* Parent = Actor->GetAttachParentActor();
            const bool bParentMovesWithSelection = Parent && MovedActors.Contains(Parent);
            if (Parent && !bParentMovesWithSelection)
            {
                Parent->Modify(false);
                Actor->Modify();
                if (USceneComponent* RootComponent = Actor->GetRootComponent())
                {
                    RootComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
                }
                DetachedTopLevelItems.Add(FIndexSequenceItem::FromActor(Actor));
            }
        }

        // Keep the selected block's descendants in the destination folder as well.
        for (const FIndexSequenceItem& Item : OrderedMoved)
        {
            if (AActor* Actor = Item.Actor.Get())
            {
                Actor->SetFolderPath_Recursively(DestinationFolder.GetPath());
            }
        }

        if (DetachedTopLevelItems.IsEmpty())
        {
            return false;
        }

        bool bOrdered = false;
        if (Target.PlacementTargetItem.IsValid())
        {
            bOrdered = FIndexSequenceState::InsertItemsRelativeInCurrentTransaction(
                DetachedTopLevelItems,
                Target.PlacementTargetItem,
                false);
        }
        else
        {
            bOrdered = FIndexSequenceState::IntegrateItemsAtTop(DetachedTopLevelItems);
        }

        RefreshAllOutliners();
        return bOrdered || !DetachedTopLevelItems.IsEmpty();
    }

    static bool ApplyActorMoveOutOfFolder(const FSpecialDropTarget& Target)
    {
        if (!Target.IsValid()
            || Target.Action != ESpecialDropAction::ActorMoveOutOfFolder
            || !Target.TargetItem.IsFolder())
        {
            return false;
        }

        const FFolder ParentFolder = Target.TargetItem.Folder;
        if (!ParentFolder.IsValid())
        {
            return false;
        }

        const FFolder DestinationFolder = ParentFolder.GetParent();
        const FName DestinationPath = DestinationFolder.IsValid()
            ? DestinationFolder.GetPath()
            : FFolder::GetEmptyPath();

        TArray<FIndexSequenceItem> OrderedMoved = Target.MovedItems;
        FIndexSequenceState::SortItemsForMove(OrderedMoved);

        const FScopedTransaction Transaction(
            FText::FromString(TEXT("Index: Move Actors Out of Folder")));

        struct FActorFolderDetachGuard
        {
            FActorFolderDetachGuard() { bApplyingActorFolderDetach = true; }
            ~FActorFolderDetachGuard() { bApplyingActorFolderDetach = false; }
        } Guard;

        TArray<FIndexSequenceItem> MovedActors;
        for (const FIndexSequenceItem& Item : OrderedMoved)
        {
            AActor* Actor = Item.IsActor() ? Item.Actor.Get() : nullptr;
            if (!IsValid(Actor) || Actor->GetAttachParentActor() || !SameFolderPath(Actor->GetFolder(), ParentFolder))
            {
                continue;
            }

            Actor->Modify();
            Actor->SetFolderPath_Recursively(DestinationPath);
            MovedActors.Add(FIndexSequenceItem::FromActor(Actor));
        }

        if (MovedActors.IsEmpty())
        {
            return false;
        }

        const bool bOrdered = FIndexSequenceState::InsertItemsRelativeInCurrentTransaction(
            MovedActors,
            Target.TargetItem,
            false);
        RefreshAllOutliners();
        return bOrdered || !MovedActors.IsEmpty();
    }

    static bool ApplyFolderDetach(const FSpecialDropTarget& Target)
    {
        if (!Target.IsValid()
            || Target.Action != ESpecialDropAction::FolderDetach
            || !Target.TargetItem.IsFolder())
        {
            return false;
        }

        const FFolder ParentFolder = Target.TargetItem.Folder;
        ULevel* Level = ParentFolder.GetRootObjectAssociatedLevel();
        UWorld* World = Level ? Level->GetWorld() : nullptr;
        if (!World)
        {
            return false;
        }

        TArray<FIndexSequenceItem> OrderedMoved = Target.MovedItems;
        FIndexSequenceState::SortItemsForMove(OrderedMoved);

        const FScopedTransaction Transaction(
            FText::FromString(TEXT("Index: Move Folders Out of Parent")));

        struct FFolderDetachGuard
        {
            FFolderDetachGuard() { bApplyingFolderDetach = true; }
            ~FFolderDetachGuard() { bApplyingFolderDetach = false; }
        } Guard;

        TArray<FIndexSequenceItem> NewItems;
        for (const FIndexSequenceItem& Item : OrderedMoved)
        {
            if (!Item.IsFolder() || Item.Folder.GetParent() != ParentFolder)
            {
                continue;
            }

            const FName NewPath = BuildDetachedFolderPath(Item.Folder, ParentFolder);
            const FFolder RequestedFolder(ParentFolder.GetRootObject(), NewPath);
            if (!FActorFolders::Get().RenameFolderInWorld(*World, Item.Folder, RequestedFolder))
            {
                continue;
            }

            FFolder ResolvedFolder;
            if (ResolveFolderByPath(*World, ParentFolder.GetRootObject(), NewPath, ResolvedFolder))
            {
                NewItems.Add(FIndexSequenceItem::FromFolder(ResolvedFolder));
            }
        }

        if (NewItems.IsEmpty())
        {
            return false;
        }

        const bool bOrdered = FIndexSequenceState::InsertItemsRelativeInCurrentTransaction(
            NewItems,
            Target.TargetItem,
            false);
        RefreshAllOutliners();
        return bOrdered || !NewItems.IsEmpty();
    }

    bool IntegrateNewActorsAtTop(const TArray<AActor*>& Actors)
    {
        TArray<FIndexSequenceItem> Items;
        for (AActor* Actor : Actors)
        {
            if (IsValid(Actor) && Actor->GetWorld() && Actor->GetWorld()->WorldType == EWorldType::Editor)
            {
                Items.Add(FIndexSequenceItem::FromActor(Actor));
            }
        }
        FIndexSequenceState::CollapseToTopLevelRoots(Items);
        const bool bChanged = FIndexSequenceState::IntegrateItemsAtTop(Items);
        if (bChanged)
        {
            RefreshAllOutliners();
        }
        return bChanged;
    }

    static FNativeDropPreview FindNativeDropPreview(
        FSlateApplication& App,
        const FPointerEvent& Event,
        const TSharedPtr<FDragDropOperation>& Operation)
    {
        FNativeDropPreview Result;
        if (!UIndexSettings::Get()->bEnableCustomOrdering || !Operation)
        {
            return Result;
        }

        UnregisterDeadOutliners();
        const FWidgetPath Path = App.LocateWindowUnderMouse(
            Event.GetScreenSpacePosition(),
            App.GetInteractiveTopLevelWindows(),
            false,
            Event.GetUserIndex());

        for (const TWeakPtr<ISceneOutliner>& WeakOutliner : Outliners)
        {
            const TSharedPtr<ISceneOutliner> Outliner = WeakOutliner.Pin();
            if (!Outliner || !PathContains(Path, Outliner) || !Outliner->GetMode())
            {
                continue;
            }

            TSharedPtr<STableRow<FSceneOutlinerTreeItemPtr>> Row;
            for (int32 Index = Path.Widgets.Num() - 1; Index >= 0; --Index)
            {
                const TSharedRef<SWidget> Widget = Path.Widgets[Index].Widget;
                if (Widget->GetType() == FName(TEXT("SSceneOutlinerTreeRow")))
                {
                    Row = StaticCastSharedRef<STableRow<FSceneOutlinerTreeItemPtr>>(Widget);
                    break;
                }
            }
            if (!Row || ResolveZone(Row->GetCachedGeometry(), Event) != EDropZone::Native)
            {
                continue;
            }

            const FSceneOutlinerTreeItemPtr* ItemPtr = Outliner->GetTree().ItemFromWidget(Row.Get());
            if (!ItemPtr || !*ItemPtr)
            {
                continue;
            }

            FSceneOutlinerDragDropPayload Payload(*Operation);
            if (!Outliner->GetMode()->ParseDragDrop(Payload, *Operation))
            {
                continue;
            }
            const FSceneOutlinerDragValidationInfo Validation =
                Outliner->GetMode()->ValidateDrop(**ItemPtr, Payload);

            TArray<FIndexSequenceItem> MovedItems;
            BuildSequencePayload(*Outliner, Operation, MovedItems);

            FString Message;
            if (FActorTreeItem* ActorItem = (*ItemPtr)->CastTo<FActorTreeItem>();
                ActorItem && ActorItem->IsValid())
            {
                const FString TargetName = ActorItem->Actor.IsValid()
                    ? ActorItem->Actor->GetActorLabel()
                    : FString(TEXT("actor"));
                if (Validation.CompatibilityType == ESceneOutlinerDropCompatibility::CompatibleAttach
                    || Validation.CompatibilityType == ESceneOutlinerDropCompatibility::CompatibleMultipleAttach)
                {
                    if (MovedItems.Num() == 1)
                    {
                        Message = FString::Printf(
                            TEXT("Parent %s to %s"),
                            *GetItemDisplayText(MovedItems[0]).ToString(),
                            *TargetName);
                    }
                    else
                    {
                        Message = FString::Printf(TEXT("Parent %d items to %s"), MovedItems.Num(), *TargetName);
                    }
                }
                else if (Validation.CompatibilityType == ESceneOutlinerDropCompatibility::CompatibleDetach
                    || Validation.CompatibilityType == ESceneOutlinerDropCompatibility::CompatibleMultipleDetach)
                {
                    Message = MovedItems.Num() == 1
                        ? FString::Printf(TEXT("Detach from %s"), *TargetName)
                        : FString::Printf(TEXT("Detach %d actors from %s"), MovedItems.Num(), *TargetName);
                }
            }
            else if (FActorFolderTreeItem* FolderItem = (*ItemPtr)->CastTo<FActorFolderTreeItem>();
                FolderItem && FolderItem->IsValid())
            {
                const FString TargetName = FolderItem->GetFolder().GetLeafName().ToString();
                if (Validation.IsValid())
                {
                    if (MovedItems.Num() == 1)
                    {
                        Message = FString::Printf(
                            TEXT("Move %s into %s"),
                            *GetItemDisplayText(MovedItems[0]).ToString(),
                            *TargetName);
                    }
                    else
                    {
                        Message = FString::Printf(TEXT("Move %d items into %s"), MovedItems.Num(), *TargetName);
                    }
                }
            }

            if (Message.IsEmpty() && !Validation.ValidationText.IsEmpty())
            {
                Message = Validation.ValidationText.ToString();
            }
            if (Message.IsEmpty())
            {
                continue;
            }

            Result.Row = Row;
            Result.Message = FText::FromString(Message);
            Result.bValid = Validation.IsValid();
            return Result;
        }

        return Result;
    }

    static FIndexSequenceItem TreeItemToSequenceItem(const FSceneOutlinerTreeItemPtr& TreeItem)
    {
        if (!TreeItem.IsValid())
        {
            return FIndexSequenceItem();
        }

        if (FActorTreeItem* ActorItem = TreeItem->CastTo<FActorTreeItem>();
            ActorItem && ActorItem->IsValid())
        {
            return FIndexSequenceItem::FromActor(ActorItem->Actor.Get());
        }

        if (FActorFolderTreeItem* FolderItem = TreeItem->CastTo<FActorFolderTreeItem>();
            FolderItem && FolderItem->IsValid())
        {
            return FIndexSequenceItem::FromFolder(FolderItem->GetFolder());
        }

        return FIndexSequenceItem();
    }

    static FIndexSequenceItem GetFirstVisibleSequenceChild(const FSceneOutlinerTreeItemPtr& ParentItem)
    {
        if (!ParentItem.IsValid() || !ParentItem->Flags.bIsExpanded)
        {
            return FIndexSequenceItem();
        }

        TArray<FIndexSequenceItem> Children;
        for (const TWeakPtr<ISceneOutlinerTreeItem>& WeakChild : ParentItem->GetChildren())
        {
            const FSceneOutlinerTreeItemPtr Child = WeakChild.Pin();
            if (!Child.IsValid() || Child->Flags.bIsFilteredOut)
            {
                continue;
            }

            const FIndexSequenceItem SequenceChild = TreeItemToSequenceItem(Child);
            if (SequenceChild.IsValid())
            {
                Children.Add(SequenceChild);
            }
        }

        if (Children.IsEmpty())
        {
            return FIndexSequenceItem();
        }

        FIndexSequenceState::SortItems(Children);
        return Children[0];
    }

    static FDropTarget FindDropTarget(
        FSlateApplication& App,
        const FPointerEvent& Event,
        const TSharedPtr<FDragDropOperation>& Operation)
    {
        FDropTarget Result;
        if (!UIndexSettings::Get()->bEnableCustomOrdering || !Operation)
        {
            return Result;
        }

        UnregisterDeadOutliners();

        const FWidgetPath Path = App.LocateWindowUnderMouse(
            Event.GetScreenSpacePosition(),
            App.GetInteractiveTopLevelWindows(),
            false,
            Event.GetUserIndex());

        for (const TWeakPtr<ISceneOutliner>& WeakOutliner : Outliners)
        {
            const TSharedPtr<ISceneOutliner> Outliner = WeakOutliner.Pin();
            if (!Outliner || !PathContains(Path, Outliner))
            {
                continue;
            }

            TSharedPtr<STableRow<FSceneOutlinerTreeItemPtr>> Row;
            for (int32 Index = Path.Widgets.Num() - 1; Index >= 0; --Index)
            {
                const TSharedRef<SWidget> Widget = Path.Widgets[Index].Widget;
                if (Widget->GetType() == FName(TEXT("SSceneOutlinerTreeRow")))
                {
                    Row = StaticCastSharedRef<STableRow<FSceneOutlinerTreeItemPtr>>(Widget);
                    break;
                }
            }

            if (!Row)
            {
                continue;
            }

            const FSceneOutlinerTreeItemPtr* ItemPtr = Outliner->GetTree().ItemFromWidget(Row.Get());
            if (!ItemPtr || !*ItemPtr)
            {
                continue;
            }

            // Edge zones always belong to Index reordering. Native attach/detach
            // validation is intentionally not consulted here; structural gestures are
            // resolved separately and only for the center zone. This keeps the same
            // Before/After affordance available on parents, grandparents and sibling
            // tiers instead of allowing native parenting semantics to steal the edge.

            const EDropZone Zone = ResolveZone(Row->GetCachedGeometry(), Event);
            if (Zone == EDropZone::Native)
            {
                continue;
            }

            FIndexSequenceItem TargetItem = TreeItemToSequenceItem(*ItemPtr);
            if (!TargetItem.IsValid())
            {
                continue;
            }

            // A visual gap immediately below an expanded parent is the same physical
            // insertion slot as the top edge of its first visible child. Resolve both
            // sides of that gap to the child's sibling tier so dragging between a
            // folder and its first child moves the item *into* that folder rather than
            // beside the folder at its parent's tier.
            bool bInsertBefore = Zone == EDropZone::Before;
            if (Zone == EDropZone::After)
            {
                const FIndexSequenceItem FirstChild = GetFirstVisibleSequenceChild(*ItemPtr);
                if (FirstChild.IsValid())
                {
                    TargetItem = FirstChild;
                    bInsertBefore = true;
                }
            }

            TArray<FIndexSequenceItem> MovedItems;
            if (!BuildSequencePayload(*Outliner, Operation, MovedItems)
                || !FIndexSequenceState::CanInsertItemsRelative(MovedItems, TargetItem))
            {
                continue;
            }

            Result.Outliner = Outliner;
            Result.Row = Row;
            Result.Item = *ItemPtr;
            Result.VisualZone = Zone;
            Result.bInsertBefore = bInsertBefore;
            Result.MovedItems = MoveTemp(MovedItems);
            Result.TargetItem = TargetItem;
            return Result;
        }

        return Result;
    }

    // Actor folder moves are intentionally not mirrored by an Index-specific
    // refresh callback. The native Scene Outliner owns the hierarchy mutation and
    // refresh. When its sibling list is sorted, IndexSequenceState::EnsureCaptured
    // reconciles any row whose stable ID now belongs to a different local tier.
    // This keeps generic folder moves and Create Folder containing selection from
    // scheduling a second refresh that can destroy Unreal's inline rename widget.

    void HandleNativeHierarchyChanged(AActor* Actor)
    {
        if (!IsValid(Actor) || bApplyingActorFolderDetach)
        {
            return;
        }
        if (FIndexSequenceState::IsApplyingExplicitReorder() || GIsTransacting || PendingNativeMove.bActive)
        {
            return;
        }

        // The native command's hierarchy transaction can close before the Scene
        // Outliner's deferred sort asks Index to persist the resulting order. Anchor
        // the current Index metadata inside that native transaction so Undo/Redo
        // always starts reconciliation from the exact pre-command sequence.
        FIndexSequenceState::CaptureNativeHierarchyUndoBaseline(Actor->GetLevel());

        // Do not eagerly remove/reinsert an actor when Unreal reports a native
        // hierarchy change. Create Folder with Selection can report several actors
        // one at a time; inserting each callback at index zero reverses otherwise
        // ordered selections (A,B becomes B,A). The native Outliner already refreshes
        // its changed tiers. FIndexColumn::SortItems() calls EnsureCaptured(), which
        // reconciles every arrived actor against one immutable pre-move hierarchy
        // baseline and preserves the original visual traversal order.
        (void)Actor;
    }

    void HandleFolderMoved(UWorld& World, const FFolder& OldFolder, const FFolder& NewFolder)
    {
        (void)World;
        if (bApplyingFolderDetach
            || FIndexSequenceState::IsApplyingExplicitReorder()
            || GIsTransacting
            || PendingNativeMove.bActive)
        {
            return;
        }
        if (!OldFolder.IsValid() || !NewFolder.IsValid())
        {
            return;
        }

        const bool bRenameOnly =
            FIndexSequenceState::GetGroupKey(FIndexSequenceItem::FromFolder(OldFolder))
                == FIndexSequenceState::GetGroupKey(FIndexSequenceItem::FromFolder(NewFolder))
            && OldFolder.GetLeafName() != NewFolder.GetLeafName();

        FIndexSequenceState::CaptureNativeHierarchyUndoBaseline(NewFolder.GetRootObjectAssociatedLevel());
        FIndexSequenceState::HandleFolderMoved(OldFolder, NewFolder);
        ScheduleFolderRefresh();
        if (bRenameOnly)
        {
            FlashRenameItem(FIndexSequenceItem::FromFolder(NewFolder));
        }
    }

    void HandleActorDeleted(AActor* Actor)
    {
        if (!Actor)
        {
            return;
        }

        FIndexSequenceState::RemoveItem(FIndexSequenceItem::FromActor(Actor));

        // Removing a row simply removes its stable ID from the level hierarchy
        // snapshot. The remaining child array is already contiguous and retains its
        // exact relative order. Clear transient drag state that could still reference
        // the deleted actor and refresh the registered Outliners.
        bool bPendingContainsActor = false;
        for (const FIndexSequenceItem& PendingItem : PendingNativeMove.Snapshot.OrderedItems)
        {
            if (PendingItem.IsActor() && PendingItem.Actor.Get() == Actor)
            {
                bPendingContainsActor = true;
                break;
            }
        }
        if (bPendingContainsActor)
        {
            PendingNativeMove.Reset();
        }

        RefreshAllOutliners();
    }

    void ResetTransientState()
    {
        bFolderRefreshScheduled = false;
        PendingNativeMove.Reset();

        for (const FTSTicker::FDelegateHandle& Handle : PendingNativeTickers)
        {
            FTSTicker::GetCoreTicker().RemoveTicker(Handle);
        }
        PendingNativeTickers.Reset();
    }

    static bool FinalizePendingNativeMove(float)
    {
        if (!PendingNativeMove.bActive)
        {
            return false;
        }

        TArray<FIndexSequenceItem> ChangedItems;
        for (int32 Index = 0; Index < PendingNativeMove.Snapshot.OrderedItems.Num(); ++Index)
        {
            const FIndexSequenceItem Current =
                FIndexSequenceState::ResolveCurrentItem(PendingNativeMove.Snapshot.OrderedItems[Index]);
            if (!Current.IsValid())
            {
                continue;
            }
            if (FIndexSequenceState::GetGroupKey(Current) != PendingNativeMove.Snapshot.SourceGroups[Index])
            {
                ChangedItems.Add(Current);
            }
        }

        bool bApplied = false;
        if (!ChangedItems.IsEmpty())
        {
            FIndexSequenceState::SortItemsForMove(ChangedItems);
            if (PendingNativeMove.bPlaceAfterTarget && PendingNativeMove.PlacementTarget.IsValid())
            {
                bApplied = FIndexSequenceState::InsertItemsRelative(
                    ChangedItems,
                    PendingNativeMove.PlacementTarget,
                    false);
            }
            else
            {
                bApplied = FIndexSequenceState::IntegrateItemsAtTop(ChangedItems);
            }
        }

        PendingNativeMove.Reset();
        if (bApplied)
        {
            RefreshAllOutliners();
        }
        return false;
    }

    class FIndexInputProcessor final : public IInputProcessor
    {
    public:
        virtual ~FIndexInputProcessor() override
        {
            ClearVisualState();
            NativeDragSnapshot.Reset();
            Overlay.Shutdown();
        }

        virtual bool HandleKeyDownEvent(
            FSlateApplication& SlateApplication,
            const FKeyEvent& KeyEvent) override
        {
            // Match the user-facing Index rule: duplicating a folder duplicates the
            // complete hierarchy beneath it. Epic exposes this as a distinct Outliner
            // command (DuplicateFoldersHierarchy), while normal Ctrl+D can duplicate
            // only the folder shell. Intercept Ctrl+D only when an Outliner selection
            // contains a folder and route it through the hierarchy-aware command.
            if (KeyEvent.GetKey() != EKeys::D
                || !KeyEvent.IsControlDown()
                || KeyEvent.IsAltDown()
                || KeyEvent.IsShiftDown())
            {
                return false;
            }

            if (const TSharedPtr<SWidget> Focused = SlateApplication.GetKeyboardFocusedWidget())
            {
                if (Focused->GetType().ToString().Contains(TEXT("EditableText")))
                {
                    return false;
                }
            }

            UnregisterDeadOutliners();
            for (const TWeakPtr<ISceneOutliner>& WeakOutliner : Outliners)
            {
                const TSharedPtr<ISceneOutliner> Outliner = WeakOutliner.Pin();
                if (!Outliner)
                {
                    continue;
                }

                bool bHasSelectedFolder = false;
                for (const FSceneOutlinerTreeItemPtr& SelectedItem : Outliner->GetTree().GetSelectedItems())
                {
                    if (SelectedItem.IsValid()
                        && SelectedItem->CastTo<FActorFolderTreeItem>() != nullptr)
                    {
                        bHasSelectedFolder = true;
                        break;
                    }
                }
                if (!bHasSelectedFolder)
                {
                    continue;
                }

                const TSharedPtr<SSceneOutliner> SceneOutliner = StaticCastSharedPtr<SSceneOutliner>(Outliner);
                if (SceneOutliner)
                {
                    SceneOutliner->DuplicateFoldersHierarchy();
                    return true;
                }
            }

            return false;
        }

        virtual bool HandleMouseMoveEvent(
            FSlateApplication& SlateApplication,
            const FPointerEvent& MouseEvent) override
        {
            const TSharedPtr<FDragDropOperation> Operation = SlateApplication.GetDragDroppingContent();
            if (!Operation)
            {
                ClearVisualState();
                NativeDragSnapshot.Reset();
                return false;
            }

            CaptureNativeDragSnapshot(SlateApplication, MouseEvent, Operation, NativeDragSnapshot);

            // Structural actions keep Unreal's native row presentation. Index
            // supplies the resolved drag tooltip, while hierarchy-specific row
            // highlighting belongs to Origin.
            const FSpecialDropTarget SpecialTarget =
                FindSpecialDropTarget(SlateApplication, MouseEvent, Operation);
            if (SpecialTarget.IsValid())
            {
                CachedTarget.Reset();
                CachedRootTarget.Reset();
                CachedNativePreview.Reset();
                CachedSpecialTarget = SpecialTarget;
                Overlay.HideLine();

                ApplyCurrentToolTip(Operation);
                return false;
            }

            CachedSpecialTarget.Reset();
            const FDropTarget Target = FindDropTarget(SlateApplication, MouseEvent, Operation);
            if (Target.IsCustomReorder() && Target.Row)
            {
                CachedNativePreview.Reset();
                CachedRootTarget.Reset();
                CachedTarget = Target;
                const bool bAboveVisual = Target.VisualZone == EDropZone::Before;
                Overlay.ShowLine(
                    Target.Row->GetCachedGeometry(),
                    Target.Outliner->GetTree().GetCachedGeometry(),
                    bAboveVisual,
                    GetInsertionLineInset(Target.TargetItem),
                    bAboveVisual
                        ? UIndexSettings::Get()->AboveLineColor
                        : UIndexSettings::Get()->BelowLineColor);
                ApplyCurrentToolTip(Operation);
            }
            else
            {
                CachedTarget.Reset();
                Overlay.HideLine();

                CachedRootTarget = FindRootDropTarget(SlateApplication, MouseEvent, Operation);
                if (CachedRootTarget.IsValid())
                {
                    CachedNativePreview.Reset();
                    Overlay.ShowLine(
                        CachedRootTarget.TreeGeometry,
                        CachedRootTarget.TreeGeometry,
                        false,
                        0.0f,
                        UIndexSettings::Get()->BelowLineColor);
                    ApplyCurrentToolTip(Operation);
                    return false;
                }

                CachedRootTarget.Reset();
                CachedNativePreview = FindNativeDropPreview(
                    SlateApplication, MouseEvent, Operation);
                if (CachedNativePreview.IsSet())
                {
                    ApplyCurrentToolTip(Operation);
                }
                else
                {
                    ResetOperationToolTip(Operation);
                }
            }

            return false;
        }

        virtual bool HandleMouseButtonUpEvent(
            FSlateApplication& SlateApplication,
            const FPointerEvent& MouseEvent) override
        {
            if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
            {
                return false;
            }

            const TSharedPtr<FDragDropOperation> Operation = SlateApplication.GetDragDroppingContent();

            if (CachedSpecialTarget.IsValid())
            {
                const FSpecialDropTarget SpecialTarget = CachedSpecialTarget;
                CachedSpecialTarget.Reset();
                CachedTarget.Reset();
                CachedRootTarget.Reset();
                Overlay.Hide();
                ResetOperationToolTip(Operation);
        
                CachedNativePreview.Reset();

                if (SpecialTarget.Action == ESpecialDropAction::ParentToActor
                    || SpecialTarget.Action == ESpecialDropAction::MoveIntoFolder)
                {
                    NativeDragSnapshot.Reset();
                    PendingNativeMove.Reset();
                    if (Operation)
                    {
                        SlateApplication.CancelDragDrop();
                    }
                    ApplyParentToTarget(SpecialTarget);
                    return true;
                }

                if (SpecialTarget.Action == ESpecialDropAction::ActorDetachToFolder)
                {
                    NativeDragSnapshot.Reset();
                    PendingNativeMove.Reset();
                    if (Operation)
                    {
                        SlateApplication.CancelDragDrop();
                    }
                    ApplyActorDetachToFolder(SpecialTarget);
                    return true;
                }

                if (SpecialTarget.Action == ESpecialDropAction::ActorMoveOutOfFolder)
                {
                    NativeDragSnapshot.Reset();
                    PendingNativeMove.Reset();
                    if (Operation)
                    {
                        SlateApplication.CancelDragDrop();
                    }
                    ApplyActorMoveOutOfFolder(SpecialTarget);
                    return true;
                }

                if (SpecialTarget.Action == ESpecialDropAction::FolderDetach)
                {
                    NativeDragSnapshot.Reset();
                    PendingNativeMove.Reset();
                    if (Operation)
                    {
                        SlateApplication.CancelDragDrop();
                    }
                    ApplyFolderDetach(SpecialTarget);
                    return true;
                }

                if (SpecialTarget.Action == ESpecialDropAction::ActorDetach)
                {
                    NativeDragSnapshot.Reset();
                    PendingNativeMove.Reset();
                    if (Operation)
                    {
                        SlateApplication.CancelDragDrop();
                    }
                    ApplyActorDetach(SpecialTarget);
                    return true;
                }
            }

            if (CachedRootTarget.IsValid())
            {
                const FRootDropTarget RootTarget = CachedRootTarget;
                CachedRootTarget.Reset();
                CachedTarget.Reset();
                CachedSpecialTarget.Reset();
                CachedNativePreview.Reset();
                NativeDragSnapshot.Reset();
                PendingNativeMove.Reset();
                Overlay.Hide();
                ResetOperationToolTip(Operation);

                if (Operation)
                {
                    SlateApplication.CancelDragDrop();
                }

                const bool bApplied = FIndexSequenceState::MoveItemsToRootAtBottom(
                    RootTarget.MovedItems,
                    RootTarget.Level.Get(),
                    RootTarget.RootObject);
                if (bApplied)
                {
                    RefreshAllOutliners();
                }
                return true;
            }

            if (CachedTarget.IsCustomReorder())
            {
                FDropTarget Target = CachedTarget;
                CachedTarget.Reset();
                CachedNativePreview.Reset();
                NativeDragSnapshot.Reset();
                Overlay.Hide();
                ResetOperationToolTip(Operation);
                PendingNativeMove.Reset();
        
                // Consume the release so the stock actor Outliner cannot also treat
                // this exact edge drop as a native attach/reparent operation.
                if (Operation)
                {
                    SlateApplication.CancelDragDrop();
                }

                const bool bApplied = FIndexSequenceState::InsertItemsRelative(
                    Target.MovedItems,
                    Target.TargetItem,
                    Target.bInsertBefore);

                if (bApplied)
                {
                    RefreshAllOutliners();
                }

                return true;
            }

            CachedTarget.Reset();
            CachedSpecialTarget.Reset();
            CachedRootTarget.Reset();
            CachedNativePreview.Reset();
            Overlay.Hide();
            ResetOperationToolTip(Operation);

            // For a center/native drop, preserve a snapshot of the actors before
            // Unreal changes their folder/attachment. Engine hierarchy delegates
            // will assign their fresh destination orders synchronously inside the
            // native transaction. A next-tick pass finalizes the recent top block.
            if (NativeDragSnapshot.IsValid())
            {
                PendingNativeMove.Reset();
                PendingNativeMove.Snapshot = NativeDragSnapshot;
                PendingNativeMove.bActive = true;
                NativeDragSnapshot.Reset();

                PendingNativeTickers.Add(FTSTicker::GetCoreTicker().AddTicker(
                    FTickerDelegate::CreateStatic(&FinalizePendingNativeMove)));
            }

            // Native center drops remain fully owned by Unreal.
            return false;
        }

        virtual void Tick(
            float,
            FSlateApplication& SlateApplication,
            TSharedRef<ICursor>) override
        {
            const TSharedPtr<FDragDropOperation> Operation = SlateApplication.GetDragDroppingContent();
            if (!Operation)
            {
                if (CachedTarget.IsCustomReorder() || CachedSpecialTarget.IsValid() || CachedRootTarget.IsValid() || CachedNativePreview.IsSet())
                {
                    ClearVisualState();
                }
                return;
            }

            // Stock Scene Outliner drag-over handling updates its decorator after
            // input preprocessors run. Re-apply Index's resolved action each frame
            // so the text the user sees matches the blue insertion preview.
            if (CachedTarget.IsCustomReorder() || CachedSpecialTarget.IsValid() || CachedRootTarget.IsValid() || CachedNativePreview.IsSet())
            {
                ApplyCurrentToolTip(Operation);
            }
        }

    private:
        void ApplyCurrentToolTip(const TSharedPtr<FDragDropOperation>& Operation) const
        {
            if (CachedSpecialTarget.IsValid())
            {
                const FText TargetName = GetItemDisplayText(CachedSpecialTarget.TargetItem);
                const int32 Count = CachedSpecialTarget.MovedItems.Num();
                FString Message;
                if (CachedSpecialTarget.Action == ESpecialDropAction::ParentToActor)
                {
                    Message = Count == 1
                        ? FString::Printf(TEXT("Parent %s to %s"), *GetItemDisplayText(CachedSpecialTarget.MovedItems[0]).ToString(), *TargetName.ToString())
                        : FString::Printf(TEXT("Parent %d items to %s"), Count, *TargetName.ToString());
                }
                else if (CachedSpecialTarget.Action == ESpecialDropAction::MoveIntoFolder)
                {
                    Message = Count == 1
                        ? FString::Printf(TEXT("Move %s into %s"), *GetItemDisplayText(CachedSpecialTarget.MovedItems[0]).ToString(), *TargetName.ToString())
                        : FString::Printf(TEXT("Move %d items into %s"), Count, *TargetName.ToString());
                }
                else if (CachedSpecialTarget.Action == ESpecialDropAction::ActorDetach)
                {
                    Message = Count == 1
                        ? FString::Printf(TEXT("Detach from %s and place after it"), *TargetName.ToString())
                        : FString::Printf(TEXT("Detach %d actors from %s and place after it"), Count, *TargetName.ToString());
                }
                else if (CachedSpecialTarget.Action == ESpecialDropAction::ActorDetachToFolder)
                {
                    Message = Count == 1
                        ? FString::Printf(TEXT("Detach actor and move into %s"), *TargetName.ToString())
                        : FString::Printf(TEXT("Detach %d actors and move into %s"), Count, *TargetName.ToString());
                }
                else if (CachedSpecialTarget.Action == ESpecialDropAction::ActorMoveOutOfFolder)
                {
                    Message = Count == 1
                        ? FString::Printf(TEXT("Move actor out of %s and place after it"), *TargetName.ToString())
                        : FString::Printf(TEXT("Move %d actors out of %s and place after it"), Count, *TargetName.ToString());
                }
                else
                {
                    Message = Count == 1
                        ? FString::Printf(TEXT("Move folder out of %s and place after it"), *TargetName.ToString())
                        : FString::Printf(TEXT("Move %d folders out of %s and place after it"), Count, *TargetName.ToString());
                }
                SetOperationToolTip(Operation, FText::FromString(Message));
                return;
            }

            if (CachedTarget.IsCustomReorder())
            {
                const FText TargetName = GetItemDisplayText(CachedTarget.TargetItem);
                const TCHAR* Relation = CachedTarget.bInsertBefore ? TEXT("before") : TEXT("after");
                const int32 Count = CachedTarget.MovedItems.Num();
                FString Message;
                if (Count == 1)
                {
                    const FText SourceName = GetItemDisplayText(CachedTarget.MovedItems[0]);
                    Message = FString::Printf(
                        TEXT("Reorder %s %s %s"),
                        *SourceName.ToString(),
                        Relation,
                        *TargetName.ToString());
                }
                else
                {
                    Message = FString::Printf(
                        TEXT("Reorder %d items %s %s"),
                        Count,
                        Relation,
                        *TargetName.ToString());
                }
                SetOperationToolTip(Operation, FText::FromString(Message));
                return;
            }

            if (CachedRootTarget.IsValid())
            {
                const int32 Count = CachedRootTarget.MovedItems.Num();
                const FString Message = Count == 1
                    ? FString::Printf(TEXT("Move %s to root"), *GetItemDisplayText(CachedRootTarget.MovedItems[0]).ToString())
                    : FString::Printf(TEXT("Move %d items to root"), Count);
                SetOperationToolTip(Operation, FText::FromString(Message));
                return;
            }

            if (CachedNativePreview.IsSet())
            {
                SetOperationToolTip(Operation, CachedNativePreview.Message, CachedNativePreview.bValid);
            }
        }

        void ClearVisualState()
        {
            CachedTarget.Reset();
            CachedSpecialTarget.Reset();
            CachedNativePreview.Reset();
            Overlay.Hide();
        }

        FDropTarget CachedTarget;
        FSpecialDropTarget CachedSpecialTarget;
        FRootDropTarget CachedRootTarget;
        FNativeDropPreview CachedNativePreview;
        FNativeDragSnapshot NativeDragSnapshot;
        FIndexLineOverlay Overlay;
    };

    TSharedRef<IInputProcessor> CreateInputProcessor()
    {
        return MakeShared<FIndexInputProcessor>();
    }

    void Shutdown()
    {
        for (const FTSTicker::FDelegateHandle& Handle : PendingSortTickers)
        {
            FTSTicker::GetCoreTicker().RemoveTicker(Handle);
        }
        PendingSortTickers.Reset();

        ResetTransientState();
        Outliners.Reset();
    }
}
