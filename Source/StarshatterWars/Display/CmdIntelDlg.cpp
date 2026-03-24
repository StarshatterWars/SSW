/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         CmdIntelDlg.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UCmdIntelDlg implementation (Unreal port)
*/

#include "CmdIntelDlg.h"

// UMG
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ListView.h"
#include "Components/RichTextBlock.h"

// Starshatter core
#include "Starshatter.h"
#include "Campaign.h"
#include "CombatEvent.h"
#include "CombatGroup.h"
#include "Sim.h"
#include "CameraManager.h"
#include "FormatUtil.h"
#include "Mouse.h"

// Your campaign screen
#include "CmpnScreen.h"

UCmdIntelDlg::UCmdIntelDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UCmdIntelDlg::NativeConstruct()
{
    Super::NativeConstruct();

    BindFormWidgets();

    Stars = Starshatter::GetInstance();
    CampaignPtr = Campaign::GetCampaign();
    if (CampaignPtr)
        UpdateTime = CampaignPtr->GetUpdateTime();

    // Buttons
    if (btn_save)     btn_save->OnClicked.AddDynamic(this, &UCmdIntelDlg::OnSaveClicked);
    if (btn_exit)     btn_exit->OnClicked.AddDynamic(this, &UCmdIntelDlg::OnExitClicked);

    if (btn_orders)   btn_orders->OnClicked.AddDynamic(this, &UCmdIntelDlg::OnModeOrdersClicked);
    if (btn_theater)  btn_theater->OnClicked.AddDynamic(this, &UCmdIntelDlg::OnModeTheaterClicked);
    if (btn_forces)   btn_forces->OnClicked.AddDynamic(this, &UCmdIntelDlg::OnModeForcesClicked);
    if (btn_intel)    btn_intel->OnClicked.AddDynamic(this, &UCmdIntelDlg::OnModeIntelClicked);
    if (btn_missions) btn_missions->OnClicked.AddDynamic(this, &UCmdIntelDlg::OnModeMissionsClicked);

    if (btn_play)     btn_play->OnClicked.AddDynamic(this, &UCmdIntelDlg::OnPlayClicked);

    // List click
    if (lst_news)
        lst_news->OnItemClicked().AddUObject(this, &UCmdIntelDlg::OnNewsItemClicked);

    // Initial visibility
    if (btn_play) btn_play->SetVisibility(ESlateVisibility::Collapsed);
    if (mov_news) mov_news->SetVisibility(ESlateVisibility::Collapsed);
}

void UCmdIntelDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    ExecFrame();
}

void UCmdIntelDlg::SetManager(UCmpnScreen* InManager)
{
    Manager = InManager;
}

void UCmdIntelDlg::BindFormWidgets()
{
    // Intentionally empty if you are using BindWidgetOptional.
    // Keep this function so you can add explicit binding by ID later if desired.
}

void UCmdIntelDlg::SetParentCmdDlg(UCmdDlg* InParentCmdDlg)
{
    ParentCmdDlg = InParentCmdDlg;
}

void UCmdIntelDlg::ShowIntelDlg()
{
    Mode = ECOMMAND_MODE::MODE_INTEL;

    // Mirror legacy: ShowCmdDlg header
    CampaignPtr = Campaign::GetCampaign();
    Stars = Starshatter::GetInstance();

    if (txt_name)
    {
        if (CampaignPtr)
            txt_name->SetText(FText::FromString(CampaignPtr->Name()));
        else
            txt_name->SetText(FText::FromString(TEXT("No Campaign Selected")));
    }

    // Hide movie and play button by default
    if (btn_play) btn_play->SetVisibility(ESlateVisibility::Collapsed);
    if (mov_news) mov_news->SetVisibility(ESlateVisibility::Collapsed);

    SetVisibility(ESlateVisibility::Visible);
}

void UCmdIntelDlg::ExecFrame()
{
    ExecHeaderFrame();
    RebuildNewsListIfCampaignChanged();
    AppendNewEventsIfAny();

    // Cutscene start countdown (legacy start_scene)
    if (StartSceneCountdown > 0)
    {
        ShowMovie();
        --StartSceneCountdown;

        if (StartSceneCountdown == 0)
        {
            if (Stars && CampaignPtr && !EventScene.IsEmpty())
            {
                Stars->ExecCutscene(TCHAR_TO_UTF8(*EventScene), CampaignPtr->Path());

                if (Stars->InCutscene())
                {
                    // Legacy camera view wiring happened here.
                    // In UE, your mov_news widget should already be rendering the cutscene viewport.
                    // If you have a custom viewport widget, update its scene/camera here.
                    Sim* SimPtr = Sim::GetSim();
                    (void)SimPtr;
                }
            }

            EventScene.Empty();
        }
    }
    else
    {
        if (Stars && Stars->InCutscene())
            ShowMovie();
        else
            HideMovie();
    }
}

void UCmdIntelDlg::ExecHeaderFrame()
{
    if (!CampaignPtr)
        CampaignPtr = Campaign::GetCampaign();

    if (!CampaignPtr)
        return;

    // Group
    if (txt_group)
    {
        CombatGroup* G = CampaignPtr->GetPlayerGroup();
        if (G)
            txt_group->SetText(FText::FromString(UTF8_TO_TCHAR(G->GetDescription())));
    }

    // Score
    if (txt_score)
    {
        const int32 TeamScore = CampaignPtr->GetPlayerTeamScore();
        const FString ScoreStr = FString::Printf(TEXT("Team Score: %d"), TeamScore);
        txt_score->SetText(FText::FromString(ScoreStr));
    }

    // Time
    if (txt_time)
    {
        char DayTime[32] = { 0 };
        FormatDayTime(DayTime, CampaignPtr->GetTime());
        txt_time->SetText(FText::FromString(UTF8_TO_TCHAR(DayTime)));
    }

    // Intel unread count label on the Intel tab button (optional):
    // If your Intel button has a child text block, expose it and set it here.
}

void UCmdIntelDlg::RebuildNewsListIfCampaignChanged()
{
    Campaign* Current = Campaign::GetCampaign();
    if (!Current)
        return;

    const double CurrentUpdateTime = Current->GetUpdateTime();

    if (CampaignPtr != Current || UpdateTime != CurrentUpdateTime)
    {
        CampaignPtr = Current;
        UpdateTime = CurrentUpdateTime;

        if (lst_news)
            lst_news->ClearListItems();

        ClearNewsDetails();
    }
}

void UCmdIntelDlg::AppendNewEventsIfAny()
{
    if (!CampaignPtr || !lst_news)
        return;

    List<CombatEvent>& Events = CampaignPtr->GetEvents();

    // ListView items count
    const int32 Existing = lst_news->GetNumItems();
    const int32 Total = Events.size();

    bool bAutoScroll = false;

    if (Total > Existing)
    {
        for (int32 i = Existing; i < Total; ++i)
        {
            CombatEvent* Info = Events[i];
            if (!Info) continue;

            UCmdIntelNewsItem* Item = NewObject<UCmdIntelNewsItem>(this);

            Item->UnreadMark = Info->Visited() ? TEXT(" ") : TEXT("*");

            char Dateline[32] = { 0 };
            FormatDayTime(Dateline, Info->Time());
            Item->Date = UTF8_TO_TCHAR(Dateline);

            Item->Title = UTF8_TO_TCHAR(Info->Title());
            Item->Loc = UTF8_TO_TCHAR(Info->Region());
            Item->Source = UTF8_TO_TCHAR(
                Game::GetText(
                    TCHAR_TO_UTF8(*Info->GetEventSourceName())
                )
            );
            Item->EventPtr = Info;

            lst_news->AddItem(Item);

            if (!Info->Visited())
                bAutoScroll = true;
        }

        if (bAutoScroll)
            AutoScrollToFirstUnreadIfNeeded();
    }
    else if (Total < Existing)
    {
        // Campaign likely reloaded; rebuild list
        lst_news->ClearListItems();

        bool bHasUnread = false;
        for (int32 i = 0; i < Total; ++i)
        {
            CombatEvent* Info = Events[i];
            if (!Info) continue;

            UCmdIntelNewsItem* Item = NewObject<UCmdIntelNewsItem>(this);

            Item->UnreadMark = Info->Visited() ? TEXT(" ") : TEXT("*");

            char Dateline[32] = { 0 };
            FormatDayTime(Dateline, Info->Time());
            Item->Date = UTF8_TO_TCHAR(Dateline);

            Item->Title = UTF8_TO_TCHAR(Info->Title());
            Item->Loc = UTF8_TO_TCHAR(Info->Region());
            Item->Source = UTF8_TO_TCHAR(
                Game::GetText(
                    TCHAR_TO_UTF8(*Info->GetEventSourceName())
                )
            );
            Item->EventPtr = Info;

            lst_news->AddItem(Item);

            if (!Info->Visited())
                bHasUnread = true;
        }

        ClearNewsDetails();

        if (bHasUnread)
            AutoScrollToFirstUnreadIfNeeded();
    }
}

void UCmdIntelDlg::AutoScrollToFirstUnreadIfNeeded()
{
    if (!lst_news)
        return;

    const int32 Num = lst_news->GetNumItems();
    for (int32 i = 0; i < Num; ++i)
    {
        UObject* Obj = lst_news->GetItemAt(i);
        UCmdIntelNewsItem* Item = Cast<UCmdIntelNewsItem>(Obj);
        if (Item && Item->UnreadMark == TEXT("*"))
        {
            lst_news->ScrollIndexIntoView(i);
            break;
        }
    }
}

void UCmdIntelDlg::ClearNewsDetails()
{
    if (txt_news)
        txt_news->SetText(FText::GetEmpty());

    if (img_news && DefaultNewsTexture)
        img_news->SetBrushFromTexture(DefaultNewsTexture, true);

    if (btn_play)
        btn_play->SetVisibility(ESlateVisibility::Collapsed);
}

CombatEvent* UCmdIntelDlg::GetSelectedEvent(int32& OutSelectedIndex) const
{
    OutSelectedIndex = -1;

    if (!lst_news)
        return nullptr;

    UObject* Selected = lst_news->GetSelectedItem();
    if (!Selected)
        return nullptr;

    OutSelectedIndex = lst_news->GetIndexForItem(Selected);

    UCmdIntelNewsItem* Item = Cast<UCmdIntelNewsItem>(Selected);
    if (!Item)
        return nullptr;

    return Item->EventPtr;
}

void UCmdIntelDlg::OnNewsItemClicked(UObject* ItemObj)
{
    if (!lst_news)
        return;

    // Select it (ListView click does not always select depending on settings)
    lst_news->SetSelectedItem(ItemObj);

    int32 Index = -1;
    CombatEvent* EventPtr = GetSelectedEvent(Index);

    ShowSelectedEvent(EventPtr, Index);
}

void UCmdIntelDlg::ShowSelectedEvent(CombatEvent* EventPtr, int32 SelectedIndex)
{
    if (!EventPtr)
    {
        ClearNewsDetails();
        return;
    }

    // Build rich text similar to legacy markup.
    // IMPORTANT: UMG RichTextBlock uses decorators; you will need matching styles in your RichTextBlock.
    // If you do not have decorators/styles, use a plain TextBlock or strip tags.
    FString Info;
    Info += TEXT("<Title>");
    Info += UTF8_TO_TCHAR(EventPtr->Title());
    Info += TEXT("</>\n\n");
    Info += UTF8_TO_TCHAR(EventPtr->Information());

    if (txt_news)
    {
        txt_news->SetText(FText::FromString(Info));
    }

    // Mark unread column
    if (SelectedIndex >= 0)
    {
        UObject* Obj = lst_news->GetItemAt(SelectedIndex);
        UCmdIntelNewsItem* Item = Cast<UCmdIntelNewsItem>(Obj);
        if (Item)
        {
            Item->UnreadMark = TEXT(" ");
            // Force refresh the row
            lst_news->RequestRefresh();
        }
    }

    // Image
    if (img_news)
    {
        // Legacy used event->Image() (Bitmap). In UE, you likely have an atlas/texture mapping by name.
        // For now, fall back to default if you do not have an asset resolver.
        if (DefaultNewsTexture)
            img_news->SetBrushFromTexture(DefaultNewsTexture, true);
    }

    // Play button visibility
    const bool bHasScene = (EventPtr->SceneFile() && *EventPtr->SceneFile());
    if (btn_play)
        btn_play->SetVisibility(bHasScene ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

    // Autoplay if not visited and play is possible
    if (!EventPtr->Visited() && bHasScene && btn_play && btn_play->GetIsEnabled())
    {
        OnPlayClicked();
    }

    EventPtr->SetVisited(true);
}

void UCmdIntelDlg::OnPlayClicked()
{
    if (!lst_news)
        return;

    int32 Index = -1;
    CombatEvent* EventPtr = GetSelectedEvent(Index);
    if (!EventPtr)
        return;

    if (!EventPtr->SceneFile() || !*EventPtr->SceneFile())
        return;

    EventScene = UTF8_TO_TCHAR(EventPtr->SceneFile());
    StartSceneCountdown = 2;

    ShowMovie();
}

void UCmdIntelDlg::ShowMovie()
{
    if (mov_news)
        mov_news->SetVisibility(ESlateVisibility::Visible);

    if (img_news) img_news->SetVisibility(ESlateVisibility::Collapsed);
    if (txt_news) txt_news->SetVisibility(ESlateVisibility::Collapsed);
    if (btn_play) btn_play->SetVisibility(ESlateVisibility::Collapsed);
}

void UCmdIntelDlg::HideMovie()
{
    // Determine if current selected event is playable
    bool bPlay = false;

    int32 Index = -1;
    CombatEvent* EventPtr = GetSelectedEvent(Index);
    if (EventPtr && EventPtr->SceneFile() && *EventPtr->SceneFile())
        bPlay = true;

    if (mov_news)
        mov_news->SetVisibility(ESlateVisibility::Collapsed);

    if (img_news) img_news->SetVisibility(ESlateVisibility::Visible);
    if (txt_news) txt_news->SetVisibility(ESlateVisibility::Visible);

    if (btn_play)
        btn_play->SetVisibility(bPlay ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

// --------------------------------------------------------------------
// Routing
// --------------------------------------------------------------------

void UCmdIntelDlg::SetModeAndRoute(ECOMMAND_MODE InMode)
{
    Mode = InMode;

    if (!Manager)
    {
        UE_LOG(LogTemp, Warning, TEXT("CmdIntelDlg: Manager is null (SetModeAndRoute)."));
        return;
    }

    switch (Mode)
    {
    case ECOMMAND_MODE::MODE_ORDERS:   Manager->ShowCmdOrdersDlg();   break;
    case ECOMMAND_MODE::MODE_THEATER:  Manager->ShowCmdTheaterDlg();  break;
    case ECOMMAND_MODE::MODE_FORCES:   Manager->ShowCmdForceDlg();    break;
    case ECOMMAND_MODE::MODE_INTEL:    Manager->ShowCmdIntelDlg();    break;
    case ECOMMAND_MODE::MODE_MISSIONS: Manager->ShowCmdMissionsDlg(); break;
    default:                               Manager->ShowCmdOrdersDlg();   break;
    }
}

void UCmdIntelDlg::OnModeOrdersClicked() { SetModeAndRoute(ECOMMAND_MODE::MODE_ORDERS); }
void UCmdIntelDlg::OnModeTheaterClicked() { SetModeAndRoute(ECOMMAND_MODE::MODE_THEATER); }
void UCmdIntelDlg::OnModeForcesClicked() { SetModeAndRoute(ECOMMAND_MODE::MODE_FORCES); }
void UCmdIntelDlg::OnModeIntelClicked() { SetModeAndRoute(ECOMMAND_MODE::MODE_INTEL); }
void UCmdIntelDlg::OnModeMissionsClicked() { SetModeAndRoute(ECOMMAND_MODE::MODE_MISSIONS); }

void UCmdIntelDlg::OnSaveClicked()
{
    if (Manager)
        Manager->ShowCmpFileDlg();
}

void UCmdIntelDlg::OnExitClicked()
{
    if (Stars)
    {
        Mouse::Show(false);
        Stars->SetGameMode(EGameMode::MENU);
    }
}
