#pragma once

#include "CoreMinimal.h"
#include "Folder.h"

class AActor;
class ULevel;

enum class EIndexSequenceItemType : uint8
{
    Actor,
    Folder
};

struct FIndexSequenceGroupKey
{
    TWeakObjectPtr<ULevel> Level;
    TWeakObjectPtr<AActor> ParentActor;
    FFolder::FRootObject RootObject = FFolder::GetInvalidRootObject();
    FName FolderPath = NAME_None;
    FGuid ParentFolderGuid;
    bool bAttachedActorTier = false;

    bool IsValid() const { return Level.IsValid(); }

    bool operator==(const FIndexSequenceGroupKey& Other) const
    {
        if (Level != Other.Level || bAttachedActorTier != Other.bAttachedActorTier)
        {
            return false;
        }

        if (bAttachedActorTier)
        {
            return ParentActor == Other.ParentActor;
        }

        if (ParentFolderGuid.IsValid() || Other.ParentFolderGuid.IsValid())
        {
            return RootObject == Other.RootObject && ParentFolderGuid == Other.ParentFolderGuid;
        }

        return RootObject == Other.RootObject && FolderPath == Other.FolderPath;
    }

    bool operator!=(const FIndexSequenceGroupKey& Other) const
    {
        return !(*this == Other);
    }
};

struct FIndexSequenceItem
{
    EIndexSequenceItemType Type = EIndexSequenceItemType::Actor;
    TWeakObjectPtr<AActor> Actor;
    FFolder Folder = FFolder::GetInvalidFolder();

    static FIndexSequenceItem FromActor(AActor* InActor);
    static FIndexSequenceItem FromFolder(const FFolder& InFolder);

    bool IsActor() const { return Type == EIndexSequenceItemType::Actor; }
    bool IsFolder() const { return Type == EIndexSequenceItemType::Folder; }
    bool IsValid() const;

    bool operator==(const FIndexSequenceItem& Other) const;
    bool operator!=(const FIndexSequenceItem& Other) const { return !(*this == Other); }
};

class FIndexSequenceState
{
public:
    static FIndexSequenceGroupKey GetGroupKey(const FIndexSequenceItem& Item);
    static TArray<FIndexSequenceItem> GetSiblings(const FIndexSequenceItem& Item);

    // Called from the Outliner sort before Index changes the order. The first time
    // a sibling tier is seen, its current Unreal order becomes the canonical order.
    static void EnsureCaptured(const TArray<FIndexSequenceItem>& ItemsInCurrentOrder);

    static bool HasOrder(const FIndexSequenceItem& Item);
    static int32 GetOrder(const FIndexSequenceItem& Item);
    static void SetOrder(const FIndexSequenceItem& Item, int32 NewOrder);

    static void SortItems(TArray<FIndexSequenceItem>& InOutItems);
    static void SortItemsForMove(TArray<FIndexSequenceItem>& InOutItems);
    static bool Less(const FIndexSequenceItem& A, const FIndexSequenceItem& B);

    // Remove descendants when one of their selected ancestors already carries them.
    static void CollapseToTopLevelRoots(TArray<FIndexSequenceItem>& InOutItems);

    static bool CanInsertItemsRelative(
        const TArray<FIndexSequenceItem>& MovedItems,
        const FIndexSequenceItem& TargetItem);

    static bool InsertItemsRelative(
        const TArray<FIndexSequenceItem>& MovedItems,
        const FIndexSequenceItem& TargetItem,
        bool bInsertBefore);

    static bool InsertItemsRelativeInCurrentTransaction(
        const TArray<FIndexSequenceItem>& MovedItems,
        const FIndexSequenceItem& TargetItem,
        bool bInsertBefore);

    // Structural drops use the same hierarchy snapshot but place the collapsed
    // highest selected roots at the top of the destination tier.
    static bool MoveItemsToParentAtTop(
        const TArray<FIndexSequenceItem>& MovedItems,
        const FIndexSequenceItem& ParentItem);

    // Dropping on empty Outliner space moves the selected root block to the level
    // root and appends it at the bottom without absorbing unrelated siblings.
    static bool MoveItemsToRootAtBottom(
        const TArray<FIndexSequenceItem>& MovedItems,
        ULevel* Level,
        const FFolder::FRootObject& RootObject);

    // New content always joins the top of its current local hierarchy tier.
    static bool IntegrateItemsAtTop(const TArray<FIndexSequenceItem>& Items);

    static bool RemoveItem(const FIndexSequenceItem& Item);

    // Native hierarchy commands can finish their Outliner reconciliation after
    // Unreal's scoped transaction has closed. Capture the current persisted Index
    // hierarchy inside the native transaction so Undo/Redo can restore a clean
    // ordering baseline before the post-transaction Outliner refresh runs.
    static void CaptureNativeHierarchyUndoBaseline(ULevel* Level);

    static bool HandleFolderMoved(const FFolder& OldFolder, const FFolder& NewFolder);
    static FIndexSequenceItem ResolveCurrentItem(const FIndexSequenceItem& Item);

    static bool IsApplyingExplicitReorder();

    // Called from FEditorDelegates::PostUndoRedo. Keeps the session-local native move
    // history alive while invalidating only the persisted-state cache, allowing the
    // following Outliner refresh to restore rows to historical sibling slots.
    static void PrepareForPostUndoRedo();
    static void InvalidateCache();
};
