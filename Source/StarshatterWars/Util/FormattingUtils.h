/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026.
    All Rights Reserved.

    SUBSYSTEM:    StarshatterWars (Unreal Engine)
    FILE:         FormattingUtils.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    FormattingUtils
    - Central place for display/formatting helpers (enum display names, prefixes, empire names, classification names).
    - Keeps USSWGameInstance lean (no generic helper clutter).
    - Safe to call from C++ anywhere.
    - Also exposed to Blueprints via UBlueprintFunctionLibrary where possible.

    NOTES
    =====
    - EnumToDisplayString<TEnum>() is a header-only template (not a UFUNCTION).
    - Blueprint-callable wrappers exist for your common UENUM types.
    - CLASSIFICATION enum should live in GameStructs.h as requested.
*/

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "GameStructs.h" // ECOMBATGROUP_TYPE, ECOMBATUNIT_TYPE, EEMPIRE_NAME, CLASSIFICATION
#include "Color.h"
#include "CombatUnit.h"
#include "MissionElement.h"
#include "GameStructs_System.h"
#include "FormattingUtils.generated.h"

// -----------------------------------------------------------------------------
// Header-only generic enum display helper (C++ only)
// -----------------------------------------------------------------------------
template <typename TEnum>
FORCEINLINE FString EnumToDisplayString(TEnum EnumValue)
{
    static_assert(TIsEnum<TEnum>::Value, "EnumToDisplayString only works with enums.");

    UEnum* EnumPtr = StaticEnum<TEnum>();
    if (!EnumPtr)
    {
        return TEXT("Invalid");
    }

    return EnumPtr->GetDisplayNameTextByValue(static_cast<int64>(EnumValue)).ToString();
}


// -----------------------------------------------------------------------------
// FormattingUtils (Blueprint + C++ static utility)
// -----------------------------------------------------------------------------
UCLASS()
class STARSHATTERWARS_API UFormattingUtils : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // -------------------------------------------------------------------------
    // Enum display name wrappers (Blueprint-friendly)
    // -------------------------------------------------------------------------
    UFUNCTION(BlueprintPure, Category = "Formatting")
    static FString GetGroupTypeDisplayName(ECOMBATGROUP_TYPE Type);

    UFUNCTION(BlueprintPure, Category = "Formatting")
    static FString GetUnitTypeDisplayName(ECOMBATUNIT_TYPE Type);

    // 1st/2nd/3rd/4th...
    UFUNCTION(BlueprintPure, Category = "Formatting")
    static FString GetOrdinal(int32 Id);

    // -------------------------------------------------------------------------
    // Empire helpers
    // -------------------------------------------------------------------------
    UFUNCTION(BlueprintPure, Category = "Formatting|Empire")
    static EEMPIRE_NAME GetEmpireTypeFromIndex(int32 Index);

    UFUNCTION(BlueprintPure, Category = "Formatting|Empire")
    static int32 GetIndexFromEmpireType(EEMPIRE_NAME Type);

    UFUNCTION(BlueprintPure, Category = "Formatting|Empire")
    static FString GetEmpireTypeNameByIndex(int32 Index);

    UFUNCTION(BlueprintPure, Category = "Formatting|Empire")
    static FString GetEmpireDisplayName(EEMPIRE_NAME Empire);

    UFUNCTION(BlueprintPure, Category = "Formatting|Empire")
    static FString GetEmpireNameFromType(EEMPIRE_NAME Empire);

    // -------------------------------------------------------------------------
    // Unit naming helpers
    // -------------------------------------------------------------------------
    UFUNCTION(BlueprintPure, Category = "Formatting|Units")
    static FString GetUnitPrefixFromType(ECOMBATUNIT_TYPE Type);

    // -------------------------------------------------------------------------
    // Legacy design class helpers (string <-> bitmask)
    // -------------------------------------------------------------------------
    // Get bitmask (1 << index) for a class name ("Fighter", "Cruiser", etc.)
    static int32 GetDesignClassFromName(const char* ClassName);

    // Get class name from a single-bit mask (1 << index)
    static const char* GetDesignClassName(int32 Mask);

    // Display a multi-bit mask as text: "Fighter | Cruiser | Carrier"
    UFUNCTION(BlueprintPure, Category = "Formatting|Design")
    static FString GetDesignClassMaskDisplayString(int32 Mask);

    // -------------------------------------------------------------------------
    // CLASSIFICATION helpers (enum lives in GameStructs.h)
    // -------------------------------------------------------------------------
    // Single value -> name ("Fighter", "Starbase", etc.)
    UFUNCTION(BlueprintPure, Category = "Formatting|Classification")
    static FString GetClassificationName(CLASSIFICATION Value);

    // Multi-bit mask -> display string ("Fighter | Cruiser | Carrier")
    UFUNCTION(BlueprintPure, Category = "Formatting|Classification")
    static FString GetClassificationMaskDisplayString(int32 Mask);

    static FString SanitizeEnumToken(const FString& In);

    static bool GetRegionTypeFromString(const FString& InString, EOrbitalType& OutValue);

    static FString FormatDateFromUnixSeconds(int64 UnixSeconds);

    static EShipEmpire GetShipEmpireFromName(const char* InName);

    static EShipEmpire GetShipEmpireFromString(const FString& InString);

    static FString FormatTimeHMS(double Seconds);
    
    static uint8 ToByteClamp(double v);

    static FColor Vec3ToColor255(const FVector& a);

    static FColor ToFColor(const Color& c);

    static FString FormatTPlus(uint64 TPlusSeconds);

    static int32 ParseStarshatterTime(const FString& InTime);

    static void FormatDayTime(char* Out, int32 TimeValue);
    
    static int32 ParseDayTimeString(const FString& In);

    static FString EmpireToString(EEMPIRE_NAME Empire);

    static FString GetUnitDesignIndicator(CombatUnit* Unit);

    static FString GetMissionElementIndicator(MissionElement* Elem);

    static float RadiansToDegrees360(double Radians);
};

