/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         MissionPlanner.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UMissionPlanner (Unreal)
    - Legacy PlanScreen port.
    - Creates and owns mission planning dialogs.
    - Centralizes tab switching and topmost dialog closing.
*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "Math/Vector.h"               // FVector
#include "Math/Color.h"                // FColor
#include "Math/UnrealMathUtility.h"    // Math

#include "GameStructs.h"

#include "MissionPlanner.generated.h"

// Forward declarations (keep header light)
class UUserWidget;

class UMissionObjectiveDlg;
class UMissionPackageDlg;
class UMissionWeaponDlg;
class UMissionNavDlg;
class UMissionDebriefDlg;
class UMissionAwardDlg;

UCLASS()
class STARSHATTERWARS_API UMissionPlanner : public UUserWidget
{
    GENERATED_BODY()

public:
    UMissionPlanner(const FObjectInitializer& ObjectInitializer);

    // Setup / teardown (legacy)
    void Setup();
    void TearDown();
    bool CloseTopmost();

    // Screen visibility (legacy)
    bool IsShown() const { return bIsShown; }
    void Show();
    void Hide();

    // Dialog switching (legacy API)
    void ShowMsnDlg();
    void HideMsnDlg();
    bool IsMsnShown() const;

    void ShowMsnObjDlg();
    void HideMsnObjDlg();
    bool IsMsnObjShown() const;
    UMissionObjectiveDlg* GetMsnObjDlg() const { return ObjectiveDlg; }

    void ShowMsnPkgDlg();
    void HideMsnPkgDlg();
    bool IsMsnPkgShown() const;
    UMissionPackageDlg* GetMsnPkgDlg() const { return PackageDlg; }

    void ShowMsnWepDlg();
    void HideMsnWepDlg();
    bool IsMsnWepShown() const;
    UMissionWeaponDlg* GetMsnWepDlg() const { return WeaponDlg; }

    void ShowNavDlg();
    void HideNavDlg();
    bool IsNavShown() const;
    UMissionNavDlg* GetNavDlg() const { return NavDlg; }

    void ShowDebriefDlg();
    void HideDebriefDlg();
    bool IsDebriefShown() const;
    UMissionDebriefDlg* GetDebriefDlg() const { return DebriefDlg; }

    void ShowAwardDlg();
    void HideAwardDlg();
    bool IsAwardShown() const;
    UMissionAwardDlg* GetAwardDlg() const { return AwardDlg; }

    // Per-frame (legacy)

    void ExecFrame(float DeltaSeconds);
    void HideAll();

    // Callbacks used by UMissionNavDlg (you referenced these in cpp)
    void OnMissionBriefingAccept();
    void OnMissionBriefingCancel();



    void ShowBriefingSitTab();
    void ShowBriefingPkgTab();
    void ShowBriefingMapTab();
    void ShowBriefingWepTab();

    void NavModeGalaxy();
    void NavModeSystem();
    void NavModeSector();

    void NavZoomIn();
    void NavZoomOut();

    void NavFilterSystem();
    void NavFilterPlanet();
    void NavFilterSector();
    void NavFilterStation();
    void NavFilterStarship();
    void NavFilterFighter();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
    template<typename TWidget>
    TWidget* CreateDialogWidget(TSubclassOf<UUserWidget> Class, const TCHAR* DebugName);

    static bool IsDialogActuallyShown(const UUserWidget* W);

    static void KeepAlive(UObject* Obj);
    static void ReleaseAlive(UObject* Obj);

    void ShowDialog(UUserWidget* Dlg);
    void HideDialog(UUserWidget* Dlg);

protected:
    // Widget classes (assign in editor or owning screen)
    UPROPERTY(EditDefaultsOnly, Category = "MissionPlanner|Classes")
    TSubclassOf<UUserWidget> MissionObjectiveDlgClass;

    UPROPERTY(EditDefaultsOnly, Category = "MissionPlanner|Classes")
    TSubclassOf<UUserWidget> MissionPackageDlgClass;

    UPROPERTY(EditDefaultsOnly, Category = "MissionPlanner|Classes")
    TSubclassOf<UUserWidget> MissionWeaponDlgClass;

    UPROPERTY(EditDefaultsOnly, Category = "MissionPlanner|Classes")
    TSubclassOf<UUserWidget> MissionNavDlgClass;

    UPROPERTY(EditDefaultsOnly, Category = "MissionPlanner|Classes")
    TSubclassOf<UUserWidget> MissionDebriefDlgClass;

    UPROPERTY(EditDefaultsOnly, Category = "MissionPlanner|Classes")
    TSubclassOf<UUserWidget> AwardDlgClass;

protected:
    // Owned dialogs
    UPROPERTY(Transient) TObjectPtr<UMissionObjectiveDlg> ObjectiveDlg = nullptr;
    UPROPERTY(Transient) TObjectPtr<UMissionPackageDlg>   PackageDlg = nullptr;
    UPROPERTY(Transient) TObjectPtr<UMissionWeaponDlg>    WeaponDlg = nullptr;
    UPROPERTY(Transient) TObjectPtr<UMissionNavDlg>       NavDlg = nullptr;
    UPROPERTY(Transient) TObjectPtr<UMissionDebriefDlg>   DebriefDlg = nullptr;
    UPROPERTY(Transient) TObjectPtr<UMissionAwardDlg>     AwardDlg = nullptr;

    bool bIsShown = false;


};
