/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    StarshatterWars
    FILE:         MenuHistory.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    MenuHistory stack (non-owning)
*/

#pragma once

#include "CoreMinimal.h"

class Menu;

// ---------------------------------------------------------------------
// MenuHistory (non-owning)
// ---------------------------------------------------------------------
class STARSHATTERWARS_API MenuHistory
{
public:
    static const char* TYPENAME() { return "MenuHistory"; }

    MenuHistory() = default;
    virtual ~MenuHistory()
    {
        History.Reset(); // does NOT delete menus
    }

    Menu* GetCurrent() const
    {
        return History.Num() > 0 ? History.Last() : nullptr;
    }

    Menu* GetLevel(int32 N) const
    {
        return History.IsValidIndex(N) ? History[N] : nullptr;
    }

    Menu* Find(const FString& InTitle) const;

    void Pop()
    {
        if (History.Num() > 0)
            History.Pop(false);
    }

    void Push(Menu* InMenu)
    {
        if (InMenu)
            History.Add(InMenu);
    }

    void Clear()
    {
        History.Reset();
    }

private:
    TArray<Menu*> History; // non-owning
};

