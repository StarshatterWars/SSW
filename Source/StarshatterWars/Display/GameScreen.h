/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         GameScreen.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UGameScreen
    - Unreal port of legacy GameScreen (IN-GAME HUD/OVERLAYS ONLY).
    - Implemented as a UMG UserWidget (inherits from UBaseScreen).
    - Owns only in-game overlays (Nav/Eng/Flt/Wep) that belong to the gameplay screen.
    - DOES NOT own Options dialogs anymore (Audio/Video/Opt/Ctl/Key/Joy).
      Those are managed by MenuScreen -> OptionsScreen -> sub-dialogs.
    - Legacy "views" (HUD/Wep/Quantum/Radio/Tactical/etc.) remain pure C++ and are ticked from ExecFrame.
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "CameraView.h"
#include "GameScreen.generated.h"

// Forward declares (UMG overlays owned by GameScreen):
class UNavDlg;
class UEngineeringDlg;
class UFlightOpsDlg;
class UWepDlg;

// Legacy systems (raw classes):
class Sim;
class Ship;
class Starshatter;
class CameraManager;
class DisplayView;
class HUDView;
class WepView;
class QuantumView;
class RadioView;
class TacticalView;
class CameraView;
class Bitmap;
class DataLoader;
class UOptionsScreen;

UCLASS()
class STARSHATTERWARS_API UGameScreen : public UBaseScreen
{
    GENERATED_BODY()

public:
    UGameScreen(const FObjectInitializer& ObjectInitializer);

    // Lifecycle:
    UFUNCTION() void Setup();
    UFUNCTION() void TearDown();

    UFUNCTION() bool CloseTopmost();

    UFUNCTION() void Show();
    UFUNCTION() void Hide();

    UFUNCTION() bool IsShown() const { return bIsShown; }

    // Overlay show/hide:
    UFUNCTION() void ShowNavDlg();
    UFUNCTION() void HideNavDlg();
    UFUNCTION() bool IsNavShown() const;

    UFUNCTION() void ShowEngDlg();
    UFUNCTION() void HideEngDlg();
    UFUNCTION() bool IsEngShown() const;

    UFUNCTION() void ShowFltDlg();
    UFUNCTION() void HideFltDlg();
    UFUNCTION() bool IsFltShown() const;

    // Weapons overlay (tied to WepView overlay mode):
    UFUNCTION() virtual void ShowWeaponsOverlay();
    UFUNCTION() virtual void HideWeaponsOverlay();

    // Legacy ops:
    void   SetFieldOfView(double Fov);
    double GetFieldOfView() const;

    void CycleMFDMode(int Mfd);
    void CycleHUDMode();
    void CycleHUDColor();
    void CycleHUDWarn();

    void FrameRate(double F);
    void ExecFrame(float DeltaTime);

    // External/Internal camera panel toggles (if you still use these in HUD):
    UFUNCTION() void ShowExternal();
    UFUNCTION() void ShowInternal();
    UFUNCTION() bool IsExternalVisible() const { return bExternalVisible; }
    UFUNCTION() bool IsFormShown() const;

    // Accessors:
    UNavDlg* GetNavDlg() const { return NavDlg; }
    UEngineeringDlg* GetEngDlg() const { return EngDlg; }
    UFlightOpsDlg* GetFltDlg() const { return FltDlg; }

    static UGameScreen* GetInstance() { return GameScreenInstance; }

protected:
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    UUserWidget* MakeDlg(TSubclassOf<UUserWidget> Class, int32 ZOrder);

    void HideAllOverlays();
    void SetDlgVisible(UUserWidget* W, bool bVisible) const;
    bool IsDlgVisible(const UUserWidget* W) const;

    bool IsOverlayShown() const;

    void ApplyInputModeForScreen(bool bExternal);

private:
    // Spawnable widget classes (assign in defaults/BP):
    UPROPERTY(EditDefaultsOnly, Category = "UI|Classes") TSubclassOf<UNavDlg>         NavDlgClass;
    UPROPERTY(EditDefaultsOnly, Category = "UI|Classes") TSubclassOf<UEngineeringDlg> EngDlgClass;
    UPROPERTY(EditDefaultsOnly, Category = "UI|Classes") TSubclassOf<UFlightOpsDlg>   FltDlgClass;

    // Spawned overlay instances:
    UNavDlg* NavDlg = nullptr;
    UEngineeringDlg* EngDlg = nullptr;
    UFlightOpsDlg* FltDlg = nullptr;
    UOptionsScreen* OptDlg = nullptr;

    // Legacy raw systems (NOT UPROPERTY):
    Sim* sim = nullptr;
    CameraManager* cam_dir = nullptr;
    DisplayView* disp_view = nullptr;
    HUDView* hud_view = nullptr;
    WepView* wep_view = nullptr;
    QuantumView* quantum_view = nullptr;
    RadioView* radio_view = nullptr;
    TacticalView* tac_view = nullptr;
    CameraView* cam_view = nullptr;

    DataLoader* loader = nullptr;

    Bitmap* flare1 = nullptr;
    Bitmap* flare2 = nullptr;
    Bitmap* flare3 = nullptr;
    Bitmap* flare4 = nullptr;

    double frame_rate = 0.0;
    bool   bIsShown = false;

    bool bExternalVisible = false;

    static UGameScreen* GameScreenInstance;
};
