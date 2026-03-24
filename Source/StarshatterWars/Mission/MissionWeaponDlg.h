/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         MissionWeaponDlg.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UMissionWeaponDlg (Unreal)
    - Port of legacy MsnWepDlg.
    - Mission briefing WEAPON / LOADOUT dialog.
    - Parses legacy FORM (.frm) via UBaseScreen.
    - Uses FORM control IDs (no direct widget UPROPERTYs).
    - Handles weapon hardpoint mounting and loadout selection.
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"

#include "Math/Vector.h"               // FVector
#include "Math/Color.h"                // FColor
#include "Math/UnrealMathUtility.h"    // Math

#include "GameStructs.h"

#include "MissionWeaponDlg.generated.h"

// --------------------------------------------------------------------
// Forward declarations (keep header light)
// --------------------------------------------------------------------

class Campaign;
class Mission;
class MissionElement; 
class SimElement;
class ShipDesign;
class WeaponDesign;
class HardPoint;
class MissionLoad;
class ShipLoad;

class UMissionPlanner;

// --------------------------------------------------------------------
// Helper struct: mount button -> weapon/station mapping
// --------------------------------------------------------------------

struct FMountSlot
{
    int32 WeaponIndex = -1;
    int32 StationIndex = -1;
};

// --------------------------------------------------------------------
// UMissionWeaponDlg
// --------------------------------------------------------------------

UCLASS()
class STARSHATTERWARS_API UMissionWeaponDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UMissionWeaponDlg(const FObjectInitializer& ObjectInitializer);

    // ------------------------------------------------------------
    // Screen lifecycle
    // ------------------------------------------------------------

    virtual void NativeConstruct() override;

    virtual void Show();
    virtual void ExecFrame(double DeltaTime) override;

    // ------------------------------------------------------------
    // Dialog input hooks (Enter / Escape)
    // ------------------------------------------------------------

    virtual void HandleAccept() override;
    virtual void HandleCancel() override;

    // ------------------------------------------------------------
    // Setup
    // ------------------------------------------------------------

    void SetMissionPlanner(UMissionPlanner* InPlanner) { MissionPlanner = InPlanner; }
    void SetCampaign(Campaign* InCampaign) { CampaignPtr = InCampaign; }
    void SetMission(Mission* InMission) { MissionPtr = InMission; }

protected:
    // ------------------------------------------------------------
    // Legacy logic ports
    // ------------------------------------------------------------

    void SetupControls();
    void BuildLists();

    int  LoadToPointIndex(int Station) const;
    int  PointIndexToLoad(int Station, int PointIndex) const;

    // ------------------------------------------------------------
    // Event wiring
    // ------------------------------------------------------------

    void WireEvents();

    // ------------------------------------------------------------
    // Button handlers (no lambdas)
    // ------------------------------------------------------------

    UFUNCTION()
    void OnAcceptClicked();

    UFUNCTION()
    void OnCancelClicked();

    UFUNCTION()
    void OnTabSit();

    UFUNCTION()
    void OnTabPkg();

    UFUNCTION()
    void OnTabMap();

    UFUNCTION()
    void OnTabWep();

    UFUNCTION()
    void OnMountClicked();

protected:
    // ------------------------------------------------------------
    // Data pointers (non-owning)
    // ------------------------------------------------------------

    Campaign* CampaignPtr = nullptr;
    Mission* MissionPtr = nullptr;
    UMissionPlanner* MissionPlanner = nullptr;

    MissionElement* Elem = nullptr;

    // ------------------------------------------------------------
    // Weapon / loadout state (legacy layout preserved)
    // ------------------------------------------------------------

    WeaponDesign* Designs[8];
    bool              Mounts[8][8];
    int               Loads[8];
    int               FirstStation = 0;

    // ------------------------------------------------------------
    // Button -> mount slot mapping
    // ------------------------------------------------------------

    TMap<UButton*, FMountSlot> ButtonIdToSlot;
};
