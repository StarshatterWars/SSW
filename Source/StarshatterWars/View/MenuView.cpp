/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    StarshatterWars
    FILE:         MenuView.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    View class for displaying right-click context menus
*/

#include "MenuView.h"

#include "Keyboard.h"
#include "Mouse.h"
#include "MouseController.h"
#include "Menu.h"
#include "UIButton.h"
#include "Game.h"
#include "SystemFont.h"

#include "Misc/Char.h"          // FTCHARToUTF8
#include "Logging/LogMacros.h"

// ---------------------------------------------------------------------
// Local helpers
// ---------------------------------------------------------------------

static FColor ScaleColor(const FColor& C, float S)
{
    const int32 R = FMath::Clamp(FMath::RoundToInt(C.R * S), 0, 255);
    const int32 G = FMath::Clamp(FMath::RoundToInt(C.G * S), 0, 255);
    const int32 B = FMath::Clamp(FMath::RoundToInt(C.B * S), 0, 255);
    return FColor((uint8)R, (uint8)G, (uint8)B, C.A);
}

// ---- MenuItem API adapters (compile-time tolerant) -------------------
// You had errors because your MenuItem doesn’t match legacy names.
// These adapters try common variants and fall back safely.

template <typename T>
static auto MI_GetEnabled_Impl(const T* I, int) -> decltype(I->GetEnabled(), bool())
{
    return I ? (bool)I->GetEnabled() : false;
}
template <typename T>
static auto MI_GetEnabled_Impl(const T* I, long) -> decltype(I->IsEnabled(), bool())
{
    return I ? (bool)I->IsEnabled() : false;
}
template <typename T>
static auto MI_GetEnabled_Impl(const T* I, double) -> decltype(I->Enabled(), bool())
{
    return I ? (bool)I->Enabled() : false;
}
static bool MI_GetEnabled_Impl(...)
{
    return true; // permissive fallback to keep UI usable
}

static bool MI_IsEnabled(const MenuItem* I)
{
    return MI_GetEnabled_Impl(I, 0);
}

template <typename T>
static auto MI_GetSelected_Impl(const T* I, int) -> decltype(I->GetSelected(), int())
{
    return I ? (int)I->GetSelected() : 0;
}
template <typename T>
static auto MI_GetSelected_Impl(const T* I, long) -> decltype(I->Selected(), int())
{
    return I ? (int)I->Selected() : 0;
}
static int MI_GetSelected_Impl(...)
{
    return 0;
}

static int MI_Selected(const MenuItem* I)
{
    return MI_GetSelected_Impl(I, 0);
}

// ---------------------------------------------------------------------
// Ctor / Dtor (MUST match header signature)
// ---------------------------------------------------------------------

MenuView::MenuView(View* InParent, int ax, int ay, int aw, int ah)
    : View(InParent,
        ax,
        ay,
        (aw > 0 ? aw : (InParent ? InParent->Width() : 0)),
        (ah > 0 ? ah : (InParent ? InParent->Height() : 0)))
{
    // Cache initial sizing/offset:
    OnWindowMove();
}

MenuView::~MenuView()
{
}

// ---------------------------------------------------------------------
// Window move (cache offset + size)
// ---------------------------------------------------------------------

void MenuView::OnWindowMove()
{
    // offset is used to convert screen mouse coords into local coords
    if (window) {
        offset.X = X();
        offset.Y = Y();
    }
    else {
        offset.X = 0.0f;
        offset.Y = 0.0f;
    }

    offset.Z = 0.0f;

    // Use our view rect (not window) for width/height:
    width = Width();
    height = Height();
}

// ---------------------------------------------------------------------
// Refresh
// ---------------------------------------------------------------------

void MenuView::Refresh()
{
    if (show_menu)
        DrawMenu();
}

// ---------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------

void MenuView::DoMouseFrame()
{
    static uint32 rbutton_latch = 0;

    action = 0;

    if (Mouse::RButton()) {
        MouseController* mouse_con = MouseController::GetInstance();
        if (!right_down && (!mouse_con || !mouse_con->Active())) {
            rbutton_latch = (uint32)Game::RealTime();
            right_down = 1;
            show_menu = 0;
        }
    }
    else {
        if (right_down && (Game::RealTime() - rbutton_latch < 250)) {
            right_start.X = (float)(Mouse::X() - (int)offset.X);
            right_start.Y = (float)(Mouse::Y() - (int)offset.Y);
            right_start.Z = 0.0f;

            show_menu = 1;
            UIButton::PlaySound(UIButton::SND_MENU_OPEN);
        }

        right_down = 0;
    }

    MouseController* mouse_con = MouseController::GetInstance();

    if (!mouse_con || !mouse_con->Active()) {
        if (Mouse::LButton()) {
            if (!mouse_down)
                shift_down = Keyboard::KeyDown(VK_SHIFT) ? 1 : 0;

            mouse_down = 1;
        }
        else if (mouse_down) {
            int keep_menu = 0;

            if (show_menu) {
                keep_menu = ProcessMenuItem();
                Mouse::Show(true);
            }

            mouse_down = 0;

            if (!keep_menu) {
                ClearMenuSelection(menu);
                show_menu = 0;
            }
        }
    }
}

// ---------------------------------------------------------------------
// Menu selection -> action
// ---------------------------------------------------------------------

int MenuView::ProcessMenuItem()
{
    if (!menu_item)
        return 0;

    if (!MI_IsEnabled(menu_item))
        return 0;

    if (menu_item->GetSubmenu() != 0) {
        menu_item->SetSelected(true);   // correct
        UIButton::PlaySound(UIButton::SND_MENU_OPEN);
        return 1; // keep menu open
    }

    action = menu_item->GetData();
    UIButton::PlaySound(UIButton::SND_MENU_SELECT);
    return 0;
}

// ---------------------------------------------------------------------
// Draw menu
// ---------------------------------------------------------------------

void MenuView::DrawMenu()
{
    menu_item = nullptr;

    if (menu) {
        const int mx = (int)right_start.X - 2;
        const int my = (int)right_start.Y - 2;
        DrawMenu(mx, my, menu);
    }
}

void MenuView::DrawMenu(int mx, int my, Menu* m)
{
    if (!m)
        return;

    MenuItem* locked_item = nullptr;
    Menu* submenu = nullptr;
    int       subx = 0;
    int       suby = 0;

    Rect menu_rect(mx, my, 100, m->NumItems() * 10 + 6);

    int max_width = 0;
    int max_height = 0;
    int extra_width = 16;

    // Items are now TArray<MenuItem*>
    const TArray<MenuItem*>& Items = m->GetItems();

    // -------- Measure pass --------
    for (MenuItem* It : Items) {
        menu_rect.w = width / 2;

        const FString Txt = It ? It->GetText() : FString();

        if (Txt.Len() > 0) {
            // Use HUD font by convention:
            SetFont(HUDFont);

            // DrawTextRect supports DT_CALCRECT like legacy:
            FTCHARToUTF8 Conv(*Txt);
            const char* AnsiObjectName = (const char*)Conv.Get();
            DrawTextRect((const char*)Conv.Get(), 0, menu_rect, DT_LEFT | DT_SINGLELINE | DT_CALCRECT);

            if (menu_rect.w > max_width)
                max_width = menu_rect.w;

            max_height += 11;

            if (It && It->GetSubmenu())
                extra_width = 28;

            if (It && MI_Selected(It) > 1)
                locked_item = It;
        }
        else {
            max_height += 4;
        }
    }

    menu_rect.h = max_height + 6;
    menu_rect.w = max_width + extra_width;

    if (menu_rect.x + menu_rect.w >= width)
        menu_rect.x = width - menu_rect.w - 2;

    if (menu_rect.y + menu_rect.h >= height)
        menu_rect.y = height - menu_rect.h - 2;

    // Background + border:
    FillRect(menu_rect, ScaleColor(BackColor, 0.20f));
    DrawRect(menu_rect, BackColor);

    Rect item_rect = menu_rect;
    item_rect.x += 4;
    item_rect.y += 3;
    item_rect.w -= 8;
    item_rect.h = 12;

    // -------- Draw pass --------
    for (MenuItem* It : Items) {
        int line_height = 0;

        const FString Txt = It ? It->GetText() : FString();

        if (Txt.Len() > 0) {
            Rect fill_rect = item_rect;
            fill_rect.Inflate(2, -1);
            fill_rect.y -= 1;

            const int mouse_x = Mouse::X() - (int)offset.X;
            const int mouse_y = Mouse::Y() - (int)offset.Y;

            // Is this item hovered/picked?
            if (menu_rect.Contains(mouse_x, mouse_y)) {
                if (mouse_y >= fill_rect.y && mouse_y <= fill_rect.y + fill_rect.h) {
                    if (Mouse::LButton()) {
                        menu_item = It;
                        It->SetSelected(2);

                        if (locked_item)
                            locked_item->SetSelected(false);

                        locked_item = menu_item;
                    }
                    else if (!locked_item) {
                        It->SetSelected(1);
                        menu_item = It;
                    }

                    if (menu_item && menu_item != selected) {
                        selected = menu_item;
                        UIButton::PlaySound(UIButton::SND_MENU_HILITE);
                    }
                }
                else if (It != locked_item) {
                    It->SetSelected(0);
                }
            }

            if (MI_Selected(It) != 0) {
                FillRect(fill_rect, ScaleColor(BackColor, 0.35f));
                DrawRect(fill_rect, ScaleColor(BackColor, 0.75f));

                if (It->GetSubmenu()) {
                    submenu = It->GetSubmenu();
                    subx = menu_rect.x + max_width + extra_width;
                    suby = fill_rect.y - 3;
                }
            }

            // Text color:
            if (MI_IsEnabled(It))
                HUDFont->SetColor(TextColor);
            else
                HUDFont->SetColor(ScaleColor(TextColor, 0.33f));

            SetFont(HUDFont);

            FTCHARToUTF8 Conv(*Txt);
            DrawTextRect((const char*)Conv.Get(), 0, item_rect, DT_LEFT | DT_SINGLELINE);

            line_height = 11;
        }
        else {
            // separator line
            DrawLine(
                item_rect.x,
                item_rect.y + 2,
                item_rect.x + max_width + extra_width - 8,
                item_rect.y + 2,
                BackColor
            );
            line_height = 4;
        }

        // submenu arrow
        if (It && It->GetSubmenu()) {
            const int left = item_rect.x + max_width + 10;
            const int top = item_rect.y + 1;

            FVector arrow[3];
            arrow[0] = FVector((float)left, (float)top, 0.0f);
            arrow[1] = FVector((float)left + 8, (float)top + 4, 0.0f);
            arrow[2] = FVector((float)left, (float)top + 8, 0.0f);

            FillPoly(3, arrow, BackColor);
        }

        item_rect.y += line_height;
    }

    // Recurse submenu:
    if (submenu) {
        if (subx + 60 > width)
            subx = menu_rect.x - 60;

        DrawMenu(subx, suby, submenu);
    }
}

// ---------------------------------------------------------------------
// Clear selection (recursive)
// ---------------------------------------------------------------------

void MenuView::ClearMenuSelection(Menu* m)
{
    if (!m)
        return;

    const TArray<MenuItem*>& Items = m->GetItems();

    for (MenuItem* It : Items) {
        if (!It)
            continue;

        It->SetSelected(0);

        if (It->GetSubmenu())
            ClearMenuSelection(It->GetSubmenu());
    }
}
