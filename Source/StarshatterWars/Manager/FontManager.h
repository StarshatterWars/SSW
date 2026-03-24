/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    nGenEx.lib (ported to Unreal)
    FILE:         FontManager.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Font Resource Manager (dual-path)
    - Maintains legacy behavior:
        Close(), Register(const char*, SystemFont*), Find(const char*) -> SystemFont*
      using an internal static registry of SystemFont*.
    - Adds Unreal subsystem-backed registry for Slate/UMG:
        Register/Find/Close using FSlateFontInfo and UFontManagerSubsystem.
    - Adds RegisterAllFonts() bootstrap so fonts can be loaded once at init.
*/

#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"

// Forward declarations:
class SystemFont;

struct FLegacyFontItem
{
    FString     Name;
    int32       Size = 0;
    SystemFont* Font = nullptr;
};

// +--------------------------------------------------------------------+
// FontManager
// +--------------------------------------------------------------------+

class FontManager
{
public:
    static const char* TYPENAME() { return "FontManager"; }

    // ------------------------------------------------------------
    // Bootstrap API (call once from GameInstance::Init or similar):
    // ------------------------------------------------------------

    // Loads and registers all default fonts into BOTH registries:
    // - Legacy registry: Name -> SystemFont*
    // - UE subsystem:    Name -> FSlateFontInfo
    //
    // WorldContext should be GameInstance (preferred) or any UObject with GI access.
    static void RegisterAllFonts(UObject* WorldContext);

    // Convenience bootstrap for legacy callers (uses viewport context).
    // This will no-op if viewport isn't created yet.
    static void RegisterAllFonts();

    // ------------------------------------------------------------
    // Legacy API (must remain stable until full conversion):
    // ------------------------------------------------------------

    static void        Close();
    static void        Register(const char* NameAnsi, SystemFont* Font);
    static SystemFont* Find(const char* NameAnsi);

    // ------------------------------------------------------------
    // UE API (explicit context):
    // ------------------------------------------------------------

    static void Register(UObject* WorldContext, const char* NameAnsi, const FSlateFontInfo& FontInfo);
    static bool Find(UObject* WorldContext, const char* NameAnsi, FSlateFontInfo& OutFontInfo);
    static void Close(UObject* WorldContext);

    // ------------------------------------------------------------
    // UE API (implicit context):
    // ------------------------------------------------------------

    static void Register(const char* NameAnsi, const FSlateFontInfo& FontInfo);
    static bool Find(const char* NameAnsi, FSlateFontInfo& OutFontInfo);
    static void CloseUE();

private:
    // Legacy registry storage:
    static TArray<FLegacyFontItem> LegacyFonts;

    // FontManager-owned SystemFont instances created during bootstrap.
    // These back the legacy registry so old code can keep using SystemFont*.
    static TArray<TUniquePtr<SystemFont>> OwnedFonts;
};
