#pragma once

#include "CoreMinimal.h"
#include "ISceneOutlinerColumn.h"

class FIndexColumn : public ISceneOutlinerColumn
{
public:
    explicit FIndexColumn(ISceneOutliner& InSceneOutliner);
    virtual ~FIndexColumn() override = default;

    static FName GetID();

    virtual FName GetColumnID() override;
    virtual SHeaderRow::FColumn::FArguments ConstructHeaderRowColumn() override;
    virtual const TSharedRef<SWidget> ConstructRowWidget(
        FSceneOutlinerTreeItemRef Item,
        const STableRow<FSceneOutlinerTreeItemPtr>& Row) override;
    virtual bool SupportsSorting() const override { return true; }
    virtual void SortItems(
        TArray<FSceneOutlinerTreeItemPtr>& OutItems,
        EColumnSortMode::Type SortMode) const override;

private:
    TWeakPtr<ISceneOutliner> SceneOutliner;
};
