/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    nGenEx.lib (ported to Unreal)
    FILE:         FontManager.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo / Destroyer Studios LLC
*/

#include "FontManager.h"
#include "SystemFont.h"
#include "FontManagerSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogFontManager, Log, All);

TArray<FLegacyFontItem> FontManager::LegacyFonts;
TArray<TUniquePtr<SystemFont>> FontManager::OwnedFonts;

// +--------------------------------------------------------------------+
// Helper: get subsystem from context
// +--------------------------------------------------------------------+

static UFontManagerSubsystem* GetFontManagerSubsystem(UObject* WorldContext)
{
    if (!WorldContext)
        return nullptr;

    if (UGameInstance* GI = UGameplayStatics::GetGameInstance(WorldContext))
        return GI->GetSubsystem<UFontManagerSubsystem>();

    return nullptr;
}

// +--------------------------------------------------------------------+
// Helper: stable implicit UObject context (viewport)
// +--------------------------------------------------------------------+

static UObject* GetImplicitContext()
{
    if (GEngine && GEngine->GameViewport)
        return GEngine->GameViewport;

    return nullptr;
}

// +--------------------------------------------------------------------+
// Bootstrap table (edit here to define your standard font set)
// +--------------------------------------------------------------------+

struct FFontBootstrapDef
{
    const char* Key;        // Registry name: "HUD", "HUD_SMALL", etc.
    const char* AssetPath;  // UFont asset path: "/Game/UI/Fonts/MyFont.MyFont"
    int32       PointSize;
    uint16      Flags;
    FColor      Color;
};

// NOTE: Replace these with your real font assets and desired sizes.
static const FFontBootstrapDef GDefaultFonts[] =
{
    // Key         AssetPath                                 Size Flags Color
    { "HUD",       "/Game/UI/Fonts/Roboto.Roboto",            16,  0,    FColor::White },
    { "HUD_SMALL", "/Game/UI/Fonts/Roboto.Roboto",            12,  0,    FColor::White },
    { "HUD_BIG",   "/Game/UI/Fonts/Roboto.Roboto",            24,  0,    FColor::White },
    { "Limerick12", "/Game/UI/Fonts/Roboto.Roboto",           12,  0,    FColor::White },
    { "Limerick18", "/Game/UI/Fonts/Roboto.Roboto",           18,  0,    FColor::White },
    { "Verdana",    "/Game/UI/Fonts/Roboto.Roboto",           18,  0,    FColor::White },
    { "Terminal",   "/Game/UI/Fonts/Roboto.Roboto",           12,  0,    FColor::White },
    { "OCRB",       "/Game/UI/Fonts/Roboto.Roboto",           12,  0,    FColor::White },
    { "GUI",        "/Game/UI/Fonts/Roboto.Roboto",           18,  0,    FColor::White },
};

// +--------------------------------------------------------------------+
// RegisterAllFonts (explicit context)
// +--------------------------------------------------------------------+

void FontManager::RegisterAllFonts(UObject* WorldContext)
{
    // Clear previous bootstrap-owned fonts and legacy registry entries:
    LegacyFonts.Reset();
    OwnedFonts.Reset();

    UFontManagerSubsystem* Subsys = GetFontManagerSubsystem(WorldContext);
    if (!Subsys)
    {
        UE_LOG(LogFontManager, Warning,
            TEXT("FontManager::RegisterAllFonts - no GameInstance subsystem available (context invalid)."));
        // Still continue bootstrapping legacy fonts; subsystem is optional during transition.
    }

    const int32 Count = (int32)(sizeof(GDefaultFonts) / sizeof(GDefaultFonts[0]));

    for (int32 i = 0; i < Count; ++i)
    {
        const FFontBootstrapDef& Def = GDefaultFonts[i];

        if (!Def.Key || !*Def.Key || !Def.AssetPath || !*Def.AssetPath)
            continue;

        // Create and load a SystemFont (legacy-compatible):
        TUniquePtr<SystemFont> NewFont = MakeUnique<SystemFont>();
        if (!NewFont->Load(Def.AssetPath))
        {
            UE_LOG(LogFontManager, Warning,
                TEXT("FontManager::RegisterAllFonts - failed to load font asset: %s (key=%s)"),
                UTF8_TO_TCHAR(Def.AssetPath), UTF8_TO_TCHAR(Def.Key));
            continue;
        }

        NewFont->SetPointSize(Def.PointSize);
        NewFont->SetFlags(Def.Flags);
        NewFont->SetColor(Def.Color);

        // Keep a stable raw pointer for legacy callers:
        SystemFont* Raw = NewFont.Get();

        // Store in owned pool (FontManager owns this memory now):
        OwnedFonts.Add(MoveTemp(NewFont));

        // Register into legacy registry:
        FontManager::Register(Def.Key, Raw);

        // Register into UE subsystem (if available):
        if (Subsys)
        {
            const FSlateFontInfo SlateInfo = Raw->MakeSlateFontInfo();
            Subsys->RegisterFont(FName(UTF8_TO_TCHAR(Def.Key)), SlateInfo);
        }
    }

    UE_LOG(LogFontManager, Log, TEXT("FontManager::RegisterAllFonts bootstrapped %d fonts."), LegacyFonts.Num());
}

// +--------------------------------------------------------------------+
// RegisterAllFonts (implicit context)
// +--------------------------------------------------------------------+

void FontManager::RegisterAllFonts()
{
    if (UObject* Ctx = GetImplicitContext())
    {
        RegisterAllFonts(Ctx);
        return;
    }

    UE_LOG(LogFontManager, Warning,
        TEXT("FontManager::RegisterAllFonts (implicit) called before viewport exists. Use explicit context from GameInstance::Init."));
}

// ====================================================================
// Legacy API (preserved behavior)
// ====================================================================

void FontManager::Close()
{
    // Legacy behavior: clear registry.
    // NOTE: We also clear owned fonts because bootstrap now centralizes ownership.
    LegacyFonts.Reset();
    OwnedFonts.Reset();
}

void FontManager::Register(const char* NameAnsi, SystemFont* Font)
{
    if (!NameAnsi || !*NameAnsi || !Font)
    {
        UE_LOG(LogFontManager, Warning, TEXT("FontManager::Register(legacy) invalid parameters."));
        return;
    }

    const FString Name = UTF8_TO_TCHAR(NameAnsi);

    for (FLegacyFontItem& Item : LegacyFonts)
    {
        if (Item.Name.Equals(Name, ESearchCase::IgnoreCase))
        {
            Item.Font = Font;
            Item.Size = Font ? Font->GetPointSize() : 0;
            return;
        }
    }

    FLegacyFontItem NewItem;
    NewItem.Name = Name;
    NewItem.Font = Font;
    NewItem.Size = Font ? Font->GetPointSize() : 0;

    LegacyFonts.Add(MoveTemp(NewItem));
}

SystemFont* FontManager::Find(const char* NameAnsi)
{
    if (!NameAnsi || !*NameAnsi)
        return nullptr;

    const FString Name = UTF8_TO_TCHAR(NameAnsi);

    for (const FLegacyFontItem& Item : LegacyFonts)
    {
        if (Item.Font && Item.Name.Equals(Name, ESearchCase::IgnoreCase))
            return Item.Font;
    }

    return nullptr;
}

// ====================================================================
// UE API (explicit context)
// ====================================================================

void FontManager::Register(UObject* WorldContext, const char* NameAnsi, const FSlateFontInfo& FontInfo)
{
    if (!NameAnsi || !*NameAnsi)
        return;

    if (UFontManagerSubsystem* FM = GetFontManagerSubsystem(WorldContext))
        FM->RegisterFont(FName(UTF8_TO_TCHAR(NameAnsi)), FontInfo);
}

bool FontManager::Find(UObject* WorldContext, const char* NameAnsi, FSlateFontInfo& OutFontInfo)
{
    if (!NameAnsi || !*NameAnsi)
        return false;

    if (UFontManagerSubsystem* FM = GetFontManagerSubsystem(WorldContext))
        return FM->FindFont(FName(UTF8_TO_TCHAR(NameAnsi)), OutFontInfo);

    return false;
}

void FontManager::Close(UObject* WorldContext)
{
    if (UFontManagerSubsystem* FM = GetFontManagerSubsystem(WorldContext))
        FM->Close();
}

// ====================================================================
// UE API (implicit context)
// ====================================================================

void FontManager::Register(const char* NameAnsi, const FSlateFontInfo& FontInfo)
{
    if (UObject* Ctx = GetImplicitContext())
        Register(Ctx, NameAnsi, FontInfo);
}

bool FontManager::Find(const char* NameAnsi, FSlateFontInfo& OutFontInfo)
{
    if (UObject* Ctx = GetImplicitContext())
        return Find(Ctx, NameAnsi, OutFontInfo);

    return false;
}

void FontManager::CloseUE()
{
    if (UObject* Ctx = GetImplicitContext())
        Close(Ctx);
}
