/*=============================================================================
    Project:        Starshatter Wars (Unreal Engine Port)
    Studio:         Fractal Dev Studios
    Copyright:      (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:      Asset Registry
    FILE:           StarshatterAssetRegistrySettings.h
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    UStarshatterAssetRegistrySettings

    Config-backed asset registry bindings for Starshatter Wars.

    Stores authoritative bindings of:
      - AssetId -> Soft object reference (UObject)
      - Optional convenience bindings (ex: WeaponDesignTable) for high-traffic assets

    Used by UStarshatterAssetRegistrySubsystem during BootAssets() to:
      - Initialize registry cache
      - Validate required IDs
      - Load assets synchronously if requested

    This class contains NO loading logic. Runtime resolution is performed
    by the subsystem only.

=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/DataTable.h"
#include "UObject/SoftObjectPtr.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Texture2D.h"

#include "StarshatterAssetRegistrySettings.generated.h"

UCLASS(config = Game, defaultconfig, BlueprintType)
class STARSHATTERWARS_API UStarshatterAssetRegistrySettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    // ------------------------------------------------------------------
    // Project Settings placement
    // ------------------------------------------------------------------
    virtual FName GetCategoryName() const override
    {
        return TEXT("Starshatter Wars");
    }

    virtual FName GetSectionName() const override
    {
        return TEXT("Asset Registry");
    }

    virtual FText GetSectionText() const override
    {
        return FText::FromString("Asset Registry");
    }

    virtual FText GetSectionDescription() const override
    {
        return FText::FromString(
            "Centralized asset bindings used during boot to validate and load required assets."
        );
    }

    // ------------------------------------------------------------------
    // Registry bindings
    // ------------------------------------------------------------------

    // Map of AssetId -> Soft reference
    // Examples:
    //   Data.CampaignTable     -> /Game/Game/DT_Campaign.DT_Campaign
    //   Data.WeaponDesignTable -> /Game/Game/DT_WeaponDesign.DT_WeaponDesign
    //   UI.MainMenu            -> /Game/Screens/WB_MainMenu.WB_MainMenu_C
    UPROPERTY(EditAnywhere, config, Category = "Starshatter|AssetRegistry")
    TMap<FName, TSoftObjectPtr<UObject>> Assets;

    // Optional: hard fail when missing required assets (recommended true)
    UPROPERTY(EditAnywhere, config, Category = "Starshatter|AssetRegistry")
    bool bFailOnMissingRequired = true;

    // ------------------------------------------------------------------
    // Optional data table  bindings (config-backed)
    // ------------------------------------------------------------------
    UPROPERTY(EditAnywhere, config, Category = "Starshatter|DataTables")
    TSoftObjectPtr<UDataTable> WeaponDesignTable;

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|DataTables")
    TSoftObjectPtr<UDataTable> ShipDesignTable;

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|DataTables")
    TSoftObjectPtr<UDataTable> SystemDesignTable;

    // ------------------------------------------------------------------
    // Optional run time table  bindings (config-backed)
    // ------------------------------------------------------------------
    UPROPERTY(EditAnywhere, config, Category = "Starshatter|DataTables")
    TSoftObjectPtr<UDataTable> CampaignTable;

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|DataTables")
    TSoftObjectPtr<UDataTable> CampaignActionTable;

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|DataTables")
    TSoftObjectPtr<UDataTable> CampaignOOBTable;

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|DataTables")
    TSoftObjectPtr<UDataTable> CombatGroupTable;

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|DataTables")
    TSoftObjectPtr<UDataTable> GalaxyMapTable;

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|DataTables")
    TSoftObjectPtr<UDataTable> OrderOfBattleTable;

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|DataTables")
    TSoftObjectPtr<UDataTable> MedalsTable;

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|DataTables")
    TSoftObjectPtr<UDataTable> RanksTable;

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|DataTables")
    TSoftObjectPtr<UDataTable> RegionsDataTable;

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|DataTables")
    TSoftObjectPtr<UDataTable> ZonesDataTable;

    // ------------------------------------------------------------------
    // Optional UI bindings (config-backed)
    // ------------------------------------------------------------------

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|UI", meta = (AllowAbstract = "false"))
    TSoftClassPtr<UUserWidget> MenuScreenClass;

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|UI", meta = (AllowAbstract = "false"))
    TSoftClassPtr<UUserWidget> CampaignSelectScreenClass;

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|UI", meta = (AllowAbstract = "false"))
    TSoftClassPtr<UUserWidget> MissionSelectScreenClass;
    
    UPROPERTY(EditAnywhere, config, Category = "Starshatter|UI", meta = (AllowAbstract = "false"))
    TSoftClassPtr<UUserWidget> OperationsScreenClass;

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|UI", meta = (AllowAbstract = "false"))
    TSoftClassPtr<UUserWidget> MissionScreenClass;


    UPROPERTY(EditAnywhere, config, Category = "Starshatter|UI", meta = (AllowAbstract = "false"))
    TSoftClassPtr<UUserWidget> PlayerLogbookScreenClass;

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|UI", meta = (AllowAbstract = "false"))
    TSoftClassPtr<UUserWidget> OptionsScreenClass;

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|UI", meta = (AllowAbstract = "false"))
    TSoftClassPtr<UUserWidget> FirstTimeScreenClass;

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|UI", meta = (AllowAbstract = "false"))
    TSoftClassPtr<UUserWidget> ExitDlgClass;

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|UI", meta = (AllowAbstract = "false"))
    TSoftClassPtr<UUserWidget> TacRefScreenClass;

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|UI", meta = (AllowAbstract = "false"))
    TSoftClassPtr<UUserWidget> CmdMessageDialogClass;

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|UI", meta = (AllowAbstract = "false"))
    TSoftClassPtr<UUserWidget> CampaignLoadScreenClass;

    // ------------------------------------------------------------------
// Optional UI Theme bindings (config-backed)
// ------------------------------------------------------------------

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|UI Theme")
    TSoftObjectPtr<UTexture2D> MenuButton_Normal;

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|UI Theme")
    TSoftObjectPtr<UTexture2D> MenuButton_Hover;

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|UI Theme")
    TSoftObjectPtr<UTexture2D> MenuButton_Pressed;

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|UI Theme")
    TSoftObjectPtr<UTexture2D> MenuButton_Disabled;
};
