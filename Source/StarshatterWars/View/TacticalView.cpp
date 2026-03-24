/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    StarshatterWars
    FILE:         TacticalView.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    TacticalView (plain C++) - ported from Starshatter 5.0 TacticalView
    - Uses View drawing primitives (no Window.h)
    - Uses UE types for math (FVector/FColor/FString)
*/

#include "TacticalView.h"

// Your ported includes:
#include "Sim.h"
#include "Ship.h"
#include "Starshatter.h"
#include "SimContact.h"
#include "SimElement.h"
#include "Instruction.h"
#include "SimProjector.h"
#include "GameScreen.h"

#include "Menu.h"
#include "MenuView.h"

#include "HUDView.h"
#include "WepView.h"
#include "QuantumView.h"
#include "RadioView.h"
#include "MenuView.h"
#include "RadioMessage.h"
#include "RadioTraffic.h"
#include "HUDSounds.h"

#include "CameraManager.h"
#include "ShipManager.h"
#include "ShipDesign.h"
#include "QuantumDrive.h"
#include "Farcaster.h"

#include "Keyboard.h"
#include "Mouse.h"
#include "Game.h"
#include "FormatUtil.h"
#include "GameStructs.h"
#include <string>

static bool gInvalidAction = false;

// Static state:
TacticalView* TacticalView::TacView = nullptr;
FColor        TacticalView::HudColor = FColor::Black;
FColor        TacticalView::TxtColor = FColor::Black;

// Legacy-like shared menus:
static Menu* gMainMenu = nullptr;
static Menu* gViewMenu = nullptr;
static Menu* gEmconMenu = nullptr;

static Menu* gFighterMenu = nullptr;
static Menu* gStarshipMenu = nullptr;
static Menu* gActionMenu = nullptr;
static Menu* gFormationMenu = nullptr;
static Menu* gSensorsMenu = nullptr;
static Menu* gQuantumMenu = nullptr;
static Menu* gFarcastMenu = nullptr;

static bool GetMenuItemText(MenuView* MenuViewPtr, FString& OutText)
{
    OutText.Reset();

    if (!MenuViewPtr)
        return false;

    MenuItem* MI = MenuViewPtr->GetMenuItem();
    if (!MI)
        return false;

    OutText = MI->GetText();       
    return !OutText.IsEmpty();
}

enum VIEW_MENU
{
    VIEW_FORWARD = 1000,
    VIEW_CHASE,
    VIEW_PADLOCK,
    VIEW_ORBIT,
    VIEW_NAV,
    VIEW_WEP,
    VIEW_ENG,
    VIEW_FLT,
    VIEW_INS,
    VIEW_CMD
};

static const int32 QUANTUM = 2000;
static const int32 FARCAST = 2001;

static SimElement* gDstElem = nullptr;

TacticalView::TacticalView(View* InParent, UGameScreen* InParentScreen)
    : View(InParent, 0, 0, InParent ? InParent->Width() : 0, InParent ? InParent->Height() : 0),
    GameParent(InParentScreen)
{
    TacView = this;

    SimPtr = Sim::GetSim();

    WidthPx = Width();
    HeightPx = Height();
    XCenter = (WidthPx / 2.0) - 0.5;
    YCenter = (HeightPx / 2.0) + 0.5;

    bMouseDown = 0;
    bRightDown = 0;
    bShiftDown = 0;
    bShowMove = 0;
    ShowAction = RadioMessageAction::NONE;

    MouseStart = FIntPoint(0, 0);
    MouseAction = FIntPoint(0, 0);
    MouseRect = Rect();

    // Menu view renders using View drawing primitives internally:
    MenuViewPtr = new MenuView(this);

    // Default colors (will be synced from HUDView in ExecFrame):
    SetColor(FColor::White, FColor::White);
}

TacticalView::~TacticalView()
{
    delete MenuViewPtr;
    MenuViewPtr = nullptr;

    TacView = nullptr;
}

void TacticalView::OnWindowMove()
{
    WidthPx = Width();
    HeightPx = Height();
    XCenter = (WidthPx / 2.0) - 0.5;
    YCenter = (HeightPx / 2.0) + 0.5;

    if (MenuViewPtr)
        MenuViewPtr->OnWindowMove();
}

bool TacticalView::Update(SimObject* Obj)
{
    if (Obj == PlayerShip)
        PlayerShip = nullptr;

    if (Obj == MsgShip)
        MsgShip = nullptr;

    return SimObserver::Update(Obj);
}

const char* TacticalView::GetObserverName() const
{
    return "TacticalView";
}

void TacticalView::UseProjector(SimProjector* InProjector)
{
    ProjectorPtr = InProjector;
}

void TacticalView::ExecFrame()
{
    HUDView* Hud = HUDView::GetInstance();
    if (Hud)
    {
        const FColor NewHud = Hud->GetHUDColor();
        const FColor NewTxt = Hud->GetTextColor();

        if (NewHud != HudColor || NewTxt != TxtColor)
            SetColor(NewHud, NewTxt);
    }
}

void TacticalView::SetColor(const FColor& InHudColor, const FColor& InTextColor)
{
    HudColor = InHudColor;
    TxtColor = InTextColor;
}

void TacticalView::Refresh()
{
    SimPtr = Sim::GetSim();
    if (!SimPtr)
        return;

    bool bRebuild = false;

    Ship* SimPlayer = SimPtr->GetPlayerShip();
    if (PlayerShip != SimPlayer)
    {
        PlayerShip = SimPlayer;

        if (PlayerShip)
        {
            if (PlayerShip->Life() == 0 || PlayerShip->IsDying() || PlayerShip->IsDead())
            {
                PlayerShip = nullptr;
            }
            else
            {
                Observe(PlayerShip);
            }
        }

        bRebuild = true;
    }

    if (!PlayerShip || PlayerShip->InTransition())
        return;

    if (PlayerShip->GetRegion())
    {
        const char* RegionName = PlayerShip->GetRegion()->GetName();
        if (RegionName && CurrentSector != FString(UTF8_TO_TCHAR(RegionName)))
            bRebuild = true;

        if (bRebuild)
        {
            BuildMenu();
            CurrentSector = RegionName ? FString(UTF8_TO_TCHAR(RegionName)) : FString();
        }
    }

    DrawMouseRect();

    // Draw selection rects + info:
    ListIter<Ship> Sel = SimPtr->GetSelection();
    if (Sel.size())
    {
        while (++Sel)
        {
            Ship* Selection = Sel.value();
            if (Selection && Selection->Rep())
                DrawSelection(Selection);
        }

        RadioView* RV = RadioView::GetInstance();
        QuantumView* QV = QuantumView::GetInstance();

        const bool bMenuShown = (RV && RV->IsMenuShown()) || (QV && QV->IsMenuShown());

        if (!bMenuShown)
        {
            Sel.reset();

            if (Sel.size() == 1)
                DrawSelectionInfo(Sel.next());
            else
                DrawSelectionList(Sel);
        }
    }

    DrawMenu();

    if (bShowMove)
    {
        Mouse::Show(false);
        DrawMove();
    }
    else if (ShowAction == RadioMessageAction::NONE)
    {
        Mouse::Show(false);
        DrawAction();
    }
}

void TacticalView::DrawMouseRect()
{
    if (MouseRect.w > 0 && MouseRect.h > 0)
    {
        FColor C = HudColor;
        C.A = (uint8)170; // about 0.66 alpha

        if (bShiftDown)
            C = FColor(255, 165, 0, 255); // orange

        DrawRect(MouseRect, C);
    }
}

void TacticalView::DrawSelection(Ship* SelectedShip)
{
    if (!SelectedShip || !ProjectorPtr)
        return;

    Graphic* G = SelectedShip->Rep();
    Rect R = G ? G->ScreenRect() : Rect();

    FVector MarkPt = SelectedShip->Location();
    ProjectorPtr->Transform(MarkPt);

    if (MarkPt.Z > 1.0)
    {
        ProjectorPtr->Project(MarkPt);

        int32 X = (int32)MarkPt.X;
        int32 Y = R.y;

        if (Y >= 2000)
            Y = (int32)MarkPt.Y;

        if (X > 4 && X < WidthPx - 4 && Y > 4 && Y < HeightPx - 4)
        {
            const int32 BAR_LENGTH = 40;

            int32 SX = X - BAR_LENGTH / 2;
            int32 SY = Y - 8;

            double HullStrength = SelectedShip->HullStrength() / 100.0;
            int32 HW = (int32)(BAR_LENGTH * HullStrength);
            int32 SW = (int32)(BAR_LENGTH * (SelectedShip->ShieldStrength() / 100.0));

            if (HW < 0) HW = 0;
            if (SW < 0) SW = 0;

            SYSTEM_STATUS S = SYSTEM_STATUS::NOMINAL;
            if (HullStrength < 0.30)      S = SYSTEM_STATUS::CRITICAL;
            else if (HullStrength < 0.60) S = SYSTEM_STATUS::DEGRADED;

            const FColor HC = HUDView::GetStatusColor(S);
            const FColor SC = HudColor;

            FillRect(SX, SY, SX + HW, SY + 1, HC);
            FillRect(SX, SY + 3, SX + SW, SY + 4, SC);
        }
    }
}

void TacticalView::DrawSelectionInfo(Ship* SelectedShip)
{
    if (!PlayerShip || !SelectedShip)
        return;

    // This function depends heavily on your Font/Text implementation.
    // Keep the legacy logic, but drive it through View::Print or your SystemFont wrapper.
    // If your SystemFont API is already in View.cpp, swap these prints to that API.

    // Minimal safe output (no std::string):
    // Example: show just the name and range.
    const FVector Delta = SelectedShip->Location() - PlayerShip->Location();
    const double DistKm = (double)Delta.Length() / 1000.0;

    char RangeBuf[64];
    RangeBuf[0] = 0;
    FormatNumberExp(RangeBuf, DistKm);
    strcat_s(RangeBuf, " km");

    // Top-right text area (approx legacy):
    int32 X = WidthPx - 220;
    int32 Y = 10;

    Print(X, Y, "%s", SelectedShip->Name());
    Y += 12;
    Print(X, Y, "%s", RangeBuf);
}

void TacticalView::DrawSelectionList(ListIter<Ship>& SelectionIter)
{
    int32 Index = 0;
    int32 X = WidthPx - 220;
    int32 Y = 10;

    while (++SelectionIter)
    {
        Ship* S = SelectionIter.value();
        if (!S)
            continue;

        Print(X, Y, "%s", S->Name());
        Y += 12;

        Index++;
        if (Index >= 10)
            break;
    }
}

void TacticalView::DoMouseFrame()
{
    static DWORD RButtonLatch = 0;

    Starshatter* Stars = Starshatter::GetInstance();
    if (Stars && Stars->InCutscene())
        return;

    // Right click latch:
    if (Mouse::RButton())
    {
        if (!bRightDown)
        {
            RButtonLatch = Game::RealTime();
            bRightDown = true;
        }
    }
    else
    {
        if (SimPtr && bRightDown && (Game::RealTime() - RButtonLatch < 250))
        {
            Ship* Seln = WillSelectAt(Mouse::X(), Mouse::Y());

            if (Seln && SimPtr->IsSelected(Seln) &&
                PlayerShip && Seln->GetIFF() == PlayerShip->GetIFF() &&
                PlayerShip->GetElement() && PlayerShip->GetElement()->CanCommand(Seln->GetElement()))
            {
                MsgShip = Seln;
                Observe(MsgShip);
            }
            else if (PlayerShip && Seln == PlayerShip &&
                (!PlayerShip->GetDirector() || PlayerShip->GetDirector()->Type() != ShipManager::DIR_TYPE))
            {
                MsgShip = Seln;
            }
            else
            {
                MsgShip = nullptr;
            }
        }

        bRightDown = false;
    }

    if (!PlayerShip || !SimPtr)
        return;

    // Left mouse selection rectangle:
    if (Mouse::LButton())
    {
        if (!bMouseDown)
        {
            MouseStart = FIntPoint(Mouse::X(), Mouse::Y());
            bShiftDown = Keyboard::KeyDown(VK_SHIFT) ? 1 : 0;
        }
        else
        {
            const int32 MX = Mouse::X();
            const int32 MY = Mouse::Y();

            int32 X0 = MouseStart.X;
            int32 Y0 = MouseStart.Y;

            int32 X1 = MX;
            int32 Y1 = MY;

            const int32 RX = (X1 < X0) ? X1 : X0;
            const int32 RY = (Y1 < Y0) ? Y1 : Y0;
            const int32 RW = (int32)FMath::Abs(X1 - X0);
            const int32 RH = (int32)FMath::Abs(Y1 - Y0);

            MouseRect = Rect(RX, RY, RW, RH);

            // Dont draw while zooming or special modes:
            if (Mouse::RButton() || bShowMove || ShowAction != RadioMessageAction::NONE)
            {
                MouseRect.w = 0;
                MouseRect.h = 0;
            }
            else
            {
                SelectRect(MouseRect);
            }
        }

        bMouseDown = true;
    }
    else
    {
        if (bMouseDown)
        {
            const int32 MX = Mouse::X();
            const int32 MY = Mouse::Y();

            if (bShowMove)
            {
                SendMove();
                bShowMove = 0;
                Mouse::Show(true);
            }
            else if (ShowAction != RadioMessageAction::NONE)
            {
                SendAction();
                ShowAction = RadioMessageAction::NONE;
                Mouse::Show(true);
            }
            else
            {
                if (!HUDView::IsMouseLatched() && !WepView::IsMouseLatched())
                {
                    const int32 DX = (int32)FMath::Abs(MX - MouseStart.X);
                    const int32 DY = (int32)FMath::Abs(MY - MouseStart.Y);

                    static DWORD ClickTime = 0;

                    if (DX < 3 && DY < 3)
                    {
                        const bool bHit = SelectAt(MX, MY);

                        if (PlayerShip && PlayerShip->IsStarship() && Game::RealTime() - ClickTime < 350)
                            SetHelm(bHit);

                        ClickTime = Game::RealTime();
                    }
                }
            }

            MouseRect = Rect();
            bMouseDown = false;
        }
    }

    if (ShowAction != RadioMessageAction::NONE && !bMouseDown && !bRightDown)
        MouseAction = FIntPoint(Mouse::X(), Mouse::Y());
}

bool TacticalView::SelectAt(int32 X, int32 Y)
{
    if (!PlayerShip)
        return false;

    Ship* Selection = WillSelectAt(X, Y);

    if (Selection && bShiftDown)
        PlayerShip->SetTarget(Selection);
    else if (SimPtr && Selection)
        SimPtr->SetSelection(Selection);

    return Selection != nullptr;
}

bool TacticalView::SelectRect(const Rect& R)
{
    bool bResult = false;

    if (!PlayerShip || !SimPtr || !ProjectorPtr)
        return false;

    if (R.w > 8 || R.h > 8)
        SimPtr->ClearSelection();

    List<SimContact>& ContactList = PlayerShip->ContactList();

    for (int32 i = 0; i < PlayerShip->NumContacts(); i++)
    {
        Ship* Test = ContactList[i] ? ContactList[i]->GetShip() : nullptr;
        if (!Test || Test == PlayerShip)
            continue;

        FVector TestLoc = Test->Location();
        ProjectorPtr->Transform(TestLoc);

        if (TestLoc.Z > 1.0)
        {
            ProjectorPtr->Project(TestLoc);

            if (R.Contains((int)TestLoc.X, (int)TestLoc.Y))
            {
                if (bShiftDown)
                {
                    if (Test->GetIFF() == 0 || Test->GetIFF() == PlayerShip->GetIFF())
                        continue;

                    PlayerShip->SetTarget(Test);
                    bResult = true;
                }
                else
                {
                    SimPtr->AddSelection(Test);
                    bResult = true;
                }
            }
        }
    }

    // Select self only in orbit cam:
    if (!bShiftDown && CameraManager::GetCameraMode() == CameraManager::MODE_ORBIT)
    {
        FVector TestLoc = PlayerShip->Location();
        ProjectorPtr->Transform(TestLoc);

        if (TestLoc.Z > 1.0)
        {
            ProjectorPtr->Project(TestLoc);

            if (R.Contains((int)TestLoc.X, (int)TestLoc.Y))
            {
                SimPtr->AddSelection(PlayerShip);
                bResult = true;
            }
        }
    }

    return bResult;
}

Ship* TacticalView::WillSelectAt(int32 X, int32 Y)
{
    Ship* Selection = nullptr;

    if (!PlayerShip)
        return nullptr;

    List<SimContact>& ContactList = PlayerShip->ContactList();

    for (int32 i = 0; i < PlayerShip->NumContacts(); i++)
    {
        Ship* Test = ContactList[i] ? ContactList[i]->GetShip() : nullptr;
        if (!Test)
            continue;

        if (bShiftDown)
        {
            if (Test->GetIFF() == 0 || Test->GetIFF() == PlayerShip->GetIFF())
                continue;
        }

        Graphic* G = Test->Rep();
        if (!G)
            continue;

        Rect R = G->ScreenRect();

        // Some reps report offscreen (legacy 2000,2000):
        if (R.x == 2000 && R.y == 2000 && R.w == 0 && R.h == 0 && ProjectorPtr)
        {
            FVector Loc = Test->Location();
            ProjectorPtr->Transform(Loc);
            ProjectorPtr->Project(Loc);

            R.x = (int)Loc.X;
            R.y = (int)Loc.Y;
        }

        if (R.w < 20 || R.h < 20) R.Inflate(20, 20);
        else                      R.Inflate(10, 10);

        if (R.Contains(X, Y))
        {
            Selection = Test;
            break;
        }
    }

    if (!Selection && !bShiftDown)
    {
        Graphic* G = PlayerShip->Rep();
        if (G)
        {
            Rect R = G->ScreenRect();
            if (R.Contains(X, Y))
                Selection = PlayerShip;
        }
    }

    if (Selection == PlayerShip && CameraManager::GetCameraMode() != CameraManager::MODE_ORBIT)
        Selection = nullptr;

    return Selection;
}

void TacticalView::SetHelm(bool bApproach)
{
    if (!PlayerShip || !ProjectorPtr || !SimPtr)
        return;

    FVector Delta = FVector::ZeroVector;

    if (bApproach)
    {
        ListIter<Ship> Iter = SimPtr->GetSelection();
        ++Iter;
        Ship* Selection = Iter.value();

        if (Selection && Selection != PlayerShip)
        {
            Delta = Selection->Location() - PlayerShip->Location();
            Delta.Normalize();
        }
    }

    if (Delta.Size() < 1.0)
    {
        const int32 MX = Mouse::X();
        const int32 MY = Mouse::Y();

        const double FocalDist = (double)WidthPx / tan(ProjectorPtr->XAngle());

        FVector V = ProjectorPtr->vpn() * (float)FocalDist +
            ProjectorPtr->vup() * -1.0f * (float)(MY - HeightPx / 2) +
            ProjectorPtr->vrt() * (float)(MX - WidthPx / 2);

        V.Normalize();
        Delta = V;
    }

    double Az = atan2(fabs((double)Delta.X), (double)Delta.Z);
    double El = asin((double)Delta.Y);

    if (Delta.X < 0)
        Az *= -1;

    Az += PI;
    if (Az >= 2 * PI)
        Az -= 2 * PI;

    PlayerShip->SetHelmHeading(Az);
    PlayerShip->SetHelmPitch(El);
}

void TacticalView::Initialize()
{
    static int32 bInitialized = 0;
    if (gViewMenu)
    {
        delete gViewMenu;
        gViewMenu = nullptr;
    }

    gViewMenu = new Menu("VIEW");
    gViewMenu->AddItem("Forward", VIEW_FORWARD);
    gViewMenu->AddItem("Chase", VIEW_CHASE);
    gViewMenu->AddItem("Orbit", VIEW_ORBIT);
    gViewMenu->AddItem("Padlock", VIEW_PADLOCK);

    gEmconMenu = new Menu("SENSORS");

    gQuantumMenu = new Menu("QUANTUM");
    gFarcastMenu = new Menu("FARCAST");

    gMainMenu = new Menu("MAIN");

    gActionMenu = new Menu("ACTION");
    gActionMenu->AddItem("Engage", (int) RadioMessageAction::ATTACK);
    gActionMenu->AddItem("Bracket", (int)RadioMessageAction::BRACKET);
    gActionMenu->AddItem("Escort", (int)RadioMessageAction::ESCORT);
    gActionMenu->AddItem("Identify", (int)RadioMessageAction::IDENTIFY);
    gActionMenu->AddItem("Hold", (int)RadioMessageAction::WEP_HOLD);

    gFormationMenu = new Menu("FORMATION");
    gFormationMenu->AddItem("Diamond", (int)RadioMessageAction::GO_DIAMOND);
    gFormationMenu->AddItem("Spread", (int)RadioMessageAction::GO_SPREAD);
    gFormationMenu->AddItem("Box", (int)RadioMessageAction::GO_BOX);
    gFormationMenu->AddItem("Trail", (int)RadioMessageAction::GO_TRAIL);

    gSensorsMenu = new Menu("SENSORS");
    gSensorsMenu->AddItem("Goto EMCON 1", (int)RadioMessageAction::GO_EMCON1);
    gSensorsMenu->AddItem("Goto EMCON 2", (int)RadioMessageAction::GO_EMCON2);
    gSensorsMenu->AddItem("TGoto EMCON 3", (int)RadioMessageAction::GO_EMCON3);
    gSensorsMenu->AddItem("Launch Probe", (int)RadioMessageAction::LAUNCH_PROBE);

    gFighterMenu = new Menu("CONTEXT");
    gFighterMenu->AddMenu("Action", gActionMenu);
    gFighterMenu->AddMenu("Formation", gFormationMenu);
    gFighterMenu->AddMenu("Sensors", gSensorsMenu);
    gFighterMenu->AddItem("Move Patrol", (int)RadioMessageAction::MOVE_PATROL);
    gFighterMenu->AddItem("Cancel Orders", (int)RadioMessageAction::RESUME_MISSION);
    gFighterMenu->AddItem("", 0);
    gFighterMenu->AddItem("Return to Base", (int)RadioMessageAction::RTB);
    gFighterMenu->AddItem("Dock With", (int)RadioMessageAction::DOCK_WITH);
    gFighterMenu->AddMenu("Farcast", gFarcastMenu);

    gStarshipMenu = new Menu("CONTEXT");
    gStarshipMenu->AddMenu("Action", gActionMenu);
    gStarshipMenu->AddMenu("Sensors", gSensorsMenu);
    gStarshipMenu->AddItem("Move Patrol", (int)RadioMessageAction::MOVE_PATROL);
    gStarshipMenu->AddItem("Cancel Orders", (int)RadioMessageAction::RESUME_MISSION);
    gStarshipMenu->AddItem("", 0);
    gStarshipMenu->AddMenu("Quantum", gQuantumMenu);
    gStarshipMenu->AddMenu("Farcast", gFarcastMenu);

    bInitialized = 1;
}

void TacticalView::Close()
{
    delete gViewMenu;      gViewMenu = nullptr;
    delete gEmconMenu;     gEmconMenu = nullptr;
    delete gMainMenu;      gMainMenu = nullptr;
    delete gFighterMenu;   gFighterMenu = nullptr;
    delete gStarshipMenu;  gStarshipMenu = nullptr;
    delete gActionMenu;    gActionMenu = nullptr;
    delete gFormationMenu; gFormationMenu = nullptr;
    delete gSensorsMenu;   gSensorsMenu = nullptr;
    delete gQuantumMenu;   gQuantumMenu = nullptr;
    delete gFarcastMenu;   gFarcastMenu = nullptr;
}

void TacticalView::BuildMenu()
{
    if (!gMainMenu || !gQuantumMenu || !gFarcastMenu || !gEmconMenu)
        return;

    gMainMenu->ClearItems();
    gQuantumMenu->ClearItems();
    gFarcastMenu->ClearItems();
    gEmconMenu->ClearItems();

    if (!PlayerShip || !SimPtr)
        return;

    // Quantum destinations:
    ListIter<SimRegion> Iter = SimPtr->GetRegions();
    while (++Iter)
    {
        SimRegion* Rgn = Iter.value();
        if (Rgn && PlayerShip->GetRegion() && Rgn != PlayerShip->GetRegion() && Rgn->GetType() != SimRegion::AIR_SPACE)
            gQuantumMenu->AddItem(Rgn->GetName(), QUANTUM);
    }

    // Farcaster destinations:
    if (PlayerShip->GetRegion())
    {
        ListIter<Ship> ShipsIter = PlayerShip->GetRegion()->GetShips();
        while (++ShipsIter)
        {
            Ship* S = ShipsIter.value();
            if (S && S->GetFarcaster())
            {
                Farcaster* Far = S->GetFarcaster();
                Far->ExecFrame(0);

                const Ship* Dest = Far->GetDest();
                if (Dest && Dest->GetRegion())
                {
                    SimRegion* Rgn = Dest->GetRegion();
                    if (Rgn)
                        gFarcastMenu->AddItem(Rgn->GetName(), FARCAST);
                }
            }
        }
    }

    // Main menu:
    gMainMenu->AddMenu("Camera", gViewMenu);
    gMainMenu->AddItem("", 0);
    gMainMenu->AddItem("INSTR", VIEW_INS);
    gMainMenu->AddItem("NAV", VIEW_NAV);

    if (PlayerShip->Design() && PlayerShip->Design()->repair_screen)
        gMainMenu->AddItem("ENG", VIEW_ENG);

    if (PlayerShip->Design() && PlayerShip->Design()->wep_screen)
        gMainMenu->AddItem("WEP", VIEW_WEP);

    if (PlayerShip->NumFlightDecks() > 0)
        gMainMenu->AddItem("FLIGHT", VIEW_FLT);

    gEmconMenu->AddItem("Goto EMCON 1", (int)RadioMessageAction::GO_EMCON1);
    gEmconMenu->AddItem("Goto EMCON 2", (int)RadioMessageAction::GO_EMCON2);
    gEmconMenu->AddItem("Goto EMCON 3", (int)RadioMessageAction::GO_EMCON3);

    if (PlayerShip->GetProbeLauncher())
        gEmconMenu->AddItem("Launch Probe", (int)RadioMessageAction::LAUNCH_PROBE);

    gMainMenu->AddItem("", 0);
    gMainMenu->AddMenu("Sensors", gEmconMenu);

    if (PlayerShip->GetQuantumDrive())
    {
        gMainMenu->AddItem("", 0);
        gMainMenu->AddMenu("Quantum", gQuantumMenu);
    }

    if (PlayerShip->IsStarship())
    {
        gMainMenu->AddItem("", 0);
        gMainMenu->AddItem("COMMAND", VIEW_CMD);
    }
}

void TacticalView::DrawMenu()
{
    ActiveMenu = nullptr;

    if (PlayerShip)
        ActiveMenu = gMainMenu;

    if (MsgShip)
    {
        if (MsgShip->IsStarship())
            ActiveMenu = gStarshipMenu;
        else if (MsgShip->IsDropship())
            ActiveMenu = gFighterMenu;
    }

    if (MenuViewPtr)
    {
        MenuViewPtr->SetBackColor(HudColor);
        MenuViewPtr->SetTextColor(TxtColor);
        MenuViewPtr->SetMenu(ActiveMenu);
        MenuViewPtr->Refresh();
    }
}

bool TacticalView::GetMouseLoc3D()
{
    if (!ProjectorPtr)
        return false;

    int32 MX = Mouse::X();
    int32 MY = Mouse::Y();

    const double FocalDist = (double)WidthPx / tan(ProjectorPtr->XAngle());

    FVector FocalVect =
        ProjectorPtr->vpn() * (float)FocalDist +
        ProjectorPtr->vup() * -1.0f * (float)(MY - HeightPx / 2) +
        ProjectorPtr->vrt() * (float)(MX - WidthPx / 2);

    FocalVect.Normalize();

    if (Keyboard::KeyDown(VK_SHIFT))
    {
        if (Mouse::RButton())
            return true;

        // Alt adjust:
        if (fabs((double)FocalVect.X) > fabs((double)FocalVect.Z))
        {
            const double DX = (double)MoveLoc.X - (double)ProjectorPtr->Pos().X;
            const double T = -1.0 * (((double)ProjectorPtr->Pos().X - DX) / (double)FocalVect.X);

            if (T > 0)
            {
                const FVector P = ProjectorPtr->Pos() + FocalVect * (float)T;
                MoveAlt = (double)P.Y - BaseAlt;
            }
        }
        else
        {
            const double DZ = (double)MoveLoc.Z - (double)ProjectorPtr->Pos().Z;
            const double T = -1.0 * (((double)ProjectorPtr->Pos().Z - DZ) / (double)FocalVect.Z);

            if (T > 0)
            {
                const FVector P = ProjectorPtr->Pos() + FocalVect * (float)T;
                MoveAlt = (double)P.Y - BaseAlt;
            }
        }

        if (MoveAlt > 25e3) MoveAlt = 25e3;
        else if (MoveAlt < -25e3) MoveAlt = -25e3;

        return true;
    }
    else
    {
        if (fabs((double)FocalVect.Y) > 1e-5)
        {
            if (Mouse::RButton())
                return true;

            bool bClamp = false;
            double T = -1.0 * (((double)ProjectorPtr->Pos().Y - BaseAlt) / (double)FocalVect.Y);

            while (T <= 0 && MY < HeightPx - 1)
            {
                MY++;
                bClamp = true;

                FocalVect =
                    ProjectorPtr->vpn() * (float)FocalDist +
                    ProjectorPtr->vup() * -1.0f * (float)(MY - HeightPx / 2) +
                    ProjectorPtr->vrt() * (float)(MX - WidthPx / 2);

                FocalVect.Normalize();
                T = -1.0 * (((double)ProjectorPtr->Pos().Y - BaseAlt) / (double)FocalVect.Y);
            }

            if (T > 0)
            {
                if (bClamp)
                    Mouse::SetCursorPos(MX, MY);

                MoveLoc = ProjectorPtr->Pos() + FocalVect * (float)T;
            }

            return true;
        }
    }

    return false;
}

void TacticalView::DrawMove()
{
    if (!ProjectorPtr || !bShowMove || !MsgShip)
        return;

    FVector Origin = MsgShip->Location();

    if (GetMouseLoc3D())
    {
        FVector Dest = MoveLoc;

        double Distance = (double)(Dest - Origin).Length();

        ProjectorPtr->Transform(Origin);
        ProjectorPtr->Project(Origin);

        int32 X0 = (int32)Origin.X;
        int32 Y0 = (int32)Origin.Y;

        ProjectorPtr->Transform(Dest);
        ProjectorPtr->Project(Dest);

        int32 X = (int32)Dest.X;
        int32 Y = (int32)Dest.Y;

        DrawEllipse(X - 10, Y - 10, X + 10, Y + 10, FColor::White);
        DrawLine(X0, Y0, X, Y, FColor::White);

        char Range[32];
        Range[0] = 0;

        Rect RangeRect(X + 12, Y - 8, 120, 20);

        if (fabs(MoveAlt) > 1.0)
        {
            Dest = MoveLoc;
            Dest.Y += (float)MoveAlt;
            Distance = (double)(Dest - MsgShip->Location()).Length();

            ProjectorPtr->Transform(Dest);
            ProjectorPtr->Project(Dest);

            int32 X1 = (int32)Dest.X;
            int32 Y1 = (int32)Dest.Y;

            DrawEllipse(X1 - 10, Y1 - 10, X1 + 10, Y1 + 10, FColor::White);
            DrawLine(X0, Y0, X1, Y1, FColor::White);
            DrawLine(X1, Y1, X, Y, FColor::White);

            RangeRect.x = X1 + 12;
            RangeRect.y = Y1 - 8;
        }

        FormatNumber(Range, Distance);
        Print(RangeRect.x, RangeRect.y, "%s", Range);
    }
}

void TacticalView::SendMove()
{
    if (!ProjectorPtr || !bShowMove || !MsgShip)
        return;

    if (GetMouseLoc3D())
    {
        SimElement* Elem = MsgShip->GetElement();
        RadioMessage* Msg = new RadioMessage(Elem, PlayerShip, RadioMessageAction::MOVE_PATROL);

        FVector Dest = MoveLoc;
        Dest.Y += (float)MoveAlt;

        Msg->SetLocation(Dest);
        RadioTraffic::Transmit(Msg);
        HUDSounds::PlaySound(HUDSounds::SND_TAC_ACCEPT);
    }
}

void TacticalView::DrawAction()
{
    if (!ProjectorPtr || ShowAction == RadioMessageAction::NONE || !MsgShip || !PlayerShip)
        return;

    FVector Origin = MsgShip->Location();
    ProjectorPtr->Transform(Origin);
    ProjectorPtr->Project(Origin);

    const int32 X0 = (int32)Origin.X;
    const int32 Y0 = (int32)Origin.Y;

    const int32 MX = MouseAction.X;
    const int32 MY = MouseAction.Y;

    int32 R = 10;

    int32 Enemy = 2;
    if (PlayerShip->GetIFF() > 1)
        Enemy = 1;

    Ship* Tgt = WillSelectAt(MX, MY);
    int32 TgtIFF = Tgt ? Tgt->GetIFF() : 0;

    FColor C = FColor::White;

    switch (ShowAction)
    {
    case RadioMessageAction::ATTACK:
    case RadioMessageAction::BRACKET:
        C = Ship::IFFColor(Enemy);
        if (Tgt && (TgtIFF == PlayerShip->GetIFF() || TgtIFF == 0))
            R = 0;
        break;

    case RadioMessageAction::ESCORT:
    case RadioMessageAction::DOCK_WITH:
        C = PlayerShip->MarkerColor();
        if (Tgt)
        {
            if (TgtIFF == Enemy)
                R = 0;

            if (ShowAction == RadioMessageAction::DOCK_WITH && Tgt->GetHangar() == 0)
                R = 0;
        }
        break;

    default:
        if (Tgt && TgtIFF == PlayerShip->GetIFF())
            R = 0;
        break;
    }

    if (Tgt && R)
    {
        if ((Game::RealTime() / 200) & 1)
            R = 20;
        else
            R = 15;
    }

    if (R)
    {
        gInvalidAction = false;
        DrawEllipse(MX - R, MY - R, MX + R, MY + R, C);
    }
    else
    {
        gInvalidAction = true;
        DrawLine(MX - 10, MY - 10, MX + 10, MY + 10, C);
        DrawLine(MX + 10, MY - 10, MX - 10, MY + 10, C);
    }

    DrawLine(X0, Y0, MX, MY, C);
}

void TacticalView::SendAction()
{
    if (ShowAction == RadioMessageAction::NONE || !MsgShip || gInvalidAction)
    {
        HUDSounds::PlaySound(HUDSounds::SND_TAC_REJECT);
        return;
    }

    Ship* Tgt = WillSelectAt(MouseAction.X, MouseAction.Y);
    if (Tgt)
    {
        SimElement* Elem = MsgShip->GetElement();
        RadioMessage* Msg = new RadioMessage(Elem, PlayerShip, ShowAction);
        Msg->AddTarget(Tgt);

        RadioTraffic::Transmit(Msg);
        HUDSounds::PlaySound(HUDSounds::SND_TAC_ACCEPT);
    }
    else
    {
        HUDSounds::PlaySound(HUDSounds::SND_TAC_REJECT);
    }
}

void TacticalView::ProcessRadioAction(RadioMessageAction action)
{
    switch (action)
    {
    case RadioMessageAction::MOVE_PATROL:
        show_move = true;
        base_alt = 0;
        move_alt = 0;

        if (msg_ship)
            base_alt = msg_ship->Location().Y; // UE FVector => .Y
        break;

    case RadioMessageAction::ATTACK:
    case RadioMessageAction::BRACKET:
    case RadioMessageAction::ESCORT:
    case RadioMessageAction::IDENTIFY:
    case RadioMessageAction::DOCK_WITH:
        ShowRadioAction = action; // make show_action type RadioMessageAction (recommended)
        break;

    case RadioMessageAction::WEP_HOLD:
    case RadioMessageAction::RESUME_MISSION:
    case RadioMessageAction::RTB:
    case RadioMessageAction::GO_DIAMOND:
    case RadioMessageAction::GO_SPREAD:
    case RadioMessageAction::GO_BOX:
    case RadioMessageAction::GO_TRAIL:
    case RadioMessageAction::GO_EMCON1:
    case RadioMessageAction::GO_EMCON2:
    case RadioMessageAction::GO_EMCON3:
    case RadioMessageAction::LAUNCH_PROBE:
        if (msg_ship)
        {
            SimElement* elem = msg_ship->GetElement();
            RadioMessage* msg = new RadioMessage(elem, ship, action);
            if (msg)
                RadioTraffic::Transmit(msg);
        }
        else if (ship)
        {
            if (action == RadioMessageAction::GO_EMCON1)           ship->SetEMCON(1);
            else if (action == RadioMessageAction::GO_EMCON2)      ship->SetEMCON(2);
            else if (action == RadioMessageAction::GO_EMCON3)      ship->SetEMCON(3);
            else if (action == RadioMessageAction::LAUNCH_PROBE)   ship->LaunchProbe();
        }
        break;

    default:
        break;
    }
}


void TacticalView::ProcessViewAction(TacticalViewMenu action)
{
    Starshatter* stars = Starshatter::GetInstance();

    switch (action)
    {
    case TacticalViewMenu::FORWARD:
        stars->PlayerCam(CameraManager::MODE_COCKPIT);
        break;

    case TacticalViewMenu::CHASE:
        stars->PlayerCam(CameraManager::MODE_CHASE);
        break;

    case TacticalViewMenu::PADLOCK:
        stars->PlayerCam(CameraManager::MODE_TARGET);
        break;

    case TacticalViewMenu::ORBIT:
        stars->PlayerCam(CameraManager::MODE_ORBIT);
        break;

    case TacticalViewMenu::NAV:
        gamescreen->ShowNavDlg();
        break;

    case TacticalViewMenu::WEP:
        gamescreen->ShowWeaponsOverlay();
        break;

    case TacticalViewMenu::ENG:
        gamescreen->ShowEngDlg();
        break;

    case TacticalViewMenu::INS:
        HUDView::GetInstance()->CycleHUDInst();
        break;

    case TacticalViewMenu::FLT:
        gamescreen->ShowFltDlg();
        break;

    case TacticalViewMenu::CMD:
        if (ship && ship->IsStarship())
            ship->CommandMode();
        break;

    case TacticalViewMenu::QUANTUM:
        if (sim)
        {
            Ship* s = msg_ship ? msg_ship : ship;

            if (s && s->GetQuantumDrive())
            {
                QuantumDrive* quantum = s->GetQuantumDrive();
                if (quantum)
                {
                    MenuItem* menu_item = menu_view->GetMenuItem();
                    const FString& rgn_name = menu_item->GetText();
                    SimRegion* rgn = sim->FindRegion(rgn_name);

                    if (rgn)
                    {
                        if (s == ship)
                        {
                            quantum->SetDestination(rgn, Point(0, 0, 0));
                            quantum->Engage();
                        }
                        else if (msg_ship)
                        {
                            SimElement* elem = msg_ship->GetElement();
                            RadioMessage* msg = new RadioMessage(elem, ship, RadioMessageAction::QUANTUM_TO);
                            if (msg)
                            {
                                Text LegacyName(TCHAR_TO_ANSI(*rgn_name));
                                msg->SetInfo(LegacyName); 
                                RadioTraffic::Transmit(msg);
                            }
                        }
                    }
                }
            }
        }
        break;

    case TacticalViewMenu::FARCAST:
        if (sim && msg_ship)
        {
            MenuItem* menu_item = menu_view->GetMenuItem();
            const FString& rgn_name = menu_item->GetText();
            Text legacyName(TCHAR_TO_ANSI(*rgn_name));
            SimRegion* rgn = sim->FindRegion(legacyName);

            if (rgn)
            {
                SimElement* elem = msg_ship->GetElement();
                RadioMessage* msg = new RadioMessage(elem, ship, RadioMessageAction::FARCAST_TO);
                if (msg)
                {
                    msg->SetInfo(Text(TCHAR_TO_ANSI(*rgn_name)));
                    RadioTraffic::Transmit(msg);
                }
            }
        }
        break;

    default:
        break;
    }
}

void TacticalView::ProcessMenuItem(int32 action)
{
    // MapView dispatch rule:
    if (action >= 1000)
    {
        ProcessViewAction(static_cast<TacticalViewMenu>(action));
    }
    else
    {
        ProcessRadioAction(static_cast<RadioMessageAction>(action));
    }
}