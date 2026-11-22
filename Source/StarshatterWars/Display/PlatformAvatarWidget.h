#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SOverlay.h"
#include "Engine/GameInstance.h"
#include "PlatformAvatarSubsystem.h"
#include "ShipSystemIcon.h"
#include "Styling/CoreStyle.h"

class SPlatformAvatarWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SPlatformAvatarWidget) {} SLATE_ARGUMENT(UGameInstance*, GameInstance) SLATE_END_ARGS()
    void Construct(const FArguments& Args)
    {
        Cache = Args._GameInstance ? Args._GameInstance->GetSubsystem<UPlatformAvatarSubsystem>() : nullptr;
        ChildSlot[SNew(SBox).WidthOverride(64).HeightOverride(64)
            [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
            .BorderBackgroundColor(FLinearColor(.08f,.16f,.2f)).Padding(2)
            .ToolTipText_Lambda([this] { return Cache.IsValid() ? Cache->GetStatus() : INVTEXT("Pilot"); })
            [SNew(SOverlay)
                + SOverlay::Slot()[SNew(SImage).Image_Lambda([this] { return Cache.IsValid() ? Cache->GetBrush() : nullptr; })]
                + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
                    [SNew(SShipSystemIcon).Icon(EShipSystemIcon::Agent)
                    .Visibility_Lambda([this] { return Cache.IsValid() && Cache->GetBrush() ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })]]]];
    }
    virtual void Tick(const FGeometry& Geometry, double Time, float Delta) override
    {
        SCompoundWidget::Tick(Geometry,Time,Delta);
        if (Time >= NextRefresh) { NextRefresh = Time + 1; if (Cache.IsValid()) Cache->Refresh(); }
    }
private:
    TWeakObjectPtr<UPlatformAvatarSubsystem> Cache;
    double NextRefresh = 0;
};
