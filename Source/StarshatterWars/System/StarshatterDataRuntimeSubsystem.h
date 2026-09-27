/*=============================================================================
    Project:        Starshatter Wars (nGenEx Unreal Port)
    Studio:         Fractal Dev Games
    Copyright:      (C) 2024–2026. All Rights Reserved.

    SUBSYSTEM:      StarshatterDataRuntimeSubsystem
    FILE:           StarshatterDataRuntimeSubsystem.h / .cpp
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    Session-scoped runtime state container for game data.

    This subsystem owns "mutable runtime state" that used to live in:
      - SSWGameInstance (selection flags, active campaign selection)
      - StarshatterGameDataSubsystem (ActiveCampaign as mutable)

    Responsibilities:
      - Track active campaign selection (RowName + 1-based index + display name)
      - Resolve active campaign from GameData (DT-first)
      - Cache the resolved FS_Campaign for fast access
      - Track whether a campaign is "set" (for menu button enable/disable rules)
      - Hold other runtime selections (system/sector/etc.) as you migrate them

    Non-goals:
      - No parsing, no file IO
      - No DataTable generation
      - No SaveGame ownership (player save stays in PlayerSubsystem)
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameStructs.h"
#include "StarshatterDataRuntimeSubsystem.generated.h"

class UStarshatterGameDataSubsystem;
class UStarshatterPlayerSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRuntimeCampaignChanged);

UCLASS()
class STARSHATTERWARS_API UStarshatterDataRuntimeSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // ------------------------------------------------------------------
    // Subsystem lifecycle
    // ------------------------------------------------------------------
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ------------------------------------------------------------------
    // Campaign selection (authoritative selection metadata)
    // ------------------------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "Starshatter|Runtime")
    void SetSelectedCampaignRowName(FName InRowName);

    UFUNCTION(BlueprintCallable, Category = "Starshatter|Runtime")
    void SetSelectedCampaignIndex1Based(int32 InIndex1Based);

    UFUNCTION(BlueprintCallable, Category = "Starshatter|Runtime")
    void SetSelectedCampaignDisplayName(const FString& InDisplayName);

    UFUNCTION(BlueprintPure, Category = "Starshatter|Runtime")
    FName GetSelectedCampaignRowName() const { return SelectedCampaignRowName; }

    UFUNCTION(BlueprintPure, Category = "Starshatter|Runtime")
    int32 GetSelectedCampaignIndex1Based() const { return SelectedCampaignIndex1Based; }

    UFUNCTION(BlueprintPure, Category = "Starshatter|Runtime")
    FString GetSelectedCampaignDisplayName() const { return SelectedCampaignDisplayName; }

    // ------------------------------------------------------------------
    // Active campaign (resolved FS_Campaign cached here)
    // ------------------------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "Starshatter|Runtime")
    bool ResolveAndSetActiveCampaignFromPlayer();

    UFUNCTION(BlueprintCallable, Category = "Starshatter|Runtime")
    bool ResolveAndSetActiveCampaignFromSelection();

    UFUNCTION(BlueprintPure, Category = "Starshatter|Runtime")
    bool HasActiveCampaign() const { return bHasActiveCampaign; }

    // Returns cached active campaign (if not set, returns default struct)
    UFUNCTION(BlueprintPure, Category = "Starshatter|Runtime")
    FS_Campaign GetActiveCampaignCopy() const { return ActiveCampaign; }

    // C++ fast path
    const FS_Campaign& GetActiveCampaignRef() const { return ActiveCampaign; }

    // ------------------------------------------------------------------
    // “Campaign set” rules for menus
    // ------------------------------------------------------------------
    UFUNCTION(BlueprintPure, Category = "Starshatter|Runtime")
    bool IsCampaignSet() const;

    // ------------------------------------------------------------------
    // Events
    // ------------------------------------------------------------------
    UPROPERTY(BlueprintAssignable, Category = "Starshatter|Runtime")
    FOnRuntimeCampaignChanged OnRuntimeCampaignChanged;

private:
    // Player-based resolution (RowName preferred, index fallback)
    bool ResolveCampaignInternal(const FS_PlayerGameInfo& PlayerInfoIn, FS_Campaign& OutCampaign) const;

    // Selection-based resolution (RowName preferred, index fallback)
    bool ResolveCampaignFromSelectionInternal(FS_Campaign& OutCampaign) const;

private:
    // Selection metadata (what UI sets)
    UPROPERTY()
    FName SelectedCampaignRowName = NAME_None;

    UPROPERTY()
    int32 SelectedCampaignIndex1Based = 0;

    UPROPERTY()
    FString SelectedCampaignDisplayName;

    // Cached resolved campaign (what gameplay reads)
    UPROPERTY()
    FS_Campaign ActiveCampaign;

    UPROPERTY()
    bool bHasActiveCampaign = false;
};