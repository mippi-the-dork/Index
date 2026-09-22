#include "IndexColumn.h"

#include "ActorTreeItem.h"
#include "ActorFolderTreeItem.h"
#include "IndexInteraction.h"
#include "IndexSettings.h"
#include "IndexSequenceState.h"
#include "ISceneOutliner.h"
#include "Widgets/SNullWidget.h"

FName FIndexColumn::GetID()
{
    static const FName ColumnID(TEXT("IndexOrderColumn"));
    return ColumnID;
}

FIndexColumn::FIndexColumn(ISceneOutliner& InSceneOutliner)
    : SceneOutliner(StaticCastSharedRef<ISceneOutliner>(InSceneOutliner.AsShared()))
{
    IndexInteraction::RegisterOutliner(InSceneOutliner);
}

FName FIndexColumn::GetColumnID()
{
    return GetID();
}

SHeaderRow::FColumn::FArguments FIndexColumn::ConstructHeaderRowColumn()
{
    // Keep the Index column visually hidden. It exists only to supply Index's
    // manual sort order; visual row sizing belongs to readability-focused plugins.
    return SHeaderRow::Column(GetID())
        .DefaultLabel(FText::GetEmpty())
        .DefaultTooltip(NSLOCTEXT("Index", "IndexColumnTooltip", "Index manual actor and folder sequence"))
        .InitialSortMode(EColumnSortMode::Ascending)
        .ManualWidth(1.0f)
        .Visibility(EVisibility::Hidden);
}

const TSharedRef<SWidget> FIndexColumn::ConstructRowWidget(
    FSceneOutlinerTreeItemRef Item,
    const STableRow<FSceneOutlinerTreeItemPtr>& Row)
{
    // Index does not contribute visual content or desired row size.
    return SNullWidget::NullWidget;
}

void FIndexColumn::SortItems(
    TArray<FSceneOutlinerTreeItemPtr>& OutItems,
    EColumnSortMode::Type SortMode) const
{
    const TSharedPtr<ISceneOutliner> Outliner = SceneOutliner.Pin();

    auto ToSequenceItem = [](const FSceneOutlinerTreeItemPtr& TreeItem) -> FIndexSequenceItem
    {
        if (!TreeItem.IsValid())
        {
            return FIndexSequenceItem();
        }
        if (const FActorTreeItem* ActorItem = TreeItem->CastTo<FActorTreeItem>();
            ActorItem && ActorItem->IsValid())
        {
            return FIndexSequenceItem::FromActor(ActorItem->Actor.Get());
        }
        if (const FActorFolderTreeItem* FolderItem = TreeItem->CastTo<FActorFolderTreeItem>();
            FolderItem && FolderItem->IsValid())
        {
            return FIndexSequenceItem::FromFolder(FolderItem->GetFolder());
        }
        return FIndexSequenceItem();
    };

    // SortItems is called with one Outliner sibling list at a time. Before Index
    // changes it, capture that exact current order as the initial canonical order.
    TArray<FIndexSequenceItem> CurrentSequence;
    for (const FSceneOutlinerTreeItemPtr& TreeItem : OutItems)
    {
        const FIndexSequenceItem SequenceItem = ToSequenceItem(TreeItem);
        if (SequenceItem.IsValid())
        {
            CurrentSequence.Add(SequenceItem);
        }
    }
    FIndexSequenceState::EnsureCaptured(CurrentSequence);

    OutItems.Sort([SortMode, Outliner, ToSequenceItem](const FSceneOutlinerTreeItemPtr& A, const FSceneOutlinerTreeItemPtr& B)
    {
        if (A == B)
        {
            return false;
        }
        if (!A.IsValid())
        {
            return false;
        }
        if (!B.IsValid())
        {
            return true;
        }

        const FIndexSequenceItem SequenceA = ToSequenceItem(A);
        const FIndexSequenceItem SequenceB = ToSequenceItem(B);
        if (SequenceA.IsValid() && SequenceB.IsValid())
        {
            if (SequenceA == SequenceB)
            {
                return false;
            }
            const bool bALessB = FIndexSequenceState::Less(SequenceA, SequenceB);
            const bool bBLessA = FIndexSequenceState::Less(SequenceB, SequenceA);
            if (bALessB == bBLessA)
            {
                return false;
            }
            return SortMode == EColumnSortMode::Descending ? bBLessA : bALessB;
        }

        if (Outliner)
        {
            const uint32 PriorityA = Outliner->GetTypeSortPriority(*A);
            const uint32 PriorityB = Outliner->GetTypeSortPriority(*B);
            if (PriorityA != PriorityB)
            {
                return PriorityA < PriorityB;
            }
        }

        const int32 CompareResult = A->GetDisplayString().Compare(B->GetDisplayString(), ESearchCase::IgnoreCase);
        if (CompareResult == 0)
        {
            return false;
        }
        return SortMode == EColumnSortMode::Descending ? CompareResult > 0 : CompareResult < 0;
    });
}
