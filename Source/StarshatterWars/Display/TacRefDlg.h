/*=============================================================================
    Project:        Starshatter Wars (Unreal Port)
    Studio:         Fractal Dev Studios
    Copyright:      (c) 2025-2026.

    SUBSYSTEM:      UI / Tactical Reference
    FILE:           TacRefDlg.h
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    UTacRefDlg implements the Tactical Reference screen.

    KEY FEATURES
    ============
    - Single shared dropdown (ComboBoxString: ShipCombo)
    - Category buttons filter the dropdown contents:
        STATION   -> EShipCategory::Station
        SHIP      -> EShipCategory::CapitalShip
        FIGHTER   -> EShipCategory::Fighter
        TRANSPORT -> EShipCategory::Transport
        FACILITY  -> EShipCategory::Building
    - Weapon button switches to weapon reference placeholder (existing behavior)

    IMPORTANT
    =========
    - CancelButton is provided by UBaseScreen (not declared here).
    - This class expects EShipCategory and DeriveShipCategoryFromClass() to live in
      GameStructs_System.h (as you described).
    - BindWidgetOptional names MUST match your WBP widget names exactly:
        ShipCombo, StationButton, ShipButton, FighterButton, TransportButton,
        BuildingButton, WeaponButton, TitleText, TxtCaption, TxtStats, TxtDescription

=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "Engine/DataTable.h"

// EShipCategory + DeriveShipCategoryFromClass(...)
#include "GameStructs_System.h"

#include "TacRefDlg.generated.h"

class UButton;
class UTextBlock;
class URichTextBlock;
class UComboBoxString;
class UMenuScreen;

struct FShipDesign;
struct FShipWeapon;

UCLASS()
class STARSHATTERWARS_API UTacRefDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UTacRefDlg(const FObjectInitializer& ObjectInitializer);

    // Menu manager hookup (matches your existing pattern)
    virtual void SetMenuManager(UMenuScreen* InManager);
    virtual void InitializeDlg(UMenuScreen* InManager);

    // Screen lifecycle
    void Show();
    void ExecFrame();

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
    // ---------------------------------------------------------------------
    // Data
    // ---------------------------------------------------------------------
    UPROPERTY(EditDefaultsOnly, Category = "TacRef|Data")
    TObjectPtr<UDataTable> ShipDesignTable = nullptr;

    // Dropdown option index -> DataTable RowName mapping (FILTERED list)
    UPROPERTY(Transient)
    TArray<FName> ShipRowNames;

    UPROPERTY(Transient)
    int32 SelectedShipIndex = INDEX_NONE;

    // Current category filter for ShipCombo
    UPROPERTY(Transient)
    EShipCategory ActiveCategory = EShipCategory::Unknown;
    
    UPROPERTY(Transient)
    EShipEmpire ActiveEmpire = EShipEmpire::NONE;

    // Dropdown option index -> Empire value mapping
    UPROPERTY(Transient)
    TArray<EShipEmpire> EmpireValues;

    // 0 = ship/facility/etc pages, 1 = weapon page
    int32 Mode = 0;

protected:
    // ---------------------------------------------------------------------
    // Widgets (BindWidgetOptional names MUST match the WBP)
    // ---------------------------------------------------------------------

    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* TitleText = nullptr;

    // Shared dropdown for ships/facilities/etc
    UPROPERTY(meta = (BindWidgetOptional)) UComboBoxString* ShipCombo = nullptr;

    UPROPERTY(meta = (BindWidgetOptional)) UComboBoxString* EmpireCombo = nullptr;

    // Category buttons
    UPROPERTY(meta = (BindWidgetOptional)) UButton* StationButton = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UButton* ShipButton = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UButton* FighterButton = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UButton* TransportButton = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UButton* BuildingButton = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UButton* WeaponButton = nullptr;

    // Output fields (optional; only used if present in WBP)

    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* TxtDisplayName = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* TxtCaption = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) URichTextBlock* TxtStats = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) URichTextBlock* TxtDescription = nullptr;

    UPROPERTY(EditAnywhere, Category = "TacRef|Filters")
    bool bShowSecretDesigns = false;   // dev/debug toggle
protected:
    // ---------------------------------------------------------------------
    // Events
    // ---------------------------------------------------------------------
    UFUNCTION() void HandleCloseClicked();

    UFUNCTION() bool PassesSecretFilter(const FShipDesign& Row) const;

    UFUNCTION() bool PassesEmpireFilter(const FShipDesign& Row) const;

    void PopulateEmpireDropdown();

    UFUNCTION() void HandleStationModeClicked();
    UFUNCTION() void HandleShipModeClicked();
    UFUNCTION() void HandleFighterModeClicked();
    UFUNCTION() void HandleTransportModeClicked();
    UFUNCTION() void HandleBuildingModeClicked();

    UFUNCTION() void HandleWeaponModeClicked();
    UFUNCTION() void HandleShipComboChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION() void HandleEmpireComboChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION() void PopulateEmpireDropdown_AutoDetect(bool bRespectCategory = true, bool bRespectSecret = true);

protected:
    // ---------------------------------------------------------------------
    // Internals
    // ---------------------------------------------------------------------
    void PopulateShipDropdown();
    bool PassesCategoryFilter(const FShipDesign& Row) const;

    void SelectShipByIndex(int32 Index);
    void BuildShipTexts(const FShipDesign& Dsn, FString& OutCaption, FString& OutStats, FString& OutDesc) const;

protected:
    UPROPERTY(Transient)
    TObjectPtr<UMenuScreen> manager = nullptr;
};
