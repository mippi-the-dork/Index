// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class SWindow;

class FIndexLineOverlay
{
public:
    ~FIndexLineOverlay();

    void ShowLine(
        const FGeometry& RowGeometry,
        const FGeometry& HorizontalBoundsGeometry,
        bool bBefore,
        float LeftInset,
        const FLinearColor& Color,
        float Thickness = 2.0f);

    void HideLine();
    void Hide();
    void Shutdown();

private:
    void EnsureLineWindow();

    TSharedPtr<SWindow> LineWindow;
    FLinearColor CurrentLineColor = FLinearColor::White;
};
