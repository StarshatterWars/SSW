/*
    Project Starshatter Wars
    Fractal Dev Studios

    SUBSYSTEM:    Mission UI
    FILE:         MissionUIStyle.cpp

    OVERVIEW
    ========
    Shared visual style helpers for mission screens.
*/

#include "MissionUIStyle.h"
#include "UObject/UObjectGlobals.h"

namespace MissionUIStyle
{
    // -----------------------------------------------------------------
    // Colors
    // -----------------------------------------------------------------

    const FLinearColor HeaderBG = FLinearColor(0.18f, 0.20f, 0.24f, 1.0f);
    const FLinearColor HeaderTopLine = FLinearColor(0.35f, 0.40f, 0.50f, 1.0f);
    const FLinearColor HeaderBottomLine = FLinearColor(0.05f, 0.05f, 0.05f, 1.0f);
    const FLinearColor HeaderText = FLinearColor(0.85f, 0.87f, 0.90f, 1.0f);

    const FLinearColor PanelBG = FLinearColor(0.06f, 0.07f, 0.08f, 0.95f);

    const FLinearColor RowBG = FLinearColor(0.05f, 0.05f, 0.06f, 1.0f);
    const FLinearColor RowText = FLinearColor(0.90f, 0.92f, 0.95f, 1.0f);
    const FLinearColor RowSelected = FLinearColor(0.65f, 0.65f, 0.35f, 0.35f);

    const FLinearColor InfoLabelText = FLinearColor(0.80f, 0.82f, 0.85f, 1.0f);
    const FLinearColor InfoValueText = FLinearColor(0.92f, 0.94f, 0.97f, 1.0f);

    // -----------------------------------------------------------------
    // Font path
    // -----------------------------------------------------------------

    const TCHAR* GetSerpentineFontPath()
    {
        // Update this path if your actual asset name/path differs.
        return TEXT("/Game/Font/SERPNTB_Font.SERPNTB_Font");
    }

    // -----------------------------------------------------------------
    // Font helpers
    // -----------------------------------------------------------------

    FSlateFontInfo GetSerpentineFont(int32 Size)
    {
        static UObject* CachedFontObject = nullptr;

        if (!CachedFontObject)
        {
            CachedFontObject = LoadObject<UObject>(nullptr, GetSerpentineFontPath());
        }

        FSlateFontInfo FontInfo;
        FontInfo.FontObject = CachedFontObject;
        FontInfo.Size = Size;
        FontInfo.TypefaceFontName = FName(TEXT("Default"));

        return FontInfo;
    }

    FSlateFontInfo GetHeaderFont(int32 Size)
    {
        return GetSerpentineFont(Size);
    }

    FSlateFontInfo GetRowFont(int32 Size)
    {
        return GetSerpentineFont(Size);
    }

    FSlateFontInfo GetInfoLabelFont(int32 Size)
    {
        return GetSerpentineFont(Size);
    }

    FSlateFontInfo GetInfoValueFont(int32 Size)
    {
        return GetSerpentineFont(Size);
    }
}