#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "IndexSettings.generated.h"

struct FPropertyChangedEvent;

UENUM(BlueprintType)
enum class EIndexModifierKey : uint8
{
    Alt UMETA(DisplayName = "Alt"),
    Shift UMETA(DisplayName = "Shift"),
    Ctrl UMETA(DisplayName = "Ctrl"),
    None UMETA(DisplayName = "None / Disabled")
};

UCLASS(config = Editor, defaultconfig, meta = (DisplayName = "Index"))
class INDEX_API UIndexSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UIndexSettings();

    virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    UPROPERTY(EditAnywhere, Config, Category = "General", meta = (DisplayName = "Enable Custom Ordering"))
    bool bEnableCustomOrdering;

    UPROPERTY(EditAnywhere, Config, Category = "Appearance|Reorder", meta = (DisplayName = "Above Line Color"))
    FLinearColor AboveLineColor;

    UPROPERTY(EditAnywhere, Config, Category = "Appearance|Reorder", meta = (DisplayName = "Below Line Color"))
    FLinearColor BelowLineColor;

    UPROPERTY(EditAnywhere, Config, Category = "Drop Zone", meta = (DisplayName = "Edge Threshold (%)", ClampMin = "1.0", ClampMax = "45.0"))
    float EdgeThresholdPercent;

    UPROPERTY(EditAnywhere, Config, Category = "Drop Zone", meta = (DisplayName = "Force Reorder Modifier Key"))
    EIndexModifierKey ReorderModifierKey;

    static const UIndexSettings* Get();
};
