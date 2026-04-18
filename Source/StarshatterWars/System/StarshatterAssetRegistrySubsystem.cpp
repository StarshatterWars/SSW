/*=============================================================================
    Project:        Starshatter Wars (Unreal Engine Port)
    Studio:         Fractal Dev Studios
    Copyright:      (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:      Asset Registry
    FILE:           StarshatterAssetRegistrySubsystem.cpp
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    UStarshatterAssetRegistrySubsystem

    Centralized runtime asset binding and resolution layer.

    Responsibilities:
      - Read config-backed bindings from UStarshatterAssetRegistrySettings
      - Cache AssetId -> Soft reference mappings in memory
      - Validate required assets during BootAssets()
      - Resolve assets on demand (optional synchronous load)

    NOTES
    =====
    - No hard-coded paths outside of the settings layer.
    - Widget references should use generated class paths (..._C) so they load as UClass.
    - This subsystem owns no gameplay state. It is a pure lookup/resolution service.

=============================================================================*/

#include "StarshatterAssetRegistrySubsystem.h"

#include "StarshatterAssetRegistrySettings.h"

#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Blueprint/UserWidget.h"

DEFINE_LOG_CATEGORY(LogStarshatterAssetRegistry);

void UStarshatterAssetRegistrySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    Cache.Reset();
    bReady = false;

    InitRegistry();
    UE_LOG(LogStarshatterAssetRegistry, Log, TEXT("[ASSETS] Initialize()"));
}

void UStarshatterAssetRegistrySubsystem::Deinitialize()
{
    UE_LOG(LogStarshatterAssetRegistry, Log, TEXT("[ASSETS] Deinitialize()"));

    Cache.Reset();
    bReady = false;

    Super::Deinitialize();
}

UObject* UStarshatterAssetRegistrySubsystem::GetAsset(FName AssetId, bool bLoadNow)
{
    if (!bReady)
    {
        UE_LOG(LogStarshatterAssetRegistry, Warning,
            TEXT("[ASSETS] GetAsset: Registry not ready (%s) - attempting InitRegistry()"),
            *AssetId.ToString());

        if (!InitRegistry())
        {
            UE_LOG(LogStarshatterAssetRegistry, Error,
                TEXT("[ASSETS] GetAsset: InitRegistry failed; cannot resolve %s"),
                *AssetId.ToString());
            return nullptr;
        }
    }

    const TSoftObjectPtr<UObject>* Found = Cache.Find(AssetId);
    if (!Found)
    {
        UE_LOG(LogStarshatterAssetRegistry, Error,
            TEXT("[ASSETS] GetAsset: Missing AssetId=%s"),
            *AssetId.ToString());
        return nullptr;
    }

    return ResolveSoftObject(*Found, AssetId, bLoadNow);
}

bool UStarshatterAssetRegistrySubsystem::InitRegistry()
{
    Cache.Reset();
    bReady = false;

    const UStarshatterAssetRegistrySettings* Settings = GetDefault<UStarshatterAssetRegistrySettings>();
    if (!Settings)
    {
        UE_LOG(LogStarshatterAssetRegistry, Error, TEXT("[ASSETS] InitRegistry: Settings missing"));
        return false;
    }

    // 1) Copy generic map (AssetId -> SoftObject)
    Cache = Settings->Assets;

    // ------------------------------------------------------------------
    // 2) Inject typed design tables (do not overwrite explicit map entries)
    // ------------------------------------------------------------------

    // Data.WeaponDesignTable
    if (!Cache.Contains(TEXT("Data.WeaponDesignTable")))
    {
        if (!Settings->WeaponDesignTable.IsNull())
        {
            const FSoftObjectPath Path = Settings->WeaponDesignTable.ToSoftObjectPath();
            Cache.Add(TEXT("Data.WeaponDesignTable"), TSoftObjectPtr<UObject>(Path));

            UE_LOG(LogStarshatterAssetRegistry, Log, TEXT("[ASSETS] Bind Data.WeaponDesignTable -> %s"),
                *Path.ToString());
        }
        else
        {
            UE_LOG(LogStarshatterAssetRegistry, Warning,
                TEXT("[ASSETS] WeaponDesignTable is not set in Project Settings"));
        }
    }

    // Data.ShipDesignTable
    if (!Cache.Contains(TEXT("Data.ShipDesignTable")))
    {
        if (!Settings->ShipDesignTable.IsNull())
        {
            const FSoftObjectPath Path = Settings->ShipDesignTable.ToSoftObjectPath();
            Cache.Add(TEXT("Data.ShipDesignTable"), TSoftObjectPtr<UObject>(Path));

            UE_LOG(LogStarshatterAssetRegistry, Log, TEXT("[ASSETS] Bind Data.ShipDesignTable -> %s"),
                *Path.ToString());
        }
        else
        {
            UE_LOG(LogStarshatterAssetRegistry, Warning,
                TEXT("[ASSETS] ShipDesignTable is not set in Project Settings"));
        }
    }

    // Data.SystemDesignTable
    if (!Cache.Contains(TEXT("Data.SystemDesignTable")))
    {
        if (!Settings->SystemDesignTable.IsNull())
        {
            const FSoftObjectPath Path = Settings->SystemDesignTable.ToSoftObjectPath();
            Cache.Add(TEXT("Data.SystemDesignTable"), TSoftObjectPtr<UObject>(Path));

            UE_LOG(LogStarshatterAssetRegistry, Log, TEXT("[ASSETS] Bind Data.SystemDesignTable -> %s"),
                *Path.ToString());
        }
        else
        {
            UE_LOG(LogStarshatterAssetRegistry, Warning,
                TEXT("[ASSETS] SystemDesignTable is not set in Project Settings"));
        }
    }

    // Data.CampaignTable
    if (!Cache.Contains(TEXT("Data.CampaignTable")))
    {
        if (!Settings->CampaignTable.IsNull())
        {
            const FSoftObjectPath Path = Settings->CampaignTable.ToSoftObjectPath();
            Cache.Add(TEXT("Data.CampaignTable"), TSoftObjectPtr<UObject>(Path));
            UE_LOG(LogStarshatterAssetRegistry, Log, TEXT("[ASSETS] Bind Data.CampaignTable -> %s"), *Path.ToString());
        }
        else
        {
            UE_LOG(LogStarshatterAssetRegistry, Warning, TEXT("[ASSETS] CampaignTable is not set in Project Settings"));
        }
    }

    // Data.CampaignActionTable
    if (!Cache.Contains(TEXT("Data.CampaignActionTable")))
    {
        if (!Settings->CampaignActionTable.IsNull())
        {
            const FSoftObjectPath Path = Settings->CampaignActionTable.ToSoftObjectPath();
            Cache.Add(TEXT("Data.CampaignActionTable"), TSoftObjectPtr<UObject>(Path));
            UE_LOG(LogStarshatterAssetRegistry, Log, TEXT("[ASSETS] Bind Data.CampaignActionTable -> %s"), *Path.ToString());
        }
        else
        {
            UE_LOG(LogStarshatterAssetRegistry, Warning, TEXT("[ASSETS] CampaignActionTable is not set in Project Settings"));
        }
    }
    // Data.CampaignOOBTable
    if (!Cache.Contains(TEXT("Data.CampaignOOBTable")))
    {
        if (!Settings->CampaignOOBTable.IsNull())
        {
            const FSoftObjectPath Path = Settings->CampaignOOBTable.ToSoftObjectPath();
            Cache.Add(TEXT("Data.CampaignOOBTable"), TSoftObjectPtr<UObject>(Path));
            UE_LOG(LogStarshatterAssetRegistry, Log, TEXT("[ASSETS] Bind Data.CampaignOOBTable -> %s"), *Path.ToString());
        }
        else
        {
            UE_LOG(LogStarshatterAssetRegistry, Warning, TEXT("[ASSETS] CampaignOOBTable is not set in Project Settings"));
        }
    }

    // Data.CombatGroupTable
    if (!Cache.Contains(TEXT("Data.CombatGroupTable")))
    {
        if (!Settings->CombatGroupTable.IsNull())
        {
            const FSoftObjectPath Path = Settings->CombatGroupTable.ToSoftObjectPath();
            Cache.Add(TEXT("Data.CombatGroupTable"), TSoftObjectPtr<UObject>(Path));
            UE_LOG(LogStarshatterAssetRegistry, Log, TEXT("[ASSETS] Bind Data.CombatGroupTable -> %s"), *Path.ToString());
        }
        else
        {
            UE_LOG(LogStarshatterAssetRegistry, Warning, TEXT("[ASSETS] CombatGroupTable is not set in Project Settings"));
        }
    }

    // Data.GalaxyMapTable
    if (!Cache.Contains(TEXT("Data.GalaxyMapTable")))
    {
        if (!Settings->GalaxyMapTable.IsNull())
        {
            const FSoftObjectPath Path = Settings->GalaxyMapTable.ToSoftObjectPath();
            Cache.Add(TEXT("Data.GalaxyMapTable"), TSoftObjectPtr<UObject>(Path));
            UE_LOG(LogStarshatterAssetRegistry, Log, TEXT("[ASSETS] Bind Data.GalaxyMapTable -> %s"), *Path.ToString());
        }
        else
        {
            UE_LOG(LogStarshatterAssetRegistry, Warning, TEXT("[ASSETS] GalaxyMapTable is not set in Project Settings"));
        }
    }

    // Data.SystemMapTable
    if (!Cache.Contains(TEXT("Data.SystemMapTable")))
    {
        if (!Settings->SystemMapTable.IsNull())
        {
            const FSoftObjectPath Path = Settings->SystemMapTable.ToSoftObjectPath();
            Cache.Add(TEXT("Data.SystemMapTable"), TSoftObjectPtr<UObject>(Path));
            UE_LOG(LogStarshatterAssetRegistry, Log, TEXT("[ASSETS] Bind Data.SystemMapTable -> %s"), *Path.ToString());
        }
        else
        {
            UE_LOG(LogStarshatterAssetRegistry, Warning, TEXT("[ASSETS] SystemMapTable is not set in Project Settings"));
        }
    }

    // Data.OrderOfBattleTable
    if (!Cache.Contains(TEXT("Data.OrderOfBattleTable")))
    {
        if (!Settings->OrderOfBattleTable.IsNull())
        {
            const FSoftObjectPath Path = Settings->OrderOfBattleTable.ToSoftObjectPath();
            Cache.Add(TEXT("Data.OrderOfBattleTable"), TSoftObjectPtr<UObject>(Path));
            UE_LOG(LogStarshatterAssetRegistry, Log, TEXT("[ASSETS] Bind Data.OrderOfBattleTable -> %s"), *Path.ToString());
        }
        else
        {
            UE_LOG(LogStarshatterAssetRegistry, Warning, TEXT("[ASSETS] OrderOfBattleTable is not set in Project Settings"));
        }
    }

    // Data.AwardsTable
    if (!Cache.Contains(TEXT("Data.MedalsTable")))
    {
        if (!Settings->MedalsTable.IsNull())
        {
            const FSoftObjectPath Path = Settings->MedalsTable.ToSoftObjectPath();
            Cache.Add(TEXT("Data.MedalsTable"), TSoftObjectPtr<UObject>(Path));
            UE_LOG(LogStarshatterAssetRegistry, Log, TEXT("[ASSETS] Bind Data.MedalsTable -> %s"), *Path.ToString());
        }
        else
        {
            UE_LOG(LogStarshatterAssetRegistry, Warning, TEXT("[ASSETS] MedalsTable is not set in Project Settings"));
        }
    }

    // Data.AwardsTable
    if (!Cache.Contains(TEXT("Data.RanksTable")))
    {
        if (!Settings->RanksTable.IsNull())
        {
            const FSoftObjectPath Path = Settings->RanksTable.ToSoftObjectPath();
            Cache.Add(TEXT("Data.RanksTable"), TSoftObjectPtr<UObject>(Path));
            UE_LOG(LogStarshatterAssetRegistry, Log, TEXT("[ASSETS] Bind Data.RanksTable -> %s"), *Path.ToString());
        }
        else
        {
            UE_LOG(LogStarshatterAssetRegistry, Warning, TEXT("[ASSETS] RanksTable is not set in Project Settings"));
        }
    }

    // Data.RegionsTable
    if (!Cache.Contains(TEXT("Data.RegionsTable")))
    {
        if (!Settings->RegionsDataTable.IsNull())
        {
            const FSoftObjectPath Path = Settings->RegionsDataTable.ToSoftObjectPath();
            Cache.Add(TEXT("Data.RegionsTable"), TSoftObjectPtr<UObject>(Path));

            UE_LOG(LogStarshatterAssetRegistry, Log,
                TEXT("[ASSETS] Bind Data.RegionsTable -> %s"),
                *Path.ToString());
        }
        else
        {
            UE_LOG(LogStarshatterAssetRegistry, Warning,
                TEXT("[ASSETS] RegionsDataTable is not set in Project Settings"));
        }
    }

    // Data.ZonesTable
    if (!Cache.Contains(TEXT("Data.ZonesTable")))
    {
        if (!Settings->ZonesDataTable.IsNull())
        {
            const FSoftObjectPath Path = Settings->ZonesDataTable.ToSoftObjectPath();
            Cache.Add(TEXT("Data.ZonesTable"), TSoftObjectPtr<UObject>(Path));

            UE_LOG(LogStarshatterAssetRegistry, Log,
                TEXT("[ASSETS] Bind Data.ZonesTable -> %s"),
                *Path.ToString());
        }
        else
        {
            UE_LOG(LogStarshatterAssetRegistry, Warning,
                TEXT("[ASSETS] ZonesDataTable is not set in Project Settings"));
        }
    }

    // ------------------------------------------------------------------
    // 3) Inject typed UI widget classes (do not overwrite explicit map entries)
    //    NOTE: We store them in the generic Cache as UObject soft refs so the
    //    existing GetWidgetClass() path works unchanged.
    // ------------------------------------------------------------------

    // UI.MenuScreenClass
    if (!Cache.Contains(TEXT("UI.MenuScreenClass")))
    {
        if (!Settings->MenuScreenClass.IsNull())
        {
            const FSoftObjectPath Path = Settings->MenuScreenClass.ToSoftObjectPath();
            Cache.Add(TEXT("UI.MenuScreenClass"), TSoftObjectPtr<UObject>(Path));

            UE_LOG(LogStarshatterAssetRegistry, Log, TEXT("[ASSETS] Bind UI.MenuScreenClass -> %s"),
                *Path.ToString());
        }
        else
        {
            UE_LOG(LogStarshatterAssetRegistry, Warning,
                TEXT("[ASSETS] MenuScreenClass is not set in Project Settings"));
        }
    }

    // UI.MenuScreenClass
    if (!Cache.Contains(TEXT("UI.CampaignScreenClass")))
    {
        if (!Settings->CampaignScreenClass.IsNull())
        {
            const FSoftObjectPath Path = Settings->CampaignScreenClass.ToSoftObjectPath();
            Cache.Add(TEXT("UI.CampaignScreenClass"), TSoftObjectPtr<UObject>(Path));

            UE_LOG(LogStarshatterAssetRegistry, Log, TEXT("[ASSETS] Bind UI.CampaignScreenClass -> %s"),
                *Path.ToString());
        }
        else
        {
            UE_LOG(LogStarshatterAssetRegistry, Warning,
                TEXT("[ASSETS] CampaignScreenClass is not set in Project Settings"));
        }
    }

    // UI.MenuScreenClass
    if (!Cache.Contains(TEXT("UI.CampaignSceneClass")))
    {
        if (!Settings->CampaignSceneClass.IsNull())
        {
            const FSoftObjectPath Path = Settings->CampaignSceneClass.ToSoftObjectPath();
            Cache.Add(TEXT("UI.CampaignSceneClass"), TSoftObjectPtr<UObject>(Path));

            UE_LOG(LogStarshatterAssetRegistry, Log, TEXT("[ASSETS] Bind UI.CampaignScreenClass -> %s"),
                *Path.ToString());
        }
        else
        {
            UE_LOG(LogStarshatterAssetRegistry, Warning,
                TEXT("[ASSETS] CampaignScreenClass is not set in Project Settings"));
        }
    }

    // UI.CampaignSelectScreenClass
    if (!Cache.Contains(TEXT("UI.CampaignSelectScreenClass")))
    {
        if (!Settings->CampaignSelectScreenClass.IsNull())
        {
            const FSoftObjectPath Path = Settings->CampaignSelectScreenClass.ToSoftObjectPath();
            Cache.Add(TEXT("UI.CampaignSelectScreenClass"), TSoftObjectPtr<UObject>(Path));

            UE_LOG(LogStarshatterAssetRegistry, Log, TEXT("[ASSETS] Bind UI.CampaignSelectScreenClass -> %s"),
                *Path.ToString());
        }
        else
        {
            UE_LOG(LogStarshatterAssetRegistry, Warning,
                TEXT("[ASSETS] CampaignSelectScreenClass is not set in Project Settings"));
        }
    }

    // UI.CampaignSelectScreenClass
    if (!Cache.Contains(TEXT("UI.OperationsScreenClass")))
    {
        if (!Settings->OperationsScreenClass.IsNull())
        {
            const FSoftObjectPath Path = Settings->OperationsScreenClass.ToSoftObjectPath();
            Cache.Add(TEXT("UI.OperationsScreenClass"), TSoftObjectPtr<UObject>(Path));

            UE_LOG(LogStarshatterAssetRegistry, Log, TEXT("[ASSETS] Bind UI.OperationsScreenClass -> %s"),
                *Path.ToString());
        }
        else
        {
            UE_LOG(LogStarshatterAssetRegistry, Warning,
                TEXT("[ASSETS] MissionSelectScreenClass is not set in Project Settings"));
        }
    }

    // UI.CampaignSelectScreenClass
    if (!Cache.Contains(TEXT("UI.MissionScreenClass")))
    {
        if (!Settings->MissionScreenClass.IsNull())
        {
            const FSoftObjectPath Path = Settings->MissionScreenClass.ToSoftObjectPath();
            Cache.Add(TEXT("UI.MissionScreenClass"), TSoftObjectPtr<UObject>(Path));

            UE_LOG(LogStarshatterAssetRegistry, Log, TEXT("[ASSETS] Bind UI.MissionScreenClass -> %s"),
                *Path.ToString());
        }
        else
        {
            UE_LOG(LogStarshatterAssetRegistry, Warning,
                TEXT("[ASSETS] MissionScreenClass is not set in Project Settings"));
        }
    }
    // UI.CampaignSelectScreenClass
    if (!Cache.Contains(TEXT("UI.MissionSelectScreenClass")))
    {
        if (!Settings->MissionSelectScreenClass.IsNull())
        {
            const FSoftObjectPath Path = Settings->MissionSelectScreenClass.ToSoftObjectPath();
            Cache.Add(TEXT("UI.MissionSelectScreenClass"), TSoftObjectPtr<UObject>(Path));

            UE_LOG(LogStarshatterAssetRegistry, Log, TEXT("[ASSETS] Bind UI.MissionSelectScreenClass -> %s"),
                *Path.ToString());
        }
        else
        {
            UE_LOG(LogStarshatterAssetRegistry, Warning,
                TEXT("[ASSETS] MissionSelectScreenClass is not set in Project Settings"));
        }
    }

    // UI.OptionsScreenClass
    if (!Cache.Contains(TEXT("UI.OptionsScreenClass")))
    {
        if (!Settings->OptionsScreenClass.IsNull())
        {
            const FSoftObjectPath Path = Settings->OptionsScreenClass.ToSoftObjectPath();
            Cache.Add(TEXT("UI.OptionsScreenClass"), TSoftObjectPtr<UObject>(Path));

            UE_LOG(LogStarshatterAssetRegistry, Log, TEXT("[ASSETS] Bind UI.OptionsScreenClass -> %s"),
                *Path.ToString());
        }
        else
        {
            UE_LOG(LogStarshatterAssetRegistry, Warning,
                TEXT("[ASSETS] OptionsScreenClass is not set in Project Settings"));
        }
    }

    // UI.MenuScreenClass
    if (!Cache.Contains(TEXT("UI.PlayerLogbookScreenClass")))
    {
        if (!Settings->PlayerLogbookScreenClass.IsNull())
        {
            const FSoftObjectPath Path = Settings->PlayerLogbookScreenClass.ToSoftObjectPath();
            Cache.Add(TEXT("UI.PlayerLogbookScreenClass"), TSoftObjectPtr<UObject>(Path));

            UE_LOG(LogStarshatterAssetRegistry, Log, TEXT("[ASSETS] Bind UI.PlayerLogbookScreenClass -> %s"),
                *Path.ToString());
        }
        else
        {
            UE_LOG(LogStarshatterAssetRegistry, Warning,
                TEXT("[ASSETS] PlayerLogbookScreenClass is not set in Project Settings"));
        }
    }
    // UI.FirstRunDlgClass (property name: FirstRunScreenClass)
    if (!Cache.Contains(TEXT("UI.FirstTimeDlgClass")))
    {
        if (!Settings->FirstTimeScreenClass.IsNull())
        {
            const FSoftObjectPath Path = Settings->FirstTimeScreenClass.ToSoftObjectPath();
            Cache.Add(TEXT("UI.FirstTimeDlgClass"), TSoftObjectPtr<UObject>(Path));

            UE_LOG(LogStarshatterAssetRegistry, Log, TEXT("[ASSETS] Bind UI.FirstTimeDlgClass -> %s"),
                *Path.ToString());
        }
        else
        {
            UE_LOG(LogStarshatterAssetRegistry, Warning,
                TEXT("[ASSETS] FirstTimeScreenClass is not set in Project Settings"));
        }
    }

    // UI.ExitDlgClass
    if (!Cache.Contains(TEXT("UI.ExitDlgClass")))
    {
        if (!Settings->ExitDlgClass.IsNull())
        {
            const FSoftObjectPath Path = Settings->ExitDlgClass.ToSoftObjectPath();
            Cache.Add(TEXT("UI.ExitDlgClass"), TSoftObjectPtr<UObject>(Path));

            UE_LOG(LogStarshatterAssetRegistry, Log, TEXT("[ASSETS] Bind UI.ExitDlgClass -> %s"),
                *Path.ToString());
        }
        else
        {
            UE_LOG(LogStarshatterAssetRegistry, Warning,
                TEXT("[ASSETS] ExitDlgClass is not set in Project Settings"));
        }
    }

    // UI.MenuScreenClass
    if (!Cache.Contains(TEXT("UI.TacRefScreenClass")))
    {
        if (!Settings->TacRefScreenClass.IsNull())
        {
            const FSoftObjectPath Path = Settings->TacRefScreenClass.ToSoftObjectPath();
            Cache.Add(TEXT("UI.TacRefScreenClass"), TSoftObjectPtr<UObject>(Path));

            UE_LOG(LogStarshatterAssetRegistry, Log, TEXT("[ASSETS] Bind UI.TacRefScreenClass -> %s"),
                *Path.ToString());
        }
        else
        {
            UE_LOG(LogStarshatterAssetRegistry, Warning,
                TEXT("[ASSETS] TacRefScreenClass is not set in Project Settings"));
        }
    }

    // UI.MenuScreenClass
    if (!Cache.Contains(TEXT("UI.CmdMessageDlgClass")))
    {
        if (!Settings->CmdMessageDialogClass.IsNull())
        {
            const FSoftObjectPath Path = Settings->CmdMessageDialogClass.ToSoftObjectPath();
            Cache.Add(TEXT("UI.CmdMessageDlgClass"), TSoftObjectPtr<UObject>(Path));

            UE_LOG(LogStarshatterAssetRegistry, Log, TEXT("[ASSETS] Bind UI.CmdMessageDlgClass -> %s"),
                *Path.ToString());
        }
        else
        {
            UE_LOG(LogStarshatterAssetRegistry, Warning,
                TEXT("[ASSETS] CmdMessageDlgClass is not set in Project Settings"));
        }
    }

    // UI.OptionsScreenClass
    if (!Cache.Contains(TEXT("UI.CampaignLoadClass")))
    {
        if (!Settings->CampaignLoadScreenClass.IsNull())
        {
            const FSoftObjectPath Path = Settings->CampaignLoadScreenClass.ToSoftObjectPath();
            Cache.Add(TEXT("UI.CampaignLoadClass"), TSoftObjectPtr<UObject>(Path));

            UE_LOG(LogStarshatterAssetRegistry, Log, TEXT("[ASSETS] Bind UI.CampaignLoadClass -> %s"),
                *Path.ToString());
        }
        else
        {
            UE_LOG(LogStarshatterAssetRegistry, Warning,
                TEXT("[ASSETS] CampaignLoadClass is not set in Project Settings"));
        }
    }

    // ------------------------------------------------------------------
    // 4) Inject typed UI Theme assets (do not overwrite explicit map entries)
    // ------------------------------------------------------------------

    if (!Cache.Contains(TEXT("UI.Theme.MenuButton.Normal")))
    {
        if (!Settings->MenuButton_Normal.IsNull())
        {
            const FSoftObjectPath Path = Settings->MenuButton_Normal.ToSoftObjectPath();
            Cache.Add(TEXT("UI.Theme.MenuButton.Normal"), TSoftObjectPtr<UObject>(Path));
            UE_LOG(LogStarshatterAssetRegistry, Log, TEXT("[ASSETS] Bind UI.Theme.MenuButton.Normal -> %s"), *Path.ToString());
        }
        else
        {
            UE_LOG(LogStarshatterAssetRegistry, Warning, TEXT("[ASSETS] MenuButton_Normal is not set in Project Settings"));
        }
    }

    if (!Cache.Contains(TEXT("UI.Theme.MenuButton.Hover")))
    {
        if (!Settings->MenuButton_Hover.IsNull())
        {
            const FSoftObjectPath Path = Settings->MenuButton_Hover.ToSoftObjectPath();
            Cache.Add(TEXT("UI.Theme.MenuButton.Hover"), TSoftObjectPtr<UObject>(Path));
            UE_LOG(LogStarshatterAssetRegistry, Log, TEXT("[ASSETS] Bind UI.Theme.MenuButton.Hover -> %s"), *Path.ToString());
        }
        else
        {
            UE_LOG(LogStarshatterAssetRegistry, Warning, TEXT("[ASSETS] MenuButton_Hover is not set in Project Settings"));
        }
    }

    if (!Cache.Contains(TEXT("UI.Theme.MenuButton.Pressed")))
    {
        if (!Settings->MenuButton_Pressed.IsNull())
        {
            const FSoftObjectPath Path = Settings->MenuButton_Pressed.ToSoftObjectPath();
            Cache.Add(TEXT("UI.Theme.MenuButton.Pressed"), TSoftObjectPtr<UObject>(Path));
            UE_LOG(LogStarshatterAssetRegistry, Log, TEXT("[ASSETS] Bind UI.Theme.MenuButton.Pressed -> %s"), *Path.ToString());
        }
        else
        {
            UE_LOG(LogStarshatterAssetRegistry, Warning, TEXT("[ASSETS] MenuButton_Pressed is not set in Project Settings"));
        }
    }

    if (!Cache.Contains(TEXT("UI.Theme.MenuButton.Disabled")))
    {
        if (!Settings->MenuButton_Disabled.IsNull())
        {
            const FSoftObjectPath Path = Settings->MenuButton_Disabled.ToSoftObjectPath();
            Cache.Add(TEXT("UI.Theme.MenuButton.Disabled"), TSoftObjectPtr<UObject>(Path));
            UE_LOG(LogStarshatterAssetRegistry, Log, TEXT("[ASSETS] Bind UI.Theme.MenuButton.Disabled -> %s"), *Path.ToString());
        }
        else
        {
            UE_LOG(LogStarshatterAssetRegistry, Warning, TEXT("[ASSETS] MenuButton_Disabled is not set in Project Settings"));
        }
    }

    UE_LOG(LogStarshatterAssetRegistry, Log, TEXT("[ASSETS] InitRegistry: Loaded %d entries"), Cache.Num());

    bReady = true;
    return true;
}

bool UStarshatterAssetRegistrySubsystem::ValidateRequired(const TArray<FName>& RequiredIds, bool bLoadNow)
{
    if (!bReady)
    {
        UE_LOG(LogStarshatterAssetRegistry, Error, TEXT("[ASSETS] ValidateRequired: Registry not ready"));
        return false;
    }

    const UStarshatterAssetRegistrySettings* Settings = GetDefault<UStarshatterAssetRegistrySettings>();
    const bool bFailHard = Settings ? Settings->bFailOnMissingRequired : true;

    bool bAllOk = true;

    for (const FName& Id : RequiredIds)
    {
        const TSoftObjectPtr<UObject>* Found = Cache.Find(Id);
        if (!Found)
        {
            UE_LOG(LogStarshatterAssetRegistry, Error, TEXT("[ASSETS] Missing required AssetId: %s"), *Id.ToString());
            bAllOk = false;
            if (bFailHard)
                return false;
            continue;
        }

        if (bLoadNow)
        {
            UObject* Obj = ResolveSoftObject(*Found, Id, true);
            if (!Obj)
            {
                UE_LOG(LogStarshatterAssetRegistry, Error, TEXT("[ASSETS] Failed to load required AssetId: %s"), *Id.ToString());
                bAllOk = false;
                if (bFailHard)
                    return false;
            }
        }
    }

    return bAllOk;
}

UObject* UStarshatterAssetRegistrySubsystem::ResolveSoftObject(const TSoftObjectPtr<UObject>& SoftPtr, FName AssetId, bool bLoadNow)
{
    if (SoftPtr.IsNull())
    {
        UE_LOG(LogStarshatterAssetRegistry, Error, TEXT("[ASSETS] ResolveSoftObject: Null soft reference for %s"),
            *AssetId.ToString());
        return nullptr;
    }

    if (bLoadNow)
    {
        UObject* Loaded = SoftPtr.LoadSynchronous();
        if (!Loaded)
        {
            UE_LOG(LogStarshatterAssetRegistry, Error, TEXT("[ASSETS] ResolveSoftObject: Load failed for %s (%s)"),
                *AssetId.ToString(), *SoftPtr.ToSoftObjectPath().ToString());
        }
        return Loaded;
    }

    return SoftPtr.Get();
}

UDataTable* UStarshatterAssetRegistrySubsystem::GetDataTable(FName AssetId, bool bLoadNow)
{
    return Cast<UDataTable>(GetAsset(AssetId, bLoadNow));
}

TSubclassOf<UUserWidget> UStarshatterAssetRegistrySubsystem::GetWidgetClass(FName AssetId, bool bLoadNow)
{
    UObject* Obj = GetAsset(AssetId, bLoadNow);
    if (!Obj)
        return nullptr;

    UClass* AsClass = Cast<UClass>(Obj);
    if (!AsClass)
    {
        UE_LOG(LogStarshatterAssetRegistry, Error,
            TEXT("[ASSETS] GetWidgetClass: %s resolved to object '%s' of type '%s'. Bind the generated class (..._C)."),
            *AssetId.ToString(),
            *GetNameSafe(Obj),
            Obj ? *Obj->GetClass()->GetName() : TEXT("NULL"));
        return nullptr;
    }

    if (!AsClass->IsChildOf(UUserWidget::StaticClass()))
    {
        UE_LOG(LogStarshatterAssetRegistry, Error,
            TEXT("[ASSETS] GetWidgetClass: %s is not a UUserWidget subclass (%s)"),
            *AssetId.ToString(), *AsClass->GetName());
        return nullptr;
    }

    return AsClass;
}

UTexture2D* UStarshatterAssetRegistrySubsystem::GetTexture2D(FName AssetId, bool bLoadNow)
{
    return Cast<UTexture2D>(GetAsset(AssetId, bLoadNow));
}
