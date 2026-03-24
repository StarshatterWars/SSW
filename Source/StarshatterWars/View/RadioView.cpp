/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    StarshatterWars
    FILE:         RadioView.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    View class for Radio Communications HUD Overlay (UE port)
    - Ported from Starshatter 5.0 RadioView.cpp
    - Uses View drawing primitives (DrawTextRect/DrawRect)
    - Menus are built using Menu/MenuItem hierarchy
    - Menu iteration assumes Menu::GetItems() returns TArray<MenuItem*>
*/

#include "RadioView.h"

#include "RadioMessage.h"
#include "RadioTraffic.h"
#include "QuantumView.h"
#include "HUDView.h"
#include "Ship.h"
#include "SimElement.h"
#include "Sim.h"
#include "Starshatter.h"

#include "Keyboard.h"
#include "Mouse.h"
#include "Game.h"
#include "Menu.h"
#include "MenuItem.h"
#include "MenuHistory.h"

#include "Misc/ScopeLock.h"
#include "Misc/Char.h"
#include "GameStructs.h"

// ---------------------------------------------------------------------
// Local colors (ported from legacy static Color)
// ---------------------------------------------------------------------

static FColor hud_color = FColor::Black;
static FColor txt_color = FColor::White;

// ---------------------------------------------------------------------
// RADIO COMMUNICATIONS MENU (legacy statics)
// ---------------------------------------------------------------------

static Menu* fighter_menu = nullptr;
static Menu* starship_menu = nullptr;
static Menu* target_menu = nullptr;
static Menu* combat_menu = nullptr;
static Menu* formation_menu = nullptr;
static Menu* sensors_menu = nullptr;
static Menu* mission_menu = nullptr;
static Menu* wing_menu = nullptr;
static Menu* elem_menu = nullptr;
static Menu* control_menu = nullptr;

static int   starship_page = 0;
static int   num_pages = 0;
static const int PAGE_SIZE = 9;

static MenuHistory history;

// ---------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------

static bool TextContainsKIA(const FString& S)
{
    return S.Contains(TEXT("KIA"));
}

static bool TargetRequired(const MenuItem* item)
{
    if (!item)
        return false;

    switch (item->GetData()) {
    case (int) RadioMessageAction::ATTACK:
    case (int) RadioMessageAction::BRACKET:
    case (int) RadioMessageAction::ESCORT:
        return true;
    default:
        break;
    }

    // NEW, SAFE CHECK:
    return item->GetSubmenu() == target_menu;
}
// ---------------------------------------------------------------------
// Statics
// ---------------------------------------------------------------------

RadioView* RadioView::radio_view = nullptr;
FCriticalSection RadioView::sync;

// ---------------------------------------------------------------------
// Initialize/Close (ported from Starshatter 5.0)
// ---------------------------------------------------------------------

void
RadioView::Initialize()
{
    static int initialized = 0;
    if (initialized) return;

    target_menu = new Menu("TARGET");
    target_menu->AddItem("1, Attack Target", (int) RadioMessageAction::ATTACK);
    target_menu->AddItem("2. Bracket Target", (int) RadioMessageAction::BRACKET);
    target_menu->AddItem("3. Escort Target", (int) RadioMessageAction::ESCORT);

    combat_menu = new Menu("COMBAT");
    combat_menu->AddItem("1. Cover Me", (int) RadioMessageAction::COVER_ME);
    combat_menu->AddItem("2. Break and attack", (int) RadioMessageAction::WEP_FREE);
    combat_menu->AddItem("3. Form up", (int) RadioMessageAction::FORM_UP);

    formation_menu = new Menu("FORMATION");
    formation_menu->AddItem("1. Goto Diamond", (int) RadioMessageAction::GO_DIAMOND);
    formation_menu->AddItem("2. Goto Spread", (int) RadioMessageAction::GO_SPREAD);
    formation_menu->AddItem("3. Goto Box", (int) RadioMessageAction::GO_BOX);
    formation_menu->AddItem("4. Goto Trail", (int) RadioMessageAction::GO_TRAIL);

    sensors_menu = new Menu("SENSORS");
    sensors_menu->AddItem("1. Goto EMCON 1", (int) RadioMessageAction::GO_EMCON1);
    sensors_menu->AddItem("2. Goto EMCON 2", (int) RadioMessageAction::GO_EMCON2);
    sensors_menu->AddItem("3. Goto EMCON 3", (int) RadioMessageAction::GO_EMCON3);
    sensors_menu->AddItem("4. Launch Probe", (int) RadioMessageAction::LAUNCH_PROBE);

    mission_menu = new Menu("MISSION");
    mission_menu->AddItem("1. Skip Navpoint", (int) RadioMessageAction::SKIP_NAVPOINT);
    mission_menu->AddItem("2. Cancel Orders", (int) RadioMessageAction::RESUME_MISSION);
    mission_menu->AddItem("3. Return to Base", (int) RadioMessageAction::RTB);

    wing_menu = new Menu("WINGMAN");
    wing_menu->AddMenu("1. Target", target_menu);
    wing_menu->AddMenu("2. Combat", combat_menu);
    wing_menu->AddMenu("3. Formation", formation_menu);
    wing_menu->AddMenu("4. Missionm", mission_menu);
    wing_menu->AddMenu("5. Sensors", sensors_menu);

    elem_menu = new Menu("ELEMENT");
    elem_menu->AddMenu("1. Target", target_menu);
    elem_menu->AddMenu("2. Combat", combat_menu);
    elem_menu->AddMenu("3. Formation", formation_menu);
    elem_menu->AddMenu("4. Missionm", mission_menu);
    elem_menu->AddMenu("5. Sensors", sensors_menu);

    control_menu = new Menu("CONTROL");
    control_menu->AddItem("1. Request Picture", (int)RadioMessageAction::REQUEST_PICTURE);
    control_menu->AddItem("2. Request Backup", (int)RadioMessageAction::REQUEST_SUPPORT);
    control_menu->AddItem("3. Call Inbound", (int)RadioMessageAction::CALL_INBOUND);
    control_menu->AddItem("4. Call Finals", (int)RadioMessageAction::CALL_FINALS);

    fighter_menu = new Menu("RADIO");
    fighter_menu->AddMenu("1. Wingman", wing_menu);
    fighter_menu->AddMenu("2.Element", elem_menu);
    fighter_menu->AddMenu("3. Control", control_menu);

    starship_menu = new Menu("RADIO");

    initialized = 1;
}

void RadioView::Close()
{
    history.Clear();

    delete fighter_menu;   fighter_menu = nullptr;
    delete starship_menu;  starship_menu = nullptr;
    delete target_menu;    target_menu = nullptr;
    delete combat_menu;    combat_menu = nullptr;
    delete formation_menu; formation_menu = nullptr;
    delete sensors_menu;   sensors_menu = nullptr;
    delete mission_menu;   mission_menu = nullptr;
    delete wing_menu;      wing_menu = nullptr;
    delete elem_menu;      elem_menu = nullptr;
    delete control_menu;   control_menu = nullptr;

    starship_page = 0;
    num_pages = 0;
}

// ---------------------------------------------------------------------
// Ctor/Dtor
// ---------------------------------------------------------------------

RadioView::RadioView(View* InParent, int ax, int ay, int aw, int ah)
    : View(InParent,
        ax, ay,
        (aw > 0 ? aw : (InParent ? InParent->Width() : 0)),
        (ah > 0 ? ah : (InParent ? InParent->Height() : 0)))
{
    radio_view = this;

    sim = Sim::GetSim();
    font = HUDFont;

    OnWindowMove();

    HUDView* hud = HUDView::GetInstance();
    if (hud)
        SetColor(hud->GetTextColor());

    for (int i = 0; i < MAX_MSG; ++i)
        msg_time[i] = 0.0;
}

RadioView::~RadioView()
{
    if (radio_view == this)
        radio_view = nullptr;
}

void RadioView::OnWindowMove()
{
    width = Width();
    height = Height();
    xcenter = (width / 2.0) - 0.5;
    ycenter = (height / 2.0) + 0.5;
}

// ---------------------------------------------------------------------
// SimObserver
// ---------------------------------------------------------------------

bool RadioView::Update(SimObject* obj)
{
    if (obj == ship) {
        ship = nullptr;
        history.Clear();
    }

    return SimObserver::Update(obj);
}

const char* RadioView::GetObserverName() const
{
    return "RadioView";
}

// ---------------------------------------------------------------------
// Refresh (draw)
// ---------------------------------------------------------------------

void RadioView::Refresh()
{
    sim = Sim::GetSim();
    
    // Keep ship pointer current:
    if (sim && ship != sim->GetPlayerShip()) {
        ship = sim->GetPlayerShip();
        history.Clear();

        if (ship) {
            if (ship->Life() == 0 || ship->IsDying() || ship->IsDead()) {
                ship = nullptr;
            }
            else {
                Observe(ship);
            }
        }
    }

    QuantumView* qv = QuantumView::GetInstance();

    // ------------------------------------------------------------
    // Draw radio menu if not blocked by quantum menu
    // ------------------------------------------------------------
    if (!qv || !qv->IsMenuShown()) {
        Menu* menu = history.GetCurrent();

        if (menu && ship) {
            Rect menu_rect(width - 115, 10, 115, 12);

            // Title
            {
                const FString Title = menu->GetTitle();
                FTCHARToUTF8 Conv(*Title);
                const char* TitleA = (const char*)Conv.Get();
                DrawTextRect(TitleA, 0, menu_rect, DT_CENTER | DT_SINGLELINE);
            }

            menu_rect.y += 15;

            const TArray<MenuItem*>& Items = menu->GetItems();
            for (MenuItem* It : Items) {
                if (!It) {
                    menu_rect.y += 10;
                    continue;
                }

                const FString Txt = It->GetText();

                const bool bDisable =
                    (ship->GetEMCON() < 2) ||
                    (TargetRequired(It) && !ship->GetTarget()) ||
                    TextContainsKIA(Txt);

                It->SetEnabled(!bDisable);

                // Approximate alpha by scaling color:
                const float S = bDisable ? 0.35f : 1.0f;
                const FColor DrawColor(
                    (uint8)FMath::Clamp((int)(txt_color.R * S), 0, 255),
                    (uint8)FMath::Clamp((int)(txt_color.G * S), 0, 255),
                    (uint8)FMath::Clamp((int)(txt_color.B * S), 0, 255),
                    255
                );

                SetColor(DrawColor);

                FTCHARToUTF8 Conv(*Txt);
                const char* TxtA = (const char*)Conv.Get();
                DrawTextRect(TxtA, 0, menu_rect, DT_LEFT | DT_SINGLELINE);

                menu_rect.y += 10;
            }

            SetColor(txt_color);
        }
    }

    // ------------------------------------------------------------
    // Message pipeline aging (same logic as legacy)
    // ------------------------------------------------------------
    int message_queue_empty = 1;

    for (int i = 0; i < MAX_MSG; i++) {
        if (msg_time[i] > 0) {
            msg_time[i] -= Game::GUITime();

            if (msg_time[i] <= 0) {
                msg_time[i] = 0;
                msg_text[i] = "";
            }

            message_queue_empty = 0;
        }
    }

    if (!message_queue_empty) {
        // advance pipeline
        for (int i = 0; i < MAX_MSG; i++) {
            if (msg_time[0] == 0) {
                for (int j = 0; j < MAX_MSG - 1; j++) {
                    msg_time[j] = msg_time[j + 1];
                    msg_text[j] = msg_text[j + 1];
                }
                msg_time[MAX_MSG - 1] = 0;
                msg_text[MAX_MSG - 1] = "";
            }
        }

        bool hud_off = false;
        if (HUDView::GetInstance())
            hud_off = (HUDView::GetInstance()->GetHUDMode() == EHUDMode::Off);

        if (!hud_off) {
            for (int i = 0; i < MAX_MSG; i++) {
                if (msg_time[i] > 0) {
                    Rect msg_rect(0, 95 + i * 10, width, 12);

                    float a = 1.0f;
                    if (msg_time[i] <= 1.0)
                        a = 0.5f + 0.5f * (float)msg_time[i];

                    const FColor FadeColor(
                        (uint8)FMath::Clamp((int)(TextColor.R * a), 0, 255),
                        (uint8)FMath::Clamp((int)(TextColor.G * a), 0, 255),
                        (uint8)FMath::Clamp((int)(TextColor.B * a), 0, 255),
                        255
                    );

                    SetColor(FadeColor);
                    DrawTextRect(msg_text[i].data(), msg_text[i].length(), msg_rect, DT_CENTER | DT_SINGLELINE);
                }
            }

            SetColor(TextColor);
        }
    }

    // ------------------------------------------------------------
    // Chat input line (ported)
    // ------------------------------------------------------------
    Starshatter* stars = Starshatter::GetInstance();
    if (stars && stars->GetChatMode()) {
        Text chat;

        switch (stars->GetChatMode()) {
        case 1:  chat = "ALL:  ";  break;
        case 2:  chat = "TEAM:  "; break;
        case 3:  chat = "WING:  "; break;
        case 4:  chat = "UNIT:  "; break;
        default: break;
        }

        chat += stars->GetChatText();

        Rect chat_rect(width / 2 - 250, height - 150, 500, 12);
        SetColor(TextColor);
        DrawTextRect(chat.data(), 0, chat_rect, DT_LEFT | DT_SINGLELINE);

        chat_rect.Inflate(2, 2);
        DrawRect(chat_rect, hud_color);
    }
}

// ---------------------------------------------------------------------
// SendRadioMessage (ported)
// ---------------------------------------------------------------------

void RadioView::SendRadioMessage(Ship* InShip, MenuItem* item)
{
    if (!InShip || !item) return;
    SimElement* elem = InShip->GetElement();
    if (!elem) return;

    if (dst_elem) {
        const auto Action =
            static_cast<RadioMessageAction>(item->GetData());

        RadioMessage* msg =
            new RadioMessage(dst_elem, InShip, Action);

        if (TargetRequired(item))
            msg->AddTarget(InShip->GetTarget());

        RadioTraffic::Transmit(msg);
        dst_elem = nullptr;
    }
    else if (history.Find("WINGMAN")) {
        int index = InShip->GetElementIndex();
        int wing = 0;

        switch (index) {
        case 1: wing = 2; break;
        case 2: wing = 1; break;
        case 3: wing = 4; break;
        case 4: wing = 3; break;
        default: break;
        }

        if (wing) {
            Ship* dst = elem->GetShip(wing);
            if (dst) {
                const auto Action = static_cast<RadioMessageAction>(item->GetData());
                RadioMessage* msg = new RadioMessage(dst, InShip, Action);

                if (TargetRequired(item))
                    msg->AddTarget(InShip->GetTarget());

                RadioTraffic::Transmit(msg);
            }
        }
    }
    else if (history.Find("ELEMENT")) {
        const auto Action = static_cast<RadioMessageAction>(item->GetData());
        RadioMessage* msg = new RadioMessage(elem, InShip, Action);

        if (TargetRequired(item))
            msg->AddTarget(InShip->GetTarget());

        RadioTraffic::Transmit(msg);
    }
    else if (history.Find("CONTROL")) {
        Ship* controller = InShip->GetController();
        if (controller) {
            const auto Action = static_cast<RadioMessageAction>(item->GetData());
            RadioMessage* msg = new RadioMessage(controller, InShip, Action);
            RadioTraffic::Transmit(msg);
        }
    }
}

// ---------------------------------------------------------------------
// ExecFrame (ported keyboard menu activation)
// ---------------------------------------------------------------------

void RadioView::ExecFrame()
{
    HUDView* hud = HUDView::GetInstance();
    if (hud) {
        if (txt_color != hud->GetTextColor()) {
            txt_color = hud->GetTextColor();
            SetColor(txt_color);
        }
        hud_color = hud->GetHUDColor();
    }

    static int current_key = 0;

    if (current_key > 0 && Keyboard::KeyDown(current_key))
        return;

    current_key = 0;

    Menu* menu = history.GetCurrent();
    if (menu && ship) {
        const int max_items = menu->NumItems();

        if (menu == starship_menu && Keyboard::KeyDown('0')) {
            current_key = '0';
            if (++starship_page >= num_pages)
                starship_page = 0;

            history.Pop();
            history.Push(GetRadioMenu(ship));
        }
        else {
            for (int i = 0; i < max_items; i++) {
                if (Keyboard::KeyDown('1' + i)) {
                    current_key = '1' + i;

                    MenuItem* item = menu->GetItem(i);
                    if (item && item->IsEnabled()) {
                        if (item->GetSubmenu() != nullptr) {
                            if (history.GetCurrent() == starship_menu) {
                                dst_elem = reinterpret_cast<SimElement*>(item->GetData());
                            }

                            history.Push(item->GetSubmenu());
                        }
                        else {
                            SendRadioMessage(ship, item);
                            history.Clear();
                        }
                    }
                    break;
                }
            }
        }
    }
}

// ---------------------------------------------------------------------
// Colors (sync with HUDView)
// ---------------------------------------------------------------------

void RadioView::SetColor(FColor c)
{
    HUDView* hud = HUDView::GetInstance();

    if (hud) {
        hud_color = hud->GetHUDColor();
        txt_color = hud->GetTextColor();
    }
    else {
        hud_color = c;
        txt_color = c;
    }
}

// ---------------------------------------------------------------------
// Menu show/hide
// ---------------------------------------------------------------------

bool RadioView::IsMenuShown()
{
    return history.GetCurrent() != nullptr;
}

void RadioView::ShowMenu()
{
    if (!ship)
        return;

    if (!history.GetCurrent()) {
        history.Push(GetRadioMenu(ship));

        for (int i = 0; i < 10; i++) {
            (void)Keyboard::KeyDown('1' + i);
        }
    }
}

void RadioView::CloseMenu()
{
    history.Clear();
    dst_elem = nullptr;
    starship_page = 0;
    num_pages = 0;
}

// ---------------------------------------------------------------------
// GetRadioMenu (legacy element paging)
// ---------------------------------------------------------------------

Menu* RadioView::GetRadioMenu(Ship* s)
{
    dst_elem = nullptr;

    if (s && sim) {
        if (s->IsStarship()) {
            starship_menu->ClearItems();

            int n = 0;
            const int page_offset = starship_page * PAGE_SIZE;

            // ------------------------------------------------------------
            // First pass: count eligible elements to compute num_pages
            // ------------------------------------------------------------
            if (num_pages == 0) {
                ListIter<SimElement> elem = sim->GetElements();
                while (++elem) {
                    if (elem->IsFinished() || elem->IsSquadron() || elem->IsStatic())
                        continue;

                    if (ship->GetIFF() == elem->GetIFF() && ship->GetElement() != elem.value())
                        n++;
                }

                num_pages = (n / PAGE_SIZE) + (n % PAGE_SIZE > 0 ? 1 : 0);
                n = 0;
            }

            // ------------------------------------------------------------
            // Second pass: build the page
            // ------------------------------------------------------------
            {
                ListIter<SimElement> elem = sim->GetElements();
                while (++elem) {
                    if (elem->IsFinished() || elem->IsSquadron() || elem->IsStatic())
                        continue;

                    if (ship->GetIFF() == elem->GetIFF() && ship->GetElement() != elem.value()) {
                        if (n >= page_offset && n < page_offset + PAGE_SIZE) {
                            char text[64];
                            sprintf_s(text, sizeof(text), "%d. %s", n + 1 - page_offset, (const char*)elem->Name());

                            if (elem->IsActive()) {
                                // store pointer safely in uintptr_t payload:
                                const uintptr_t Payload = reinterpret_cast<uintptr_t>(elem.value());
                                starship_menu->AddMenu(text, elem_menu, Payload);
                            }
                            else {
                                strcat_s(text, sizeof(text), " ");
                                strcat_s(text, sizeof(text), "Not available");
                                starship_menu->AddItem(text, 0, false);
                            }
                        }
                        n++;
                    }
                }
            }

            // ------------------------------------------------------------
            // Add paging item (0 key)
            // ------------------------------------------------------------
            if (num_pages > 1) {
                char text[64];

                const Text fmt = "Next Page";
                sprintf_s(text, sizeof(text), fmt.data(), starship_page + 1, num_pages);

                starship_menu->AddItem(text);
            }

            return starship_menu;
        }
        else if (s->IsDropship()) {
            return fighter_menu;
        }
    }

    return nullptr;
}

// ---------------------------------------------------------------------
// Message queue (thread-safe)
// ---------------------------------------------------------------------

void RadioView::Message(const char* msg)
{
    FScopeLock Lock(&sync);

    if (radio_view) {
        int index = -1;

        for (int i = 0; i < MAX_MSG; i++) {
            if (radio_view->msg_time[i] <= 0) {
                index = i;
                break;
            }
        }

        if (index < 0) {
            for (int i = 0; i < MAX_MSG - 1; i++) {
                radio_view->msg_text[i] = radio_view->msg_text[i + 1];
                radio_view->msg_time[i] = radio_view->msg_time[i + 1];
            }
            index = MAX_MSG - 1;
        }

        radio_view->msg_text[index] = msg;
        radio_view->msg_time[index] = 10;
    }
}

void RadioView::ClearMessages()
{
    FScopeLock Lock(&sync);

    if (radio_view) {
        for (int i = 0; i < MAX_MSG; i++) {
            radio_view->msg_text[i] = Text();
            radio_view->msg_time[i] = 0;
        }
    }
}
