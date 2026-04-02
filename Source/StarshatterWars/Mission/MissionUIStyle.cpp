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
    const FLinearColor HeaderBG = FLinearColor(0.48f, 0.52f, 0.58f, 1.0f);
    const FLinearColor HeaderTopLine = FLinearColor(0.35f, 0.40f, 0.50f, 1.0f);
    const FLinearColor HeaderBottomLine = FLinearColor(0.05f, 0.05f, 0.05f, 1.0f);
    const FLinearColor HeaderText = FLinearColor(0.85f, 0.87f, 0.90f, 1.0f);

    const FLinearColor PanelBG = FLinearColor(0.06f, 0.07f, 0.08f, 0.95f);

    const FLinearColor RowBG = FLinearColor(0.04f, 0.04f, 0.05f, 1.0f);
    const FLinearColor RowText = FLinearColor::White;
    const FLinearColor RowSelected = FLinearColor(0.65f, 0.65f, 0.35f, 0.35f);

    const FLinearColor InfoLabelText = FLinearColor(0.80f, 0.82f, 0.85f, 1.0f);
    const FLinearColor InfoValueText = FLinearColor(0.92f, 0.94f, 0.97f, 1.0f);

    const FLinearColor RowSelectedBG = FLinearColor(0.15f, 0.25f, 0.45f, 1.f);

    const FLinearColor ComboBG = FLinearColor(0.07f, 0.08f, 0.10f, 1.0f);
    const FLinearColor ComboHoverBG = FLinearColor(0.14f, 0.18f, 0.24f, 1.0f);
    const FLinearColor ComboPressedBG = FLinearColor(0.35f, 0.35f, 0.18f, 1.0f);
    const FLinearColor ComboText = FLinearColor(0.95f, 0.95f, 0.95f, 1.0f);
    const FLinearColor ComboMenuBG = FLinearColor(0.05f, 0.06f, 0.08f, 1.0f);
    const FLinearColor ComboItemBG = FLinearColor(0.08f, 0.09f, 0.11f, 1.0f);
    const FLinearColor ComboItemHoverBG = FLinearColor(0.18f, 0.22f, 0.30f, 1.0f);
    const FLinearColor ComboItemSelectedBG = FLinearColor(0.42f, 0.42f, 0.18f, 1.0f);

    const FLinearColor SSWHeaderBG = FLinearColor(0.08f, 0.10f, 0.14f, 1.0f);

    const TCHAR* GetSerpentineFontPath()
    {
        return TEXT("/Game/Font/SERPNTB_Font.SERPNTB_Font");
    }

    const TCHAR* GetLimerickFontPath()
    {
        return TEXT("/Game/Font/Limerick-Serial_Bold_Font.Limerick-Serial_Bold_Font");
    }

    static UObject* GetCachedFontObject(const TCHAR* Path)
    {
        static TMap<FString, UObject*> Cache;

        const FString Key(Path);

        if (UObject** Found = Cache.Find(Key))
        {
            return *Found;
        }

        UObject* FontObj = LoadObject<UObject>(nullptr, Path);

        if (FontObj)
        {
            Cache.Add(Key, FontObj);
        }

        return FontObj;
    }

    FSlateFontInfo GetSerpentineFont(int32 Size)
    {
        FSlateFontInfo FontInfo;
        FontInfo.FontObject = GetCachedFontObject(GetSerpentineFontPath());
        FontInfo.Size = Size;
        FontInfo.TypefaceFontName = NAME_None;
        return FontInfo;
    }

    FSlateFontInfo GetLimerickFont(int32 Size)
    {
        FSlateFontInfo FontInfo;
        FontInfo.FontObject = GetCachedFontObject(GetLimerickFontPath());
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
        return GetLimerickFont(Size);
    }

    FSlateFontInfo GetInfoLabelFont(int32 Size)
    {
        return GetLimerickFont(Size);
    }

    FSlateFontInfo GetInfoValueFont(int32 Size)
    {
        return GetLimerickFont(Size);
    }

    FSlateFontInfo GetTableRowFont()
    {
        return GetLimerickFont(16);
    }
}