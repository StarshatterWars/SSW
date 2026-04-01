/*
    Project Starshatter Wars
    Fractal Dev Studios

    SUBSYSTEM:    Mission UI
    FILE:         MissionUIStyle.h

    OVERVIEW
    ========
    Shared visual style helpers for mission screens.
*/

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateTypes.h"

namespace MissionUIStyle
{
    // -----------------------------------------------------------------
    // Colors
    // -----------------------------------------------------------------

    extern const FLinearColor HeaderBG;
    extern const FLinearColor HeaderTopLine;
    extern const FLinearColor HeaderBottomLine;
    extern const FLinearColor HeaderText;

    extern const FLinearColor PanelBG;

    extern const FLinearColor RowBG;
    extern const FLinearColor RowText;
    extern const FLinearColor RowSelected;

    extern const FLinearColor InfoLabelText;
    extern const FLinearColor InfoValueText;

    // -----------------------------------------------------------------
    // Fonts
    // -----------------------------------------------------------------

    FSlateFontInfo GetSerpentineFont(int32 Size);
    FSlateFontInfo GetHeaderFont(int32 Size = 18);
    FSlateFontInfo GetRowFont(int32 Size = 16);
    FSlateFontInfo GetInfoLabelFont(int32 Size = 13);
    FSlateFontInfo GetInfoValueFont(int32 Size = 14);

    // -----------------------------------------------------------------
    // Font paths
    // -----------------------------------------------------------------

    const TCHAR* GetSerpentineFontPath();
}
