/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    StarshatterWars (Unreal Engine)
    FILE:         FontManagerSubsystem.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo / Destroyer Studios LLC
*/

#include "FontManagerSubsystem.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogFontManager, Log, All);

// +--------------------------------------------------------------------+
// Close Registry
// +--------------------------------------------------------------------+

void UFontManagerSubsystem::Close()
{
    Fonts.Reset();
}

// +--------------------------------------------------------------------+
// Register Font
// +--------------------------------------------------------------------+

void UFontManagerSubsystem::RegisterFont(FName Name, const FSlateFontInfo& FontInfo)
{
    if (Name.IsNone())
    {
        UE_LOG(LogFontManager, Warning,
            TEXT("FontManagerSubsystem::RegisterFont called with NAME_None"));
        return;
    }

    // Update existing entry if found:
    for (FFontItem& Item : Fonts)
    {
        if (Item.Name == Name)
        {
            Item.FontInfo = FontInfo;
            return;
        }
    }

    // Add new entry:
    FFontItem NewItem;
    NewItem.Name = Name;
    NewItem.FontInfo = FontInfo;

    Fonts.Add(MoveTemp(NewItem));
}

// +--------------------------------------------------------------------+
// Find Font
// +--------------------------------------------------------------------+

bool UFontManagerSubsystem::FindFont(FName Name, FSlateFontInfo& OutFontInfo) const
{
    if (Name.IsNone())
        return false;

    for (const FFontItem& Item : Fonts)
    {
        if (Item.Name == Name)
        {
            OutFontInfo = Item.FontInfo;
            return true;
        }
    }

    return false;
}
