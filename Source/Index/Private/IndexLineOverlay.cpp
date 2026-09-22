#include "IndexLineOverlay.h"

#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SWindow.h"

FIndexLineOverlay::~FIndexLineOverlay()
{
    Shutdown();
}

void FIndexLineOverlay::EnsureLineWindow()
{
    if (LineWindow.IsValid() || !FSlateApplication::IsInitialized())
    {
        return;
    }

    LineWindow = SNew(SWindow)
        .Type(EWindowType::ToolTip)
        .SizingRule(ESizingRule::FixedSize)
        .ClientSize(FVector2D(1.0f, 2.0f))
        .CreateTitleBar(false)
        .SupportsMinimize(false)
        .SupportsMaximize(false)
        .IsPopupWindow(true)
        .IsTopmostWindow(true)
        .FocusWhenFirstShown(false)
        .ActivationPolicy(EWindowActivationPolicy::Never)
        .SupportsTransparency(EWindowTransparency::PerPixel)
        .UseOSWindowBorder(false)
        [
            SNew(SBorder)
            .Padding(0.0f)
            .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
            .BorderBackgroundColor_Lambda([this]() { return CurrentLineColor; })
        ];

    LineWindow->SetAcceptsInput(false);
    FSlateApplication::Get().AddWindow(LineWindow.ToSharedRef(), false);
}


void FIndexLineOverlay::ShowLine(
    const FGeometry& RowGeometry,
    const FGeometry& HorizontalBoundsGeometry,
    bool bBefore,
    float LeftInset,
    const FLinearColor& Color,
    float Thickness)
{
    EnsureLineWindow();
    if (!LineWindow.IsValid())
    {
        return;
    }

    CurrentLineColor = Color;
    const FVector2D RowPosition = RowGeometry.GetAbsolutePosition();
    const FVector2D RowSize = RowGeometry.GetAbsoluteSize();
    const FVector2D BoundsPosition = HorizontalBoundsGeometry.GetAbsolutePosition();
    const FVector2D BoundsSize = HorizontalBoundsGeometry.GetAbsoluteSize();
    const float LineThickness = FMath::Max(1.0f, Thickness);

    const float BoundaryY = bBefore
        ? RowPosition.Y
        : RowPosition.Y + RowSize.Y;
    const float LineY = BoundaryY - (LineThickness * 0.5f);

    // Reorder feedback is intentionally a full-width row boundary. Hierarchy depth
    // is communicated by the target row and drag tooltip rather than shortening the
    // line, which made deeper targets look inconsistent and visually weaker. Use the
    // complete Outliner tree viewport for horizontal bounds every time.
    (void)LeftInset;
    const float LineX = BoundsPosition.X;
    const float LineWidth = BoundsSize.X;
    LineWindow->ReshapeWindow(
        FVector2D(LineX, LineY),
        FVector2D(FMath::Max(1.0f, LineWidth), LineThickness));

    LineWindow->ShowWindow();
}


void FIndexLineOverlay::HideLine()
{
    if (LineWindow.IsValid())
    {
        LineWindow->HideWindow();
    }
}

void FIndexLineOverlay::Hide()
{
    HideLine();
}

void FIndexLineOverlay::Shutdown()
{
    if (FSlateApplication::IsInitialized() && LineWindow.IsValid())
    {
        FSlateApplication::Get().RequestDestroyWindow(LineWindow.ToSharedRef());
    }
    LineWindow.Reset();
}
