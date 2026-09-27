#pragma once
/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    StarshatterWars
    FILE:         MenuItem.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Unreal-friendly MenuItem
    - FString label
    - Pointer-sized payload (uintptr_t)
    - Selection supports Starshatter states (none/hover/locked)
*/

#pragma once

#include "CoreMinimal.h"

class Menu;

// ---------------------------------------------------------------------
// EMenuItemSelectState (legacy-compatible)
// ---------------------------------------------------------------------
enum class EMenuItemSelectState : uint8
{
    None = 0,   // not selected
    Hover = 1,   // highlighted
    Locked = 2    // submenu-open lock
};

// ---------------------------------------------------------------------
// MenuItem
// ---------------------------------------------------------------------
class STARSHATTERWARS_API MenuItem
{
public:
    static const char* TYPENAME() { return "MenuItem"; }

    MenuItem() = default;

    MenuItem(const FString& InLabel, uintptr_t InData = 0, bool bInEnabled = true)
        : Text(InLabel)
        , Data(InData)
        , bEnabled(bInEnabled)
        , SelectState(EMenuItemSelectState::None)
        , MenuPtr(nullptr)
        , SubMenuPtr(nullptr)
    {
    }

    ~MenuItem() = default;

    // -----------------------------------------------------------------
    // Text
    // -----------------------------------------------------------------
    const FString& GetText() const { return Text; }
    void SetText(const FString& InText) { Text = InText; }

    // -----------------------------------------------------------------
    // Data (opaque payload)
    // -----------------------------------------------------------------
    uintptr_t GetData() const { return Data; }
    void SetData(uintptr_t InData) { Data = InData; }

    // -----------------------------------------------------------------
    // Flags
    // -----------------------------------------------------------------
    bool IsEnabled() const { return bEnabled; }
    void SetEnabled(bool bInEnabled) { bEnabled = bInEnabled; }

    // -----------------------------------------------------------------
    // Selection (Starshatter-compatible)
    // -----------------------------------------------------------------
    EMenuItemSelectState GetSelected() const { return SelectState; }
    bool IsSelected() const { return SelectState != EMenuItemSelectState::None; }

    void SetSelected(EMenuItemSelectState InState) { SelectState = InState; }
    void ClearSelected() { SelectState = EMenuItemSelectState::None; }

    // Convenience for old call sites:
    void SetSelected(bool bInSelected)
    {
        SelectState = bInSelected ? EMenuItemSelectState::Hover : EMenuItemSelectState::None;
    }

    // If you still have legacy code passing ints (0/1/2):
    void SetSelected(int32 InLegacyState)
    {
        switch (InLegacyState) {
        case 2:  SelectState = EMenuItemSelectState::Locked; break;
        case 1:  SelectState = EMenuItemSelectState::Hover;  break;
        default: SelectState = EMenuItemSelectState::None;   break;
        }
    }

    // -----------------------------------------------------------------
    // Links
    // -----------------------------------------------------------------
    Menu* GetMenu() const { return MenuPtr; }
    Menu* GetSubmenu() const { return SubMenuPtr; }
    bool  HasSubMenu() const { return SubMenuPtr != nullptr; }

private:
    FString   Text;
    uintptr_t Data = 0;

    bool bEnabled = true;

    EMenuItemSelectState SelectState = EMenuItemSelectState::None;

    Menu* MenuPtr = nullptr;
    Menu* SubMenuPtr = nullptr;

    friend class Menu;
};
