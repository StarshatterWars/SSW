/*=============================================================================
    Project:        Starshatter Wars (Unreal Engine Port)
    Studio:         Fractal Dev Studios
    Copyright:      (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:      Asset Registry
    FILE:           StarshatterAssetRegistrySubsystem.h
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    UStarshatterAssetRegistrySubsystem

    Centralized runtime asset resolver.

    Responsibilities:
    - Reads config-backed bindings from UStarshatterAssetRegistrySettings
    - Caches AssetId -> SoftObjectPath mappings at boot
    - Validates required assets during BootAssets()
    - Loads assets synchronously on demand (boot-safe)
    - Provides typed accessors for common asset classes

    DESIGN RULES
    ============
    - NO hard-coded paths
    - NO ConstructorHelpers
    - NO asset loads outside this subsystem
    - Boot subsystem controls ordering

=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/DataTable.h"
#include "Blueprint/UserWidget.h"

#include "StarshatterAssetRegistrySubsystem.generated.h"

class UTexture2D;

DECLARE_LOG_CATEGORY_EXTERN(LogStarshatterAssetRegistry, Log, All);

UCLASS()
class STARSHATTERWARS_API UStarshatterAssetRegistrySubsystem
    : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // ------------------------------------------------------------------
    // Subsystem lifecycle
    // ------------------------------------------------------------------
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ------------------------------------------------------------------
    // Boot entry
    // ------------------------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "Starshatter|Assets")
    bool InitRegistry();

    UFUNCTION(BlueprintPure, Category = "Starshatter|Assets")
    bool IsReady() const { return bReady; }

    // ------------------------------------------------------------------
    // Validation
    // ------------------------------------------------------------------
    bool ValidateRequired(const TArray<FName>& RequiredIds, bool bLoadNow);

    // ------------------------------------------------------------------
    // Generic access
    // ------------------------------------------------------------------
    UObject* GetAsset(FName AssetId, bool bLoadNow = true);

    // ------------------------------------------------------------------
    // Typed helpers
    // ------------------------------------------------------------------
    UDataTable* GetDataTable(FName AssetId, bool bLoadNow = true);

    TSubclassOf<UUserWidget> GetWidgetClass(FName AssetId, bool bLoadNow = true);

    UTexture2D* GetTexture2D(FName AssetId, bool bLoadNow = true);

private:
    // ------------------------------------------------------------------
    // Internal helpers
    // ------------------------------------------------------------------
    UObject* ResolveSoftObject(const TSoftObjectPtr<UObject>& SoftPtr, FName AssetId, bool bLoadNow);

private:
    // ------------------------------------------------------------------
    // Cached registry
    // ------------------------------------------------------------------
    TMap<FName, TSoftObjectPtr<UObject>> Cache;

    bool bReady = false;
};
