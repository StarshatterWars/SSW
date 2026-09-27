/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026.

    SUBSYSTEM:    nGenEx.lib (ported to Unreal)
    FILE:         SystemFont.h
    AUTHOR:       Carlos Bott
    ORIGINAL:     John DiCamillo / Destroyer Studios LLC (1997-2004)

    OVERVIEW
    ========
    SystemFont
    - Unreal-backed font state wrapper (UFont + size + tint + flags)
    - Maintains legacy-style method names used across the port
    - DOES NOT RENDER. Window/Video layer is responsible for drawing.
    - Provides UE helpers for Slate/UMG migration (FSlateFontInfo + color)
*/

#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"

#ifdef _WIN32
#ifdef DrawText
#undef DrawText
#endif
#ifdef DrawTextEx
#undef DrawTextEx
#endif
#endif

class UFont;

// Forward declarations (keep header light):
class Video;
class Bitmap;
class Text;

// NOTE:
// Rect MUST be a real type name (struct/class) for forward declare to work.
// If Rect is a typedef/using alias, include the header that defines it instead of forward-declaring.
struct Rect;

// +--------------------------------------------------------------------+
// SystemFont
// +--------------------------------------------------------------------+

class SystemFont
{
public:
    static const char* TYPENAME() { return "SystemFont"; }

    enum FLAGS
    {
        FONT_FIXED_PITCH = 1,
        FONT_ALL_CAPS = 2,
        FONT_NO_KERN = 4
    };

public:
    SystemFont();
    explicit SystemFont(const char* InName);
    ~SystemFont();

    // Legacy-style load. Recommended usage is a UFont asset path:
    // "/Game/UI/Fonts/MyFont.MyFont"
    // For legacy names (non asset paths), this will accept the name but may not load a font.
    bool Load(const char* InName);

    // Legacy compatibility (renderer delegation if provided; otherwise no-op):
    int DrawString(const char* text, int len, int x, int y, const Rect& clip, Video* video) const;
    int DrawTextW(const wchar_t* text, int len, int x, int y, const Rect& clip, Video* video) const;

    // No renderer: SystemFont does not render (no-op by design):
    int DrawString(const char* text, int len, int x, int y, const Rect& clip) const;
    int DrawTextW(const wchar_t* text, int len, int x, int y, const Rect& clip) const;

    // Block-style text APIs (ANSI/UTF-8 only; wide block-text not provided here):
    int DrawText(const char* text, int len, const Rect& clip, uint32 flags) const;
    int DrawText(const Text& text, int len, const Rect& clip, uint32 flags) const;

    int DrawText(const char* text, int len, const Rect& clip, uint32 flags, Bitmap* target) const;
    int DrawText(const Text& text, int len, const Rect& clip, uint32 flags, Bitmap* target) const;

    // Basic state:
    void   SetUFont(UFont* InFont);
    UFont* GetUFont() const;

    void   SetPointSize(int32 InSize);
    int32  GetPointSize() const;

    uint16 GetFlags() const;
    void   SetFlags(uint16 InFlags);

    FColor GetColor() const;
    void   SetColor(const FColor& InColor);

    double GetExpansion() const;
    void   SetExpansion(double InExpansion);

    double GetAlpha() const;
    void   SetAlpha(double InAlpha);

    int    GetBlend() const;
    void   SetBlend(int InBlend);

    // Useful for UE/Slate drawing:
    FLinearColor   GetLinearColor() const;
    FSlateFontInfo MakeSlateFontInfo() const;

    // One-call payload for Slate rendering migration:
    void GetSlateDrawParams(FSlateFontInfo& OutFont, FLinearColor& OutColor) const;

    const char* GetName() const;

private:
    void UpdateHeuristics();
    bool LooksLikeAssetPath(const FString& Str) const;

private:
    char  Name[64];

    uint16 Flags;
    float  Expansion;
    float  Alpha;
    int    Blend;

    int32  PointSize;
    FColor Color;

    // Unreal backing:
    TWeakObjectPtr<UFont> UnrealFont;
};
