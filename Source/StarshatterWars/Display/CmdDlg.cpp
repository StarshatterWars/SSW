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
#include "FormatUtil.h"      // FormatDayTime(...)
#include "Mouse.h"           // Mouse::Show(...)

// Your campaign screen widget (port of CmpnScreen):
#include "CmpnScreen.h"
// Your campaign file dialog widget (port of CmpFileDlg):
#include "CmpFileDlg.h"
#include "MenuButton.h"
#include "GameStructs.h"

#include "SelectableButtonGroup.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"

#include "StarshatterPlayerSubsystem.h"
#include "StarshatterGameDataSubsystem.h"
#include "StarshatterUIStyleSubsystem.h"

#include "CmdOrdersDlg.h"
#include "CmdMissionsDlg.h"
#include "CmdForceDlg.h"
#include "CmdIntelDlg.h"
#include "CmdTheaterDlg.h"

#include "TimerSubsystem.h"
#include "FormattingUtils.h"
#include "SSWGameInstance.h"

/*template<typename TEnum>
bool FStringToEnum(const FString& InString, TEnum& OutEnum, bool bCaseSensitive = true)
{
    UEnum* Enum = StaticEnum<TEnum>();
    if (!Enum) return false;

    for (int32 i = 0; i < Enum->NumEnums(); ++i)
    {
        FString Name = Enum->GetNameStringByIndex(i);
        if ((bCaseSensitive && Name == InString) ||
            (!bCaseSensitive && Name.Equals(InString, ESearchCase::IgnoreCase)))
        {
            OutEnum = static_cast<TEnum>(Enum->GetValueByIndex(i));
            return true;
        }
    }
    return true;
}*/

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

    // Ensure player is loaded
    if (!PlayerSS->HasLoaded())
    {
        PlayerSS->LoadPlayer();
    }

    const FS_PlayerGameInfo& PlayerInfo = PlayerSS->GetPlayerInfo();

    HandleGameTimers();

    // =========================================================
    // RESOLVE & CACHE CURRENT CAMPAIGN (COPY SAFE VERSION)
    // =========================================================
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

    // =========================================================
    // UI SETUP
    // =========================================================
    if (TitleText)
    {
        TitleText->SetText(FText::FromString(TEXT("CAMPAIGN OPERATIONS")));
    }

    if (!MenuButtonClass || !MenuToggleGroup || !MenuButtonContainer)
        return;

    MenuButtonContainer->ClearChildren();
    AllMenuButtons.Empty();

    for (int32 i = 0; i < 5; ++i)
    {
        UMenuButton* NewButton = CreateWidget<UMenuButton>(this, MenuButtonClass);
        if (!NewButton)
            continue;

        if (UTextBlock* Label = Cast<UTextBlock>(NewButton->GetWidgetFromName(TEXT("Label"))))
        {
            Label->SetText(FText::FromString(MenuItems[i]).ToUpper());
        }

        NewButton->MenuOption = MenuItems[i];

        MenuButtonContainer->AddChild(NewButton);
        MenuToggleGroup->RegisterButton(NewButton);

        NewButton->OnSelected.RemoveDynamic(this, &UCmdDlg::OnMenuToggleSelected);
        NewButton->OnSelected.AddDynamic(this, &UCmdDlg::OnMenuToggleSelected);
        NewButton->OnHovered.AddDynamic(this, &UCmdDlg::OnMenuToggleHovered);

        AllMenuButtons.Add(NewButton);
    }
    
    
    // =========================================================
    // BUTTON SETUP
    // =========================================================
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
    // =========================================================
    // PLAYER INFO
    // =========================================================
    if (PlayerNameText)
    {
        PlayerNameText->SetText(FText::FromString(PlayerInfo.Name));
        UE_LOG(LogTemp, Log, TEXT("[CmdDlg] Player Name: %s"), *PlayerInfo.Name);
    }

    if (OperationalSwitcher)
    {
        OperationalSwitcher->SetActiveWidgetIndex(0);
    }

    // =========================================================
    // CAMPAIGN UI (USES CACHED COPY)
    // =========================================================
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

    // =========================================================
    // BUTTON BINDS
    // =========================================================
    if (btn_save)     btn_save->OnClicked.AddDynamic(this, &UCmdDlg::OnSaveClicked);
    if (btn_exit)     btn_exit->OnClicked.AddDynamic(this, &UCmdDlg::OnExitClicked);

    // =========================================================
    // LEGACY SYSTEM HOOKS
    // =========================================================
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
    MenuButtonContainer->ClearChildren();
    if (CmdOrdersPanel)
    {
        CmdOrdersPanel->SetParentCmdDlg(this);
        CmdOrdersPanel->ShowOrdersDlg(); // or Refresh
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

    ExecFrame();
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

    // Example action when one of the buttons is selected
    UE_LOG(LogTemp, Log, TEXT("Selected button: %s"), *GetNameSafe(SelectedButton));

    // Example: compare button text, tags, or use a mapping
    // Then trigger OOB filtering or changes in UI accordingly
    const FString& MenuOption = SelectedButton->MenuOption;

    if (MenuOption == MenuItems[0])
    {
        LoadOrdersInfo();
    }
    else if (MenuOption == MenuItems[1])
    {
        LoadTheaterInfo();
    }
    else if (MenuOption == MenuItems[2])
    {
        LoadForcesInfo();
    }
    else if (MenuOption == MenuItems[3])
    {
        LoadIntelInfo();
    }
    else if (MenuOption == MenuItems[4])
    {
        LoadMissionsInfo();
    }
}

void UCmdDlg::OnMenuToggleHovered(UMenuButton* HoveredButton)
{
    if (!HoveredButton) return;

    UE_LOG(LogTemp, Log, TEXT("Hovered over: %s"), *HoveredButton->MenuOption);
    USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
    SSWInstance->PlayHoverSound(this);
}

// --------------------------------------------------------------------
// UBaseScreen overrides
// --------------------------------------------------------------------

void UCmdDlg::BindFormWidgets()
{

    BindButton(1, btn_save);
    BindButton(2, btn_exit);
}

// --------------------------------------------------------------------
// Public API
// --------------------------------------------------------------------

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
            txt_name->SetText(FText::FromString(CampaignPtr->Name()));
        else
            txt_name->SetText(FText::FromString(TEXT("No Campaign Selected")));
    }

    if (CampaignPtr)
    {
        const bool bTraining = CampaignPtr->IsTraining();

        if (btn_save)     btn_save->SetIsEnabled(!bTraining);
    }

    SetVisibility(ESlateVisibility::Visible);
}

void UCmdDlg::ExecFrame()
{
    if (!CampaignPtr)
        CampaignPtr = Campaign::GetCampaign();

    if (!CampaignPtr)
        return;

    // Player group:
    if (CurrentUnitText)
    {
        CombatGroup* G = CampaignPtr->GetPlayerGroup();
        if (G)
            CurrentUnitText->SetText(FText::FromString(G->GetDescription()));
    }

    // Score:
    if (PlayerScoreText)
    {
        const int32 TeamScore = CampaignPtr->GetPlayerTeamScore();
        const FString ScoreStr = FString::Printf(TEXT("Team Score: %d"), TeamScore);
        PlayerScoreText->SetText(FText::FromString(ScoreStr));
        PlayerScoreText->SetJustification(ETextJustify::Right);
    }

    // Time:
    if (CampaignTPlusText)
    {
        const double T = CampaignPtr->GetTime();

        char DayTime[32] = { 0 };
        FormatDayTime(DayTime, T);

        CampaignTPlusText->SetText(FText::FromString(UTF8_TO_TCHAR(DayTime)));
    }

    // Intel unread count -> change button label:
    const int32 Unread = CampaignPtr->CountNewEvents();

    RefreshCommandButtons();

    if (CmdOrdersPanel)
    {
        CmdOrdersPanel->ShowOrdersDlg();
    }

    if (AllMenuButtons[3])
    {
        if (Unread > 0) {
            if(UTextBlock * Label = Cast<UTextBlock>(AllMenuButtons[3]->GetWidgetFromName(TEXT("Label"))))
            {
                Label->SetText(FText::FromString(FString::Printf(TEXT("INTEL (%d)"), Unread)));
            }
        }
        else {
            if (UTextBlock* Label = Cast<UTextBlock>(AllMenuButtons[3]->GetWidgetFromName(TEXT("Label"))))
            {
                Label->SetText(FText::FromString(FString::Printf(TEXT("INTEL"))));
            }
        }
    }
}

// --------------------------------------------------------------------
// Button handlers
// --------------------------------------------------------------------

void UCmdDlg::OnSaveClicked()
{
    if (!CampaignPtr)
        CampaignPtr = Campaign::GetCampaign();

    if (CampaignPtr && CmpnScreen)
    {
        // Classic:
        // CmpFileDlg* fdlg = cmpn_screen->GetCmpFileDlg();
        // cmpn_screen->ShowCmpFileDlg();
        //
        // Unreal port depends on your UCmpnScreen API:
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
    if (Stars)
    {
        Mouse::Show(false);
        Stars->SetGameMode(EGameMode::MENU);
    }

    if (manager)
        manager->ShowMenuDlg();
    else
        HideDlg();
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

void UCmdDlg::LoadForcesInfo()
{
    if (ShouldDisableCommandPanels())
    {
        return;
    }

    USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
    SSWInstance->PlayAcceptSound(this);

    if (OperationalSwitcher) {
        OperationalSwitcher->SetActiveWidgetIndex(2);
    }
    if (OperationsModeText) {
        OperationsModeText->SetText(FText::FromString("FORCES"));
    }

    if (CmdForcesPanel)
    {
        CmdForcesPanel->ShowForceDlg(); // or Refresh
    }
}

void UCmdDlg::LoadOrdersInfo()
{
    USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
    SSWInstance->PlayAcceptSound(this);

    if (OperationalSwitcher) {
        OperationalSwitcher->SetActiveWidgetIndex(0);
    }
    if (OperationsModeText) {
        OperationsModeText->SetText(FText::FromString("ORDERS"));
    }
    
    if (CmdOrdersPanel)
    {
        CmdOrdersPanel->ShowOrdersDlg(); // or Refresh
    }
}

void UCmdDlg::LoadMissionsInfo()
{
    USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
    SSWInstance->PlayAcceptSound(this);

    UE_LOG(LogTemp, Warning, TEXT("[CmdDlg] LoadMissionsInfo: BEGIN"));
    UE_LOG(LogTemp, Warning, TEXT("[CmdDlg] LoadMissionsInfo: CmdMissionsPanel=%s"),
        CmdMissionsPanel ? TEXT("VALID") : TEXT("NULL"));

    if (OperationalSwitcher) {
        OperationalSwitcher->SetActiveWidgetIndex(4);
    }

    if (OperationsModeText) {
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
}
void UCmdDlg::LoadIntelInfo()
{
    if (ShouldDisableCommandPanels())
    {
        return;
    }

    USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
    SSWInstance->PlayAcceptSound(this);

    if (OperationalSwitcher) {
        OperationalSwitcher->SetActiveWidgetIndex(3);
    }

    if (OperationsModeText) {
        OperationsModeText->SetText(FText::FromString("INTEL"));
    }
    
    if (CmdIntelPanel)
    {
        CmdIntelPanel->ShowIntelDlg(); // or Refresh
    }
}

void UCmdDlg::LoadTheaterInfo()
{
    
    if (ShouldDisableCommandPanels())
    {
        return;
    }
    
    USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
    SSWInstance->PlayAcceptSound(this);

    if (OperationalSwitcher) {
        OperationalSwitcher->SetActiveWidgetIndex(1);
    }

    if (OperationsModeText) {
        OperationsModeText->SetText(FText::FromString("THEATER"));
    }

    if (CmdTheaterPanel)
    {
        CmdTheaterPanel->ShowTheaterDlg(); // or Refresh
    }
}

void UCmdDlg::HandleGameTimers()
{
    // -------------------------------
    // PUB/SUB SUBSCRIBE  
    // -------------------------------
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

        // Initial push:
        const uint64 Now = Timer->GetUniverseTimeSeconds();
        HandleUniverseSecondTick(Now);

        // If campaign T+ depends on universe seconds, keep this:
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
}

void UCmdDlg::HandleUniverseMinuteTick(uint64 UniverseSecondsNow)
{
    USSWGameInstance* GI = Cast<USSWGameInstance>(GetGameInstance());
    if (!GI) return;
}

void UCmdDlg::HandleCampaignTPlusChanged(uint64 UniverseSecondsNow, uint64 TPlusSeconds)
{
    if (!CampaignTPlusText) return;

    //CampaignTPlusText->SetText(FText::FromString(UFormattingUtils::FormatTPlus(TPlusSeconds)));
}

bool UCmdDlg::ShouldDisableCommandPanels() const
{
    if (!CampaignPtr)
    {
        return false;
    }

    // Replace with your actual logic
    return CampaignPtr->IsScripted();
}

void UCmdDlg::RefreshCommandButtons()
{
    const bool bDisable = ShouldDisableCommandPanels();

    if (bDisable != bLastDisableState)
    {
        bLastDisableState = bDisable;

        if (AllMenuButtons[1])
        {
            AllMenuButtons[1]->SetIsEnabled(!bDisable);
        }

        if (AllMenuButtons[2])
        {
            AllMenuButtons[2]->SetIsEnabled(!bDisable);
        }

        if (AllMenuButtons[3])
        {
            AllMenuButtons[3]->SetIsEnabled(!bDisable);
        }
    }
}
