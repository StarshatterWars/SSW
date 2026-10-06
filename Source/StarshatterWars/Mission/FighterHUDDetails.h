#pragma once
#include "Widgets/SLeafWidget.h"
#include "Engine/Texture2D.h"
#include "UObject/StrongObjectPtr.h"

class AActor;

// Live fighter readouts. Simulation objects are sampled, never retained.
class SFighterHUDDetails : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SFighterHUDDetails) {}
        SLATE_ATTRIBUTE(bool, ShowHUD)
        SLATE_ATTRIBUTE(TWeakObjectPtr<AActor>, SelectedTarget)
        SLATE_ATTRIBUTE(FText, SelectedName)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args);
    virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }
    virtual void Tick(const FGeometry&, double Now, float Delta) override;
    virtual int32 OnPaint(const FPaintArgs&, const FGeometry&, const FSlateRect&,
        FSlateWindowElementList&, int32, const FWidgetStyle&, bool) const override;
    void CycleMFD(int32 Index);
    void ToggleWeaponsPanel() { bShowWeapons = !bShowWeapons; bLoggedPanelPaint = false; Invalidate(EInvalidateWidgetReason::Paint); }
    void ToggleCautionPanel() { bShowCaution = !bShowCaution; bLoggedPanelPaint = false; Invalidate(EInvalidateWidgetReason::Paint); }
    void CycleWeapon(bool bPrimary);
private:
    struct FRow { FString Label; double Percent; };
    struct FBlip { double Az, El, Range; FLinearColor Color; };
    enum class EMode { Off, Ship, FOV, HSD, ThreeD };
    void Refresh();
    void LoadShipHUDIcon(const FString& Path, FString& CachedPath, TStrongObjectPtr<UTexture2D>& Texture, FSlateBrush& Brush);
    TAttribute<bool> ShowHUD;
    TAttribute<TWeakObjectPtr<AActor>> SelectedTarget;
    TAttribute<FText> SelectedName;
    double NextSample = 0;
    mutable bool bLoggedPanelPaint = false;
    bool bShowWeapons = true;
    bool bShowCaution = true;
    EMode Modes[2] = { EMode::Ship, EMode::FOV };
    TArray<FRow> StatusRows, DamageRows;
    TArray<FBlip> Contacts;
    FString PlayerName, TargetName, TargetClass, Primary, Secondary;
    double PlayerHull = -1, TargetHull = -1, TargetShield = -1, TargetRange = -1, SensorRange = 1;
    bool bHasPlayer = false, bHasTarget = false;
    FString PlayerIconPath, TargetIconPath;
    TStrongObjectPtr<UTexture2D> PlayerTexture, TargetTexture;
    FSlateBrush PlayerBrush, TargetBrush;
    // TAC left/right, followed by sensor FOV/HSD/3D.
    TStrongObjectPtr<UTexture2D> PanelTextures[7];
    FSlateBrush PanelBrushes[7];
};
