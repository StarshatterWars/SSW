/*=============================================================================
    Project:        Starshatter Wars (nGenEx Unreal Port)
    Studio:         Fractal Dev Games
    Copyright:      (C) 2024–2026. All Rights Reserved.

    SUBSYSTEM:      StarshatterDataRuntimeSubsystem
    FILE:           StarshatterDataRuntimeSubsystem.cpp
    AUTHOR:         Carlos Bott
=============================================================================*/

#include "StarshatterDataRuntimeSubsystem.h"

#include "Engine/GameInstance.h"
#include "Logging/LogMacros.h"

// Reads DataTables / cached arrays from here (ownership stays in GameData):
#include "StarshatterGameDataSubsystem.h"

// Player selection comes from here:
#include "StarshatterPlayerSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogStarshatterRuntime, Log, All);

// ------------------------------------------------------------------
// UE lifecycle
// ------------------------------------------------------------------

void UStarshatterDataRuntimeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    SelectedCampaignRowName = NAME_None;
    SelectedCampaignIndex1Based = 0;
    SelectedCampaignDisplayName.Empty();

    ActiveCampaign = FS_Campaign{};
    bHasActiveCampaign = false;
}

void UStarshatterDataRuntimeSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

// ------------------------------------------------------------------
// Selection metadata
// ------------------------------------------------------------------

void UStarshatterDataRuntimeSubsystem::SetSelectedCampaignRowName(FName InRowName)
{
    SelectedCampaignRowName = InRowName;
}

void UStarshatterDataRuntimeSubsystem::SetSelectedCampaignIndex1Based(int32 InIndex1Based)
{
    SelectedCampaignIndex1Based = InIndex1Based;
}

void UStarshatterDataRuntimeSubsystem::SetSelectedCampaignDisplayName(const FString& InDisplayName)
{
    SelectedCampaignDisplayName = InDisplayName;
}

// ------------------------------------------------------------------
// Menu rule: is campaign set?
// ------------------------------------------------------------------

bool UStarshatterDataRuntimeSubsystem::IsCampaignSet() const
{
    return (!SelectedCampaignRowName.IsNone()) || (SelectedCampaignIndex1Based > 0);
}

// ------------------------------------------------------------------
// Internal helpers (DT-first)
// ------------------------------------------------------------------

static bool Runtime_TryGetCampaignByRowName_DTFirst(
    const UStarshatterGameDataSubsystem* GameData,
    FName RowName,
    FS_Campaign& OutCampaign)
{
    if (!GameData || RowName.IsNone())
        return false;

    UDataTable* DT = GameData->GetCampaignDataTable();
    if (!DT)
        return false;

    static const FString Context(TEXT("Runtime_TryGetCampaignByRowName_DTFirst"));
    if (const FS_Campaign* Row = DT->FindRow<FS_Campaign>(RowName, Context, /*bWarnIfRowMissing*/ false))
    {
        OutCampaign = *Row;
        return true;
    }

    return false;
}

static bool Runtime_TryGetCampaignByIndex1Based_ArrayThenDT(
    const UStarshatterGameDataSubsystem* GameData,
    int32 CampaignIndex1Based,
    FS_Campaign& OutCampaign)
{
    if (!GameData || CampaignIndex1Based <= 0)
        return false;

    const int32 ZeroBased = CampaignIndex1Based - 1;

    // Preferred: stable array ordering you control
    const TArray<FS_Campaign>& Arr = GameData->GetCampaignDataArray();
    if (Arr.IsValidIndex(ZeroBased))
    {
        OutCampaign = Arr[ZeroBased];
        return true;
    }

    // Fallback: DataTable row order (best effort, not guaranteed stable)
    UDataTable* DT = GameData->GetCampaignDataTable();
    if (DT)
    {
        const TArray<FName> RowNames = DT->GetRowNames(); // <-- correct signature in UE
        if (RowNames.IsValidIndex(ZeroBased))
        {
            return Runtime_TryGetCampaignByRowName_DTFirst(GameData, RowNames[ZeroBased], OutCampaign);
        }
    }

    UE_LOG(LogStarshatterRuntime, Warning,
        TEXT("[Runtime] Campaign index %d not found (CampaignDataArray.Num=%d)."),
        CampaignIndex1Based, Arr.Num());

    return false;
}

bool UStarshatterDataRuntimeSubsystem::ResolveCampaignInternal(
    const FS_PlayerGameInfo& PlayerInfoIn,
    FS_Campaign& OutCampaign) const
{
    UGameInstance* GI = GetGameInstance();
    if (!GI) return false;

    const UStarshatterGameDataSubsystem* GameData = GI->GetSubsystem<UStarshatterGameDataSubsystem>();
    if (!GameData) return false;

    // Prefer stable RowName:
    if (!PlayerInfoIn.CampaignRowName.IsNone())
    {
        if (Runtime_TryGetCampaignByRowName_DTFirst(GameData, PlayerInfoIn.CampaignRowName, OutCampaign))
            return true;
    }

    // Fallback to legacy 1-based index:
    if (PlayerInfoIn.Campaign > 0)
    {
        if (Runtime_TryGetCampaignByIndex1Based_ArrayThenDT(GameData, PlayerInfoIn.Campaign, OutCampaign))
            return true;
    }

    return false;
}

bool UStarshatterDataRuntimeSubsystem::ResolveCampaignFromSelectionInternal(FS_Campaign& OutCampaign) const
{
    UGameInstance* GI = GetGameInstance();
    if (!GI) return false;

    const UStarshatterGameDataSubsystem* GameData = GI->GetSubsystem<UStarshatterGameDataSubsystem>();
    if (!GameData) return false;

    // Prefer RowName:
    if (!SelectedCampaignRowName.IsNone())
        return Runtime_TryGetCampaignByRowName_DTFirst(GameData, SelectedCampaignRowName, OutCampaign);

    // Fallback index:
    if (SelectedCampaignIndex1Based > 0)
        return Runtime_TryGetCampaignByIndex1Based_ArrayThenDT(GameData, SelectedCampaignIndex1Based, OutCampaign);

    return false;
}

// ------------------------------------------------------------------
// Public resolution entry points
// ------------------------------------------------------------------

bool UStarshatterDataRuntimeSubsystem::ResolveAndSetActiveCampaignFromPlayer()
{
    UGameInstance* GI = GetGameInstance();
    if (!GI) return false;

    UStarshatterPlayerSubsystem* PlayerSS = GI->GetSubsystem<UStarshatterPlayerSubsystem>();
    if (!PlayerSS) return false;

    if (!PlayerSS->HasLoaded())
        PlayerSS->LoadPlayer();

    FS_Campaign Resolved{};
    if (!ResolveCampaignInternal(PlayerSS->GetPlayerInfo(), Resolved))
    {
        bHasActiveCampaign = false;
        ActiveCampaign = FS_Campaign{};
        return false;
    }

    ActiveCampaign = Resolved;
    bHasActiveCampaign = true;

    // Keep selection metadata consistent with loaded player:
    const FS_PlayerGameInfo& PlayerInfoRef = PlayerSS->GetPlayerInfo();

    if (!PlayerInfoRef.CampaignRowName.IsNone())
        SelectedCampaignRowName = PlayerInfoRef.CampaignRowName;

    if (PlayerInfoRef.Campaign > 0)
        SelectedCampaignIndex1Based = PlayerInfoRef.Campaign;

    OnRuntimeCampaignChanged.Broadcast();
    return true;
}

bool UStarshatterDataRuntimeSubsystem::ResolveAndSetActiveCampaignFromSelection()
{
    FS_Campaign Resolved{};
    if (!ResolveCampaignFromSelectionInternal(Resolved))
    {
        bHasActiveCampaign = false;
        ActiveCampaign = FS_Campaign{};
        return false;
    }

    ActiveCampaign = Resolved;
    bHasActiveCampaign = true;

    OnRuntimeCampaignChanged.Broadcast();
    return true;
}
