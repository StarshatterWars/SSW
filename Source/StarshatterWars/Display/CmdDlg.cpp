/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         CmdDlg.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UCmdDlg implementation (Unreal port)
*/

#include "CmdDlg.h"

// UMG:
#include "Components/TextBlock.h"
#include "Components/Button.h"

// Starshatter core:
#include "Campaign.h"
#include "CombatGroup.h"
#include "Starshatter.h"
#include "FormatUtil.h"
#include "Mouse.h"

// Your campaign screen widget (port of CmpnScreen):
#include "CmpnScreen.h"
#include "CmpFileDlg.h"
#include "MenuButton.h"
#include "GameStructs.h"

#include "SelectableButtonGroup.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBoxSlot.h"

#include "StarshatterPlayerSubsystem.h"
#include "StarshatterGameDataSubsystem.h"
#include "StarshatterUIStyleSubsystem.h"
#include "StarshatterEnvironmentSubsystem.h"

#include "CmdOrdersDlg.h"
#include "CmdMissionsDlg.h"
#include "CmdForceDlg.h"
#include "CmdIntelDlg.h"
#include "CmdTheaterDlg.h"
#include "Mission.h"
#include "GameStructs_UI.h"

#include "MissionListObject.h"

#include "TimerSubsystem.h"
#include "FormattingUtils.h"
#include "SSWGameInstance.h"

void UCmdDlg::NativeConstruct()
{
    Super::NativeConstruct();

    UGameInstance* GI = GetGameInstance();
    if (!GI)
    {
        UE_LOG(LogTemp, Error, TEXT("[CmdDlg] NativeConstruct: GameInstance is NULL"));
        return;
    }

    UStarshatterPlayerSubsystem* PlayerSS = GI->GetSubsystem<UStarshatterPlayerSubsystem>();
    if (!PlayerSS)
    {
        UE_LOG(LogTemp, Error, TEXT("[CmdDlg] NativeConstruct: PlayerSubsystem is NULL"));
        return;
    }

    UStarshatterGameDataSubsystem* DataSubsystem = GI->GetSubsystem<UStarshatterGameDataSubsystem>();
    if (!DataSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("[CmdDlg] NativeConstruct: GameDataSubsystem is NULL"));
        return;
    }

    if (!PlayerSS->HasLoaded())
    {
        PlayerSS->LoadPlayer();
    }

    const FS_PlayerGameInfo& PlayerInfo = PlayerSS->GetPlayerInfo();

    HandleGameTimers();

    const FS_Campaign* FoundCampaign =
        DataSubsystem->GetCampaignByIndex1Based(PlayerInfo.Campaign);

    if (FoundCampaign)
    {
        CurrentCampaignData = *FoundCampaign;
        bHasCurrentCampaign = true;

        UE_LOG(LogTemp, Log,
            TEXT("[CmdDlg] Loaded Campaign: %s (Index=%d)"),
            *CurrentCampaignData.Name,
            PlayerInfo.Campaign);
    }
    else
    {
        bHasCurrentCampaign = false;

        UE_LOG(LogTemp, Warning,
            TEXT("[CmdDlg] NativeConstruct: No campaign found for index %d"),
            PlayerInfo.Campaign);
    }

    if (TitleText)
    {
        TitleText->SetText(FText::FromString(TEXT("CAMPAIGN OPERATIONS")));
    }

    if (!MenuButtonClass || !MenuToggleGroup || !MenuButtonContainer)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[CmdDlg] Missing bindings: MenuButtonClass=%s MenuToggleGroup=%s MenuButtonContainer=%s"),
            *GetNameSafe(MenuButtonClass.Get()),
            *GetNameSafe(MenuToggleGroup),
            *GetNameSafe(MenuButtonContainer));
        return;
    }

    MenuButtonContainer->ClearChildren();
    AllMenuButtons.Empty();

    for (int32 i = 0; i < 5; ++i)
    {
        UMenuButton* NewButton = CreateWidget<UMenuButton>(this, MenuButtonClass);
        if (!NewButton)
        {
            continue;
        }

        NewButton->SetLayoutMode(ELayoutMode::FillWidth);
        NewButton->SetButtonSize(256.0f, 32.0f);
        NewButton->SetLabelFontSizeValue(16);
        NewButton->SetMenuOption(MenuItems[i]);
        NewButton->SetButtonText(FText::FromString(MenuItems[i]).ToUpper());

        if (UVerticalBoxSlot* VBoxSlot = MenuButtonContainer->AddChildToVerticalBox(NewButton))
        {
            VBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 0.f));
            VBoxSlot->SetHorizontalAlignment(HAlign_Fill);
            VBoxSlot->SetVerticalAlignment(VAlign_Center);
        }

        MenuToggleGroup->RegisterButton(NewButton);

        NewButton->OnSelected.RemoveDynamic(this, &UCmdDlg::OnMenuToggleSelected);
        NewButton->OnSelected.AddDynamic(this, &UCmdDlg::OnMenuToggleSelected);

        NewButton->OnHovered.RemoveDynamic(this, &UCmdDlg::OnMenuToggleHovered);
        NewButton->OnHovered.AddDynamic(this, &UCmdDlg::OnMenuToggleHovered);

        AllMenuButtons.Add(NewButton);
    }

    if (ReturnButton)
    {
        ReturnButton->OnClicked.RemoveDynamic(this, &UCmdDlg::OnCancelButtonClicked);
        ReturnButton->OnClicked.AddDynamic(this, &UCmdDlg::OnCancelButtonClicked);

        ReturnButton->OnHovered.RemoveDynamic(this, &UCmdDlg::OnCancelButtonHovered);
        ReturnButton->OnHovered.AddDynamic(this, &UCmdDlg::OnCancelButtonHovered);

        ReturnButton->OnUnhovered.RemoveDynamic(this, &UCmdDlg::OnCancelButtonUnHovered);
        ReturnButton->OnUnhovered.AddDynamic(this, &UCmdDlg::OnCancelButtonUnHovered);

        if (ReturnButtonText)
        {
            ReturnButtonText->SetText(FText::FromString(TEXT("CANCEL")));
        }
    }

    if (MissionButton)
    {
        MissionButton->OnClicked.RemoveDynamic(this, &UCmdDlg::OnMissionButtonClicked);
        MissionButton->OnClicked.AddDynamic(this, &UCmdDlg::OnMissionButtonClicked);

        MissionButton->OnHovered.RemoveDynamic(this, &UCmdDlg::OnMissionButtonHovered);
        MissionButton->OnHovered.AddDynamic(this, &UCmdDlg::OnMissionButtonHovered);

        MissionButton->OnUnhovered.RemoveDynamic(this, &UCmdDlg::OnMissionButtonUnHovered);
        MissionButton->OnUnhovered.AddDynamic(this, &UCmdDlg::OnMissionButtonUnHovered);

        if (MissionButtonText)
        {
            MissionButtonText->SetText(FText::FromString(TEXT("ACCEPT")));
        }
    }

    if (PlayerNameText)
    {
        PlayerNameText->SetText(FText::FromString(PlayerInfo.Name));
        UE_LOG(LogTemp, Log, TEXT("[CmdDlg] Player Name: %s"), *PlayerInfo.Name);
    }

    if (OperationalSwitcher)
    {
        OperationalSwitcher->SetActiveWidgetIndex(0);
    }

    if (bHasCurrentCampaign)
    {
        if (CurrentLocationText)
        {
            CurrentLocationText->SetText(
                FText::FromString(CurrentCampaignData.System + TEXT(" System")).ToUpper());
        }

        if (CampaignNameText)
        {
            CampaignNameText->SetText(
                FText::FromString(CurrentCampaignData.Name));
        }
    }

    if (btn_save) btn_save->OnClicked.AddDynamic(this, &UCmdDlg::OnSaveClicked);
    if (btn_exit) btn_exit->OnClicked.AddDynamic(this, &UCmdDlg::OnExitClicked);

    Stars = Starshatter::GetInstance();
    CampaignPtr = Campaign::GetCampaign();

    const FString Frm = GetLegacyFormText();
    if (!Frm.IsEmpty())
    {
        FString Err;
        FParsedForm Parsed;

        if (ParseLegacyForm(Frm, Parsed, Err))
        {
            ParsedForm = Parsed;
            ApplyLegacyFormDefaults(ParsedForm);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[CmdDlg] ParseLegacyForm failed: %s"), *Err);
        }
    }
}

void UCmdDlg::NativePreConstruct()
{
    Super::NativePreConstruct();

    if (CmdOrdersPanel)
    {
        CmdOrdersPanel->SetParentCmdDlg(this);
        CmdOrdersPanel->ShowOrdersDlg();
    }

    if (CmdMissionsPanel)
    {
        CmdMissionsPanel->SetParentCmdDlg(this);
    }

    if (CmdForcesPanel)
    {
        CmdForcesPanel->SetParentCmdDlg(this);
    }

    if (CmdIntelPanel)
    {
        CmdIntelPanel->SetParentCmdDlg(this);
    }

    if (CmdTheaterPanel)
    {
        CmdTheaterPanel->SetParentCmdDlg(this);
    }
}

void UCmdDlg::NativeOnInitialized()
{
}

void UCmdDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    ExecFrame(InDeltaTime);
}

UCmdDlg::UCmdDlg(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UCmdDlg::SetMenuManager(UMenuScreen* InManager)
{
    manager = InManager;
}

void UCmdDlg::InitializeDlg(UMenuScreen* InManager)
{
    manager = InManager;
}

void UCmdDlg::OnMenuButtonSelected(UMenuButton* SelectedButton)
{
    USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
    SSWInstance->PlayAcceptSound(this);

    for (UMenuButton* Button : AllMenuButtons)
    {
        if (Button)
        {
            Button->SetSelected(Button == SelectedButton);
        }
    }
}

void UCmdDlg::OnMenuToggleSelected(UMenuButton* SelectedButton)
{
    if (!SelectedButton) return;

    UE_LOG(LogTemp, Log, TEXT("Selected button: %s"), *GetNameSafe(SelectedButton));

    const FString& MenuOption = SelectedButton->MenuOption;

    if (MenuOption == MenuItems[0])
    {
        CurrentScreen = ECmdScreen::Orders;
        LoadOrdersInfo();
    }
    else if (MenuOption == MenuItems[1])
    {
        CurrentScreen = ECmdScreen::Theater;
        LoadTheaterInfo();
    }
    else if (MenuOption == MenuItems[2])
    {
        CurrentScreen = ECmdScreen::Forces;
        LoadForcesInfo();
    }
    else if (MenuOption == MenuItems[3])
    {
        CurrentScreen = ECmdScreen::Intel;
        LoadIntelInfo();
    }
    else if (MenuOption == MenuItems[4])
    {
        CurrentScreen = ECmdScreen::Missions;
        LoadMissionsInfo();
    }

    UpdateMissionButton();
}

void UCmdDlg::OnMenuToggleHovered(UMenuButton* HoveredButton)
{
    if (!HoveredButton) return;

    UE_LOG(LogTemp, Log, TEXT("Hovered over: %s"), *HoveredButton->MenuOption);
    USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
    SSWInstance->PlayHoverSound(this);
}

void UCmdDlg::BindFormWidgets()
{
    BindButton(1, btn_save);
    BindButton(2, btn_exit);
}

void UCmdDlg::SetManager(UCmpnScreen* InManager)
{
    CmpnScreen = InManager;
}

void UCmdDlg::ShowCmdDlg()
{
    CampaignPtr = Campaign::GetCampaign();

    if (txt_name)
    {
        if (CampaignPtr)
            txt_name->SetText(FText::FromString(CampaignPtr->GetName()));
        else
            txt_name->SetText(FText::FromString(TEXT("No Campaign Selected")));
    }

    if (CampaignPtr)
    {
        const bool bTraining = CampaignPtr->IsTraining();

        if (btn_save)
            btn_save->SetIsEnabled(!bTraining);
    }

    SetVisibility(ESlateVisibility::Visible);

    if (CurrentScreen == ECmdScreen::None)
    {
        CurrentScreen = ECmdScreen::Orders;
        LoadOrdersInfo();
    }

    UpdateMissionButton();

    // Prime intel data immediately on command dialog open
    IntelRefreshCounterSeconds = 0;
    RefreshIntelDataBackground();
}

void UCmdDlg::ExecFrame(double DeltaTime)
{
    if (!CampaignPtr)
        CampaignPtr = Campaign::GetCampaign();

    if (!CampaignPtr)
        return;

    if (CurrentUnitText)
    {
        CombatGroup* G = CampaignPtr->GetPlayerGroup();
        if (G)
            CurrentUnitText->SetText(FText::FromString(G->GetDescription()));
    }

    if (PlayerScoreText)
    {
        const int32 TeamScore = CampaignPtr->GetPlayerTeamScore();
        const FString ScoreStr = FString::Printf(TEXT("Team Score: %d"), TeamScore);
        PlayerScoreText->SetText(FText::FromString(ScoreStr));
        PlayerScoreText->SetJustification(ETextJustify::Right);
    }

    if (CampaignTPlusText)
    {
        const double T = CampaignPtr->GetTime();

        char DayTime[32] = { 0 };
        FormatDayTime(DayTime, T);

        CampaignTPlusText->SetText(FText::FromString(UTF8_TO_TCHAR(DayTime)));
    }

    const int32 Unread = CampaignPtr->CountNewEvents();

    RefreshCommandButtons();

    if (CmdOrdersPanel)
    {
        CmdOrdersPanel->ShowOrdersDlg();
    }

    if (AllMenuButtons.IsValidIndex(3) && AllMenuButtons[3])
    {
        if (UTextBlock* Label = Cast<UTextBlock>(AllMenuButtons[3]->GetWidgetFromName(TEXT("Label"))))
        {
            if (Unread > 0)
            {
                Label->SetText(FText::FromString(FString::Printf(TEXT("INTEL (%d)"), Unread)));
            }
            else
            {
                Label->SetText(FText::FromString(TEXT("INTEL")));
            }
        }
    }

    UpdateMissionButton();
}

void UCmdDlg::OnSaveClicked()
{
    if (!CampaignPtr)
        CampaignPtr = Campaign::GetCampaign();

    if (CampaignPtr && CmpnScreen)
    {
        CmpnScreen->ShowCmpFileDlg();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("CmdDlg: CampaignPtr or CmpnScreen is null (OnSaveClicked)."));
    }
}

void UCmdDlg::OnExitClicked()
{
    if (Stars)
    {
        Mouse::Show(false);
        Stars->SetGameMode(EGameMode::MENU);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("CmdDlg: Starshatter instance is null (OnExitClicked)."));
    }
}

void UCmdDlg::OnCancelButtonClicked()
{
    SetDialogInputEnabled(false);
    SetVisibility(ESlateVisibility::Hidden);

    Mouse::Show(true);

    if (manager)
    {
        manager->ShowCampaignSelectDlg();
    }
    else
    {
        HideDlg();
    }
}

void UCmdDlg::OnCancelButtonHovered()
{
    USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
    SSWInstance->PlayHoverSound(this);
}

void UCmdDlg::OnCancelButtonUnHovered()
{
}

void UCmdDlg::OnMissionButtonClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("[CmdDlg] OnMissionButtonClicked: BEGIN"));

    UStarshatterEnvironmentSubsystem* EnvSubsystem =
        GetGameInstance() ? GetGameInstance()->GetSubsystem<UStarshatterEnvironmentSubsystem>() : nullptr;

    if (!CampaignPtr)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CmdDlg] OnMissionButtonClicked: CampaignPtr is NULL"));
        return;
    }

    if (!CmdMissionsPanel)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CmdDlg] OnMissionButtonClicked: CmdMissionsPanel is NULL"));
        return;
    }

    if (!EnvSubsystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CmdDlg] OnMissionButtonClicked: EnvSubsystem is NULL"));
        return;
    }

    if (!manager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CmdDlg] OnMissionButtonClicked: manager is NULL"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[CmdDlg] OnMissionButtonClicked: Preconditions passed"));

    if (!CmdMissionsPanel->CanAcceptSelectedMission())
    {
        UE_LOG(LogTemp, Warning, TEXT("[CmdDlg] OnMissionButtonClicked: CanAcceptSelectedMission returned false"));
        return;
    }

    const int32 SelectedMissionId = CmdMissionsPanel->GetSelectedMissionId();

    UE_LOG(LogTemp, Warning, TEXT("[CmdDlg] OnMissionButtonClicked: SelectedMissionId=%d"), SelectedMissionId);

    if (SelectedMissionId <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CmdDlg] OnMissionButtonClicked: Invalid SelectedMissionId"));
        return;
    }

    CampaignPtr->SetMissionId(SelectedMissionId);

    Mission* ActiveMission = CampaignPtr->GetMission(SelectedMissionId);

    UE_LOG(LogTemp, Warning, TEXT("[CmdDlg] OnMissionButtonClicked: Campaign mission now=%s"),
        ActiveMission ? TEXT("VALID") : TEXT("NULL"));

    if (ActiveMission)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CmdDlg] OnMissionButtonClicked: ActiveMissionName=%s"),
            ANSI_TO_TCHAR(ActiveMission->GetName()));
    }

    UE_LOG(LogTemp, Warning, TEXT("[CmdDlg] OnMissionButtonClicked: Switching to PLAN mode"));

    EnvSubsystem->SetGameMode(EGameMode::PLAN);
    UE_LOG(LogTemp, Warning, TEXT("[CmdDlg] OnMissionButtonClicked: About to call ShowMissionDlg"));

    manager->ShowMissionDlg();

    UE_LOG(LogTemp, Warning, TEXT("[CmdDlg] OnMissionButtonClicked: Returned from ShowMissionDlg"));
}

void UCmdDlg::OnMissionButtonHovered()
{
    USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
    SSWInstance->PlayHoverSound(this);
}

void UCmdDlg::OnMissionButtonUnHovered()
{
}

void UCmdDlg::ShowDlg()
{
    SetVisibility(ESlateVisibility::Visible);
    RefreshUIFromSubsystem();
}

void UCmdDlg::HideDlg()
{
    SetVisibility(ESlateVisibility::Collapsed);
}

void UCmdDlg::RefreshUIFromSubsystem()
{
}

int32 UCmdDlg::GetIntelEntryCount() const
{
    Campaign* CurrentCampaign = Campaign::GetCampaign();
    if (!CurrentCampaign)
    {
        return 0;
    }

    return CurrentCampaign->GetEvents().size();
}

void UCmdDlg::LoadForcesInfo()
{
    if (ShouldDisableCommandPanels())
    {
        return;
    }

    USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
    SSWInstance->PlayAcceptSound(this);

    if (OperationalSwitcher)
    {
        OperationalSwitcher->SetActiveWidgetIndex(2);
    }

    if (OperationsModeText)
    {
        OperationsModeText->SetText(FText::FromString("FORCES"));
    }

    if (CmdForcesPanel)
    {
        CmdForcesPanel->ShowForceDlg();
    }
}

void UCmdDlg::LoadOrdersInfo()
{
    USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
    SSWInstance->PlayAcceptSound(this);

    if (OperationalSwitcher)
    {
        OperationalSwitcher->SetActiveWidgetIndex(0);
    }

    if (OperationsModeText)
    {
        OperationsModeText->SetText(FText::FromString("ORDERS"));
    }

    if (CmdOrdersPanel)
    {
        CmdOrdersPanel->ShowOrdersDlg();
    }
}

void UCmdDlg::LoadMissionsInfo()
{
    USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
    SSWInstance->PlayAcceptSound(this);

    UE_LOG(LogTemp, Warning, TEXT("[CmdDlg] LoadMissionsInfo: BEGIN"));
    UE_LOG(LogTemp, Warning, TEXT("[CmdDlg] LoadMissionsInfo: CmdMissionsPanel=%s"),
        CmdMissionsPanel ? TEXT("VALID") : TEXT("NULL"));

    if (OperationalSwitcher)
    {
        OperationalSwitcher->SetActiveWidgetIndex(4);
    }

    if (OperationsModeText)
    {
        OperationsModeText->SetText(FText::FromString("MISSIONS"));
    }

    if (CmdMissionsPanel)
    {
        CmdMissionsPanel->ShowMissionsDlg();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[CmdDlg] LoadMissionsInfo: CmdMissionsPanel is NULL"));
    }

    UpdateMissionButton();
}

void UCmdDlg::LoadIntelInfo()
{
    if (ShouldDisableCommandPanels())
    {
        return;
    }

    if (GetIntelEntryCount() <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CmdDlg] LoadIntelInfo: no intel entries"));
        return;
    }

    USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
    SSWInstance->PlayAcceptSound(this);

    if (OperationalSwitcher)
    {
        OperationalSwitcher->SetActiveWidgetIndex(3);
    }

    if (OperationsModeText)
    {
        OperationsModeText->SetText(FText::FromString("INTEL"));
    }

    IntelRefreshCounterSeconds = 0;
    RefreshIntelPanel();
}

void UCmdDlg::LoadTheaterInfo()
{
    if (ShouldDisableCommandPanels())
    {
        return;
    }

    USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
    SSWInstance->PlayAcceptSound(this);

    if (OperationalSwitcher)
    {
        OperationalSwitcher->SetActiveWidgetIndex(1);
    }

    if (OperationsModeText)
    {
        OperationsModeText->SetText(FText::FromString("THEATER"));
    }

    if (CmdTheaterPanel)
    {
        CmdTheaterPanel->ShowTheaterDlg();
    }
}

void UCmdDlg::RefreshIntelPanel()
{
    if (ShouldDisableCommandPanels())
    {
        return;
    }

    if (!CmdIntelPanel)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CmdDlg] RefreshIntelPanel: CmdIntelPanel is NULL"));
        return;
    }

    CmdIntelPanel->ShowIntelDlg();
}
void UCmdDlg::HandleGameTimers()
{
    UGameInstance* GI = GetGameInstance();
    if (!GI)
    {
        UE_LOG(LogTemp, Error, TEXT("[CmdDlg] HandleGameTimers: GameInstance is NULL"));
        return;
    }

    UTimerSubsystem* Timer = GI->GetSubsystem<UTimerSubsystem>();
    USSWGameInstance* SSWInstance = Cast<USSWGameInstance>(GI);

    if (!SSWInstance) return;

    if (Timer)
    {
        Timer->OnUniverseSecond.AddUObject(this, &UCmdDlg::HandleUniverseSecondTick);
        Timer->OnUniverseMinute.AddUObject(this, &UCmdDlg::HandleUniverseMinuteTick);
        Timer->OnCampaignTPlusChanged.AddUObject(this, &UCmdDlg::HandleCampaignTPlusChanged);

        const uint64 Now = Timer->GetUniverseTimeSeconds();
        HandleUniverseSecondTick(Now);

        if (SSWInstance->CampaignSave)
        {
            HandleCampaignTPlusChanged(Now, SSWInstance->CampaignSave->GetTPlusSeconds(Now));
        }
    }
}

void UCmdDlg::HandleUniverseSecondTick(uint64 UniverseSecondsNow)
{
    USSWGameInstance* GI = Cast<USSWGameInstance>(GetGameInstance());
    if (!GI) return;

    UTimerSubsystem* Timer = GI->GetSubsystem<UTimerSubsystem>();
    if (!Timer) return;

    if (GameTimeText)
    {
        GameTimeText->SetText(
            FText::FromString(Timer->GetUniverseDateTimeString())
        );
    }

    // Keep intel data fresh every 10 seconds even when hidden.
    ++IntelRefreshCounterSeconds;

    if (IntelRefreshCounterSeconds >= 10)
    {
        IntelRefreshCounterSeconds = 0;
        RefreshIntelDataBackground();

        UE_LOG(LogTemp, Verbose,
            TEXT("[CmdDlg] Intel data refreshed in background"));
    }
}

void UCmdDlg::HandleUniverseMinuteTick(uint64 UniverseSecondsNow)
{
    USSWGameInstance* GI = Cast<USSWGameInstance>(GetGameInstance());
    if (!GI) return;
}

void UCmdDlg::HandleCampaignTPlusChanged(uint64 UniverseSecondsNow, uint64 TPlusSeconds)
{
    if (!CampaignTPlusText) return;
}

bool UCmdDlg::ShouldDisableCommandPanels() const
{
    if (!CampaignPtr)
    {
        return false;
    }

    return CampaignPtr->IsScripted();
}

void UCmdDlg::RefreshCommandButtons()
{
    const bool bDisablePanels = ShouldDisableCommandPanels();
    const bool bHasIntelEntries = GetIntelEntryCount() > 0;

    if (AllMenuButtons.IsValidIndex(1) && AllMenuButtons[1])
    {
        AllMenuButtons[1]->SetIsEnabled(!bDisablePanels);
    }

    if (AllMenuButtons.IsValidIndex(2) && AllMenuButtons[2])
    {
        AllMenuButtons[2]->SetIsEnabled(!bDisablePanels);
    }

    if (AllMenuButtons.IsValidIndex(3) && AllMenuButtons[3])
    {
        AllMenuButtons[3]->SetIsEnabled(!bDisablePanels && bHasIntelEntries);
    }

    bLastDisableState = bDisablePanels;
}

void UCmdDlg::UpdateMissionButton()
{
    if (!MissionButton)
    {
        return;
    }

    bool bEnable = false;

    if (CurrentScreen == ECmdScreen::Missions && CmdMissionsPanel)
    {
        bEnable = CmdMissionsPanel->CanAcceptSelectedMission();
    }

    MissionButton->SetIsEnabled(bEnable);
}

void UCmdDlg::ShowMissionsPanel()
{
    CurrentScreen = ECmdScreen::Missions;
    LoadMissionsInfo();
    UpdateMissionButton();
}

void UCmdDlg::RefreshIntelDataBackground()
{
    if (!CmdIntelPanel)
    {
        return;
    }

    CmdIntelPanel->RefreshIntelData();
}