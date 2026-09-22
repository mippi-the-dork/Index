#include "IndexSettings.h"

#include "IndexInteraction.h"

UIndexSettings::UIndexSettings()
    : bEnableCustomOrdering(true)
    , AboveLineColor(FLinearColor(0.0f, 0.65f, 1.0f, 1.0f))
    , BelowLineColor(FLinearColor(0.0f, 0.35f, 1.0f, 1.0f))
    , EdgeThresholdPercent(10.0f)
    , ReorderModifierKey(EIndexModifierKey::Alt)
{}

const UIndexSettings* UIndexSettings::Get()
{
    return GetDefault<UIndexSettings>();
}

#if WITH_EDITOR
void UIndexSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    IndexInteraction::RefreshAllOutliners();
}
#endif
