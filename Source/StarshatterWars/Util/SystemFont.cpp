/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026.

    SUBSYSTEM:    nGenEx.lib (ported to Unreal)
    FILE:         SystemFont.cpp
    AUTHOR:       Carlos Bott
    ORIGINAL:     John DiCamillo / Destroyer Studios LLC (1997-2004)
*/

#include "SystemFont.h"

#include "Engine/Font.h"
#include "Styling/CoreStyle.h"
#include "Logging/LogMacros.h"

// If you actually USE Text methods in the .cpp later, include its real definition:
// #include "Text.h"

SystemFont::SystemFont()
    : Flags(0)
    , Expansion(0.0f)
    , Alpha(1.0f)
    , Blend(0)
    , PointSize(12)
    , Color(FColor::White)
    , UnrealFont(nullptr)
{
    FMemory::Memzero(Name, sizeof(Name));
}

SystemFont::SystemFont(const char* InName)
    : SystemFont()
{
    if (InName && InName[0])
    {
        FCStringAnsi::Strncpy(Name, InName, sizeof(Name));
        Load(Name);
    }
}

SystemFont::~SystemFont()
{
    // no heap allocations
}

// +--------------------------------------------------------------------+
// Helpers
// +--------------------------------------------------------------------+

bool SystemFont::LooksLikeAssetPath(const FString& Str) const
{
    return Str.StartsWith(TEXT("/Game/")) || Str.StartsWith(TEXT("/Engine/"));
}

// +--------------------------------------------------------------------+
// Load
// +--------------------------------------------------------------------+

bool SystemFont::Load(const char* InName)
{
    if (!InName || !InName[0])
        return false;

    FCStringAnsi::Strncpy(Name, InName, sizeof(Name));

    const FString Str = UTF8_TO_TCHAR(InName);

    // If this doesn't look like an Unreal asset path, accept as legacy name and continue.
    // This avoids failing hard when legacy code passes names like "Arial12".
    if (!LooksLikeAssetPath(Str))
    {
        UnrealFont = nullptr;
        UpdateHeuristics();
        return true;
    }

    UFont* Loaded = Cast<UFont>(StaticLoadObject(UFont::StaticClass(), nullptr, *Str));
    if (!Loaded)
    {
        UE_LOG(LogTemp, Warning, TEXT("SystemFont::Load failed. Expected UFont asset path. Got: %s"), *Str);
        UnrealFont = nullptr;
        return false;
    }

    UnrealFont = Loaded;
    UpdateHeuristics();
    return true;
}

// +--------------------------------------------------------------------+
// Basic State
// +--------------------------------------------------------------------+

void SystemFont::SetUFont(UFont* InFont)
{
    UnrealFont = InFont;
}

UFont* SystemFont::GetUFont() const
{
    return UnrealFont.Get();
}

void SystemFont::SetPointSize(int32 InSize)
{
    PointSize = FMath::Max(1, InSize);
    UpdateHeuristics();
}

int32 SystemFont::GetPointSize() const
{
    return PointSize;
}

uint16 SystemFont::GetFlags() const
{
    return Flags;
}

void SystemFont::SetFlags(uint16 InFlags)
{
    Flags = InFlags;
}

FColor SystemFont::GetColor() const
{
    return Color;
}

void SystemFont::SetColor(const FColor& InColor)
{
    Color = InColor;
}

double SystemFont::GetExpansion() const
{
    return (double)Expansion;
}

void SystemFont::SetExpansion(double InExpansion)
{
    Expansion = (float)InExpansion;
}

double SystemFont::GetAlpha() const
{
    return (double)Alpha;
}

void SystemFont::SetAlpha(double InAlpha)
{
    Alpha = (float)FMath::Clamp(InAlpha, 0.0, 1.0);
}

int SystemFont::GetBlend() const
{
    return Blend;
}

void SystemFont::SetBlend(int InBlend)
{
    Blend = InBlend;
}

// +--------------------------------------------------------------------+
// UE Helpers
// +--------------------------------------------------------------------+

FLinearColor SystemFont::GetLinearColor() const
{
    FColor C = Color;
    C.A = (uint8)FMath::Clamp(FMath::RoundToInt(Alpha * 255.0f), 0, 255);
    return FLinearColor(C);
}

FSlateFontInfo SystemFont::MakeSlateFontInfo() const
{
    if (UFont* UF = UnrealFont.Get())
    {
        // Basic Slate font construction (works for migration; later you may move to FontFace assets).
        return FSlateFontInfo(UF, PointSize);
    }

    return FCoreStyle::GetDefaultFontStyle("Regular", PointSize);
}

void SystemFont::GetSlateDrawParams(FSlateFontInfo& OutFont, FLinearColor& OutColor) const
{
    OutFont = MakeSlateFontInfo();
    OutColor = GetLinearColor();
}

const char* SystemFont::GetName() const
{
    return Name;
}

void SystemFont::UpdateHeuristics()
{
    // Hook for future baseline/height metrics if you decide to cache them.
}

// --------------------------------------------------------------------------------------
// Legacy draw calls
// NOTE: SystemFont does NOT render. These are compatibility stubs.
// If/when your Video layer exposes matching APIs, you can delegate here.
// --------------------------------------------------------------------------------------

int SystemFont::DrawString(const char* text, int len, int x, int y, const Rect& clip, Video* video) const
{
    // No-op by design (renderer owns drawing).
    return 0;
}

int SystemFont::DrawTextW(const wchar_t* text, int len, int x, int y, const Rect& clip, Video* video) const
{
    // No-op by design (renderer owns drawing).
    return 0;
}

int SystemFont::DrawString(const char* text, int len, int x, int y, const Rect& clip) const
{
    // No-op by design (renderer owns drawing).
    return 0;
}

int SystemFont::DrawTextW(const wchar_t* text, int len, int x, int y, const Rect& clip) const
{
    // No-op by design (renderer owns drawing).
    return 0;
}

int SystemFont::DrawText(const char* text, int len, const Rect& clip, uint32 flags) const
{
    // No-op by design (renderer owns drawing).
    return 0;
}

int SystemFont::DrawText(const Text& text, int len, const Rect& clip, uint32 flags) const
{
    // No-op by design (renderer owns drawing).
    // If you later want to support Text here, include "Text.h" and convert to UTF-8.
    return 0;
}

int SystemFont::DrawText(const char* text, int len, const Rect& clip, uint32 flags, Bitmap* target) const
{
    // No-op by design (renderer owns drawing).
    return 0;
}

int SystemFont::DrawText(const Text& text, int len, const Rect& clip, uint32 flags, Bitmap* target) const
{
    // No-op by design (renderer owns drawing).
    // If you later want to support Text here, include "Text.h" and convert to UTF-8.
    return 0;
}
