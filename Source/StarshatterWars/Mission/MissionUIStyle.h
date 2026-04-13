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

    extern const FLinearColor SSWHeaderBG;
    
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

    extern const FLinearColor RowSelectedBG;

    extern const FLinearColor ComboBG;
    extern const FLinearColor ComboHoverBG;
    extern const FLinearColor ComboPressedBG;
    extern const FLinearColor ComboText;
    extern const FLinearColor ComboMenuBG;
    extern const FLinearColor ComboItemBG;
    extern const FLinearColor ComboItemHoverBG;
    extern const FLinearColor ComboItemSelectedBG;
    extern const FLinearColor DropdownHoverBG;

    // -----------------------------------------------------------------
    // Font paths
    // -----------------------------------------------------------------

    const TCHAR* GetSerpentineFontPath();
    const TCHAR* GetLimerickFontPath();

    // -----------------------------------------------------------------
    // Font helpers
    // -----------------------------------------------------------------

    FSlateFontInfo GetSerpentineFont(int32 Size);
    FSlateFontInfo GetLimerickFont(int32 Size);

    // -----------------------------------------------------------------
    // Standard UI roles
    // -----------------------------------------------------------------

    // Headers (Serpentine)
    FSlateFontInfo GetHeaderFont(int32 Size = 18);

    // Table rows (Limerick 16)
    FSlateFontInfo GetRowFont(int32 Size = 16);

    // Info panels (Limerick, smaller)
    FSlateFontInfo GetInfoLabelFont(int32 Size = 13);
    FSlateFontInfo GetInfoValueFont(int32 Size = 14);

    // Convenience
    FSlateFontInfo GetTableRowFont(); // always 16 Limerick


}