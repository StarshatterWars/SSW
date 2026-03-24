/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    StarshatterWars
    FILE:         MenuHistory.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    MenuHistory stack (non-owning)
*/

#include "MenuHistory.h"
#include "Menu.h"   // must define Menu interface

// ---------------------------------------------------------------------
// Find menu by title (searches from most-recent to oldest)
// ---------------------------------------------------------------------
Menu* MenuHistory::Find(const FString& InTitle) const
{
    if (InTitle.IsEmpty())
        return nullptr;

    // Search from top of stack downward (UI-expected behavior)
    for (int32 i = History.Num() - 1; i >= 0; --i)
    {
        Menu* MenuPtr = History[i];
        if (!MenuPtr)
            continue;

        // ASSUMPTION:
        // Menu exposes title as FString.
        // Change GetTitle() if your API differs.
        if (MenuPtr->GetTitle().Equals(InTitle, ESearchCase::IgnoreCase))
        {
            return MenuPtr;
        }
    }

    return nullptr;
}
