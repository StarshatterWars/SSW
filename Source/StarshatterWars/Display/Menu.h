/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    StarshatterWars
    FILE:         Menu.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Unreal-friendly Menu
    - Owns MenuItem allocations
    - Stores items in TArray<MenuItem*>
    - Parent pointers for back-navigation
*/

#pragma once

#include "CoreMinimal.h"
#include "MenuItem.h"

// ---------------------------------------------------------------------
// Menu
// ---------------------------------------------------------------------
class STARSHATTERWARS_API Menu
{
public:
    static const char* TYPENAME() { return "Menu"; }

    Menu() = default;
    explicit Menu(const FString& InTitle) : Title(InTitle) {}

    virtual ~Menu()
    {
        ClearItems();
    }

    // -----------------------------------------------------------------
    // Title
    // -----------------------------------------------------------------
    const FString& GetTitle() const { return Title; }
    void SetTitle(const FString& InTitle) { Title = InTitle; }

    // -----------------------------------------------------------------
    // Parent
    // -----------------------------------------------------------------
    Menu* GetParent() const { return Parent; }
    void  SetParent(Menu* InParent) { Parent = InParent; }

    // -----------------------------------------------------------------
    // Items
    // -----------------------------------------------------------------
    void AddItem(const FString& Label, uintptr_t Value = 0, bool bEnabled = true)
    {
        MenuItem* Item = new MenuItem(Label, Value, bEnabled);
        if (!Item)
            return;

        Item->MenuPtr = this;
        Items.Add(Item);
    }

    void AddMenu(const FString& Label, Menu* SubMenu, uintptr_t Value = 0)
    {
        MenuItem* Item = new MenuItem(Label, Value, true);
        if (!Item)
            return;

        Item->MenuPtr = this;
        Item->SubMenuPtr = SubMenu;

        if (SubMenu)
            SubMenu->Parent = this;

        Items.Add(Item);
    }

    MenuItem* GetItem(int32 Index) const
    {
        return Items.IsValidIndex(Index) ? Items[Index] : nullptr;
    }

    int32 NumItems() const
    {
        return Items.Num();
    }

    void ClearItems()
    {
        for (MenuItem* Item : Items) {
            delete Item;
        }
        Items.Reset();
    }

    const TArray<MenuItem*>& GetItems() const { return Items; }
    TArray<MenuItem*>& GetItems() { return Items; }

private:
    FString Title;
    TArray<MenuItem*> Items;
    Menu* Parent = nullptr;
};
