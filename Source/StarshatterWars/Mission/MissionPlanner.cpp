/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         MissionPlanner.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UMissionPlanner (Unreal)
    - Legacy PlanScreen port.
    - Creates and owns mission planning dialogs.
    - Centralizes tab switching and dialog lifetime.
*/

#include "MissionPlanner.h"

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"

#include "MissionObjectiveDlg.h"
#include "MissionPackageDlg.h"
#include "MissionWeaponDlg.h"
#include "MissionNavDlg.h"
#include "MissionDebriefDlg.h"
#include "MissionAwardDlg.h"

// --------------------------------------------------------------------
// CONSTRUCTION / LIFECYCLE
// --------------------------------------------------------------------

UMissionPlanner::UMissionPlanner(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UMissionPlanner::NativeConstruct()
{
    Super::NativeConstruct();
}

void UMissionPlanner::NativeDestruct()
{
    TearDown();
    Super::NativeDestruct();
}

void UMissionPlanner::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    ExecFrame(InDeltaTime);
}

// --------------------------------------------------------------------
// SETUP / TEARDOWN
// --------------------------------------------------------------------

void UMissionPlanner::Setup()
{
    if (ObjectiveDlg || PackageDlg || WeaponDlg || NavDlg || DebriefDlg || AwardDlg)
        return;

    ObjectiveDlg = CreateDialogWidget<UMissionObjectiveDlg>(MissionObjectiveDlgClass, TEXT("MissionObjectiveDlg"));
    PackageDlg = CreateDialogWidget<UMissionPackageDlg>(MissionPackageDlgClass, TEXT("MissionPackageDlg"));
    NavDlg = CreateDialogWidget<UMissionNavDlg>(MissionNavDlgClass, TEXT("MissionNavDlg"));
    WeaponDlg = CreateDialogWidget<UMissionWeaponDlg>(MissionWeaponDlgClass, TEXT("MissionWeaponDlg"));
    AwardDlg = CreateDialogWidget<UMissionAwardDlg>(AwardDlgClass, TEXT("MissionAwardDlg"));
    DebriefDlg = CreateDialogWidget<UMissionDebriefDlg>(MissionDebriefDlgClass, TEXT("MissionDebriefDlg"));

    HideAll();
    bIsShown = false;
}

void UMissionPlanner::TearDown()
{
    HideAll();

    auto DestroyDlg = [this](UUserWidget*& W)
        {
            if (!W)
                return;

            ReleaseAlive(W);

            if (W->IsInViewport())
                W->RemoveFromParent();

            W = nullptr;
        };

    DestroyDlg(reinterpret_cast<UUserWidget*&>(ObjectiveDlg));
    DestroyDlg(reinterpret_cast<UUserWidget*&>(PackageDlg));
    DestroyDlg(reinterpret_cast<UUserWidget*&>(WeaponDlg));
    DestroyDlg(reinterpret_cast<UUserWidget*&>(NavDlg));
    DestroyDlg(reinterpret_cast<UUserWidget*&>(DebriefDlg));
    DestroyDlg(reinterpret_cast<UUserWidget*&>(AwardDlg));

    bIsShown = false;
}

// --------------------------------------------------------------------
// FRAME / CLOSE TOPMOST (LEGACY)
// --------------------------------------------------------------------

void UMissionPlanner::ExecFrame(float DeltaSeconds)
{
    if (!bIsShown)
        return;

    (void)DeltaSeconds;
}

bool UMissionPlanner::CloseTopmost()
{
    if (DebriefDlg && IsDialogActuallyShown(DebriefDlg))
    {
        HideDialog(DebriefDlg);
    }

    if (AwardDlg && IsDialogActuallyShown(AwardDlg))
    {
        return true;
    }

    return false;
}

// --------------------------------------------------------------------
// SHOW / HIDE (SCREEN LEVEL)
// --------------------------------------------------------------------

void UMissionPlanner::Show()
{
    if (!bIsShown)
    {
        ShowMsnDlg();
        bIsShown = true;
    }
}

void UMissionPlanner::Hide()
{
    HideAll();
    bIsShown = false;
}

// --------------------------------------------------------------------
// DIALOG SWITCHING (LEGACY API)
// --------------------------------------------------------------------

void UMissionPlanner::ShowMsnDlg()
{
    HideAll();
    ShowDialog(ObjectiveDlg);
}

void UMissionPlanner::HideMsnDlg()
{
    HideAll();
}

bool UMissionPlanner::IsMsnShown() const
{
    return IsMsnObjShown() || IsMsnPkgShown() || IsMsnWepShown();
}

// -----------------------------
// Mission Objective
// -----------------------------

void UMissionPlanner::ShowMsnObjDlg()
{
    HideAll();
    ShowDialog(ObjectiveDlg);
}

void UMissionPlanner::HideMsnObjDlg()
{
    HideAll();
}

bool UMissionPlanner::IsMsnObjShown() const
{
    return ObjectiveDlg && IsDialogActuallyShown(ObjectiveDlg);
}

// -----------------------------
// Mission Package
// -----------------------------

void UMissionPlanner::ShowMsnPkgDlg()
{
    HideAll();
    ShowDialog(PackageDlg);
}

void UMissionPlanner::HideMsnPkgDlg()
{
    HideAll();
}

bool UMissionPlanner::IsMsnPkgShown() const
{
    return PackageDlg && IsDialogActuallyShown(PackageDlg);
}

// -----------------------------
// Mission Weapon
// -----------------------------

void UMissionPlanner::ShowMsnWepDlg()
{
    HideAll();
    ShowDialog(WeaponDlg);
}

void UMissionPlanner::HideMsnWepDlg()
{
    HideAll();
}

bool UMissionPlanner::IsMsnWepShown() const
{
    return WeaponDlg && IsDialogActuallyShown(WeaponDlg);
}

// -----------------------------
// Navigation
// -----------------------------

void UMissionPlanner::ShowNavDlg()
{
    if (NavDlg && !IsDialogActuallyShown(NavDlg))
    {
        HideAll();
        ShowDialog(NavDlg);
    }
}

void UMissionPlanner::HideNavDlg()
{
    if (NavDlg && IsDialogActuallyShown(NavDlg))
    {
        HideAll();
    }
}

bool UMissionPlanner::IsNavShown() const
{
    return NavDlg && IsDialogActuallyShown(NavDlg);
}

// -----------------------------
// Debrief
// -----------------------------

void UMissionPlanner::ShowDebriefDlg()
{
    HideAll();
    ShowDialog(DebriefDlg);
}

void UMissionPlanner::HideDebriefDlg()
{
    HideAll();
}

bool UMissionPlanner::IsDebriefShown() const
{
    return DebriefDlg && IsDialogActuallyShown(DebriefDlg);
}

// -----------------------------
// Award
// -----------------------------

void UMissionPlanner::ShowAwardDlg()
{
    HideAll();
    ShowDialog(AwardDlg);
}

void UMissionPlanner::HideAwardDlg()
{
    HideAll();
}

bool UMissionPlanner::IsAwardShown() const
{
    return AwardDlg && IsDialogActuallyShown(AwardDlg);
}

// --------------------------------------------------------------------
// HIDE ALL
// --------------------------------------------------------------------

void UMissionPlanner::HideAll()
{
    HideDialog(ObjectiveDlg);
    HideDialog(PackageDlg);
    HideDialog(WeaponDlg);
    HideDialog(NavDlg);
    HideDialog(AwardDlg);
    HideDialog(DebriefDlg);
}

// --------------------------------------------------------------------
// INTERNAL HELPERS
// --------------------------------------------------------------------

template<typename TWidget>
TWidget* UMissionPlanner::CreateDialogWidget(TSubclassOf<UUserWidget> Class, const TCHAR* DebugName)
{
    if (!Class)
    {
        UE_LOG(LogTemp, Warning, TEXT("MissionPlanner: Missing widget class for %s"), DebugName);
        return nullptr;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("MissionPlanner: No world for %s"), DebugName);
        return nullptr;
    }

    UUserWidget* Created = CreateWidget<UUserWidget>(World, Class);
    if (!Created)
    {
        UE_LOG(LogTemp, Warning, TEXT("MissionPlanner: Failed to create %s"), DebugName);
        return nullptr;
    }

    Created->AddToViewport();
    Created->SetVisibility(ESlateVisibility::Collapsed);

    KeepAlive(Created);

    return Cast<TWidget>(Created);
}

bool UMissionPlanner::IsDialogActuallyShown(const UUserWidget* W)
{
    if (!W) return false;
    if (!W->IsInViewport()) return false;

    const ESlateVisibility V = W->GetVisibility();
    return (V != ESlateVisibility::Collapsed && V != ESlateVisibility::Hidden);
}

void UMissionPlanner::KeepAlive(UObject* Obj)
{
    if (Obj && !Obj->IsRooted())
        Obj->AddToRoot();
}

void UMissionPlanner::ReleaseAlive(UObject* Obj)
{
    if (Obj && Obj->IsRooted())
        Obj->RemoveFromRoot();
}

void UMissionPlanner::ShowDialog(UUserWidget* Dlg)
{
    if (!Dlg) return;

    if (!Dlg->IsInViewport())
        Dlg->AddToViewport();

    Dlg->SetVisibility(ESlateVisibility::Visible);
}

void UMissionPlanner::HideDialog(UUserWidget* Dlg)
{
    if (!Dlg) return;
    Dlg->SetVisibility(ESlateVisibility::Collapsed);
}

// --------------------------------------------------------------------
// CALLBACKS USED BY NAV DIALOG
// --------------------------------------------------------------------

void UMissionPlanner::OnMissionBriefingAccept()
{
    ShowMsnPkgDlg();
}

void UMissionPlanner::OnMissionBriefingCancel()
{
    Hide();
}

void UMissionPlanner::ShowBriefingSitTab() {}
void UMissionPlanner::ShowBriefingPkgTab() {}
void UMissionPlanner::ShowBriefingMapTab() {}
void UMissionPlanner::ShowBriefingWepTab() {}

void UMissionPlanner::NavModeGalaxy() {}
void UMissionPlanner::NavModeSystem() {}
void UMissionPlanner::NavModeSector() {}

void UMissionPlanner::NavZoomIn() {}
void UMissionPlanner::NavZoomOut() {}

void UMissionPlanner::NavFilterSystem() {}
void UMissionPlanner::NavFilterPlanet() {}
void UMissionPlanner::NavFilterSector() {}
void UMissionPlanner::NavFilterStation() {}
void UMissionPlanner::NavFilterStarship() {}
void UMissionPlanner::NavFilterFighter() {}
