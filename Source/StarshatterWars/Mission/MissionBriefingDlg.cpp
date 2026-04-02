/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    ORIGINAL SYSTEM
    ===============
    Starshatter 4.5 (Destroyer Studios)

    SUBSYSTEM:    Stars.exe
    FILE:         MissionBriefingDlg.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Mission Briefing Screen (Unreal Port)

    - Uses CmdDlg-style MenuButton system (SIT / PKG / NAV / WEP)
    - Drives UI state via WidgetSwitcher (MissionSwitcher)
    - Hosts child mission subpanels
    - Displays mission header (name, system, sector, time)
    - Handles Accept / Cancel flow
    - Calculates Time-On-Target using navigation instructions
*/

#include "MissionBriefingDlg.h"
#include "MissionPlanner.h"

#include "MissionObjectiveDlg.h"
#include "MissionPackageDlg.h"
#include "MissionNavDlg.h"
#include "MissionWeaponDlg.h"

// Legacy sim/campaign:
#include "Campaign.h"
#include "Mission.h"
#include "MissionInfo.h"
#include "StarSystem.h"
#include "FormatUtil.h"
#include "Instruction.h"
#include "MissionElement.h"
#include "CombatGroup.h"
#include "CampaignSituationReport.h"

// UI:
#include "MenuButton.h"
#include "SelectableButtonGroup.h"
#include "Components/SizeBox.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"
#include "Components/WidgetSwitcher.h"

// Input:
#include "InputCoreTypes.h"
#include "GameFramework/PlayerController.h"

// Optional sound:
#include "SSWGameInstance.h"

#include "StarshatterPlayerSubsystem.h"
#include "StarshatterGameDataSubsystem.h"
#include "StarshatterUIStyleSubsystem.h"
#include "StarshatterEnvironmentSubsystem.h"

UMissionBriefingDlg::UMissionBriefingDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UMissionBriefingDlg::SetMenuManager(UMenuScreen* InManager)
{
    manager = InManager;
}

void UMissionBriefingDlg::InitializeDlg(UMenuScreen* InManager)
{
    manager = InManager;
}

void UMissionBriefingDlg::NativePreConstruct()
{
    MenuButtonContainer->ClearChildren();

    if (MissionSituationPanel)
    {
        MissionSituationPanel->SetParentDlg(this);
    }

    if (MissionPackagePanel)
    {
        MissionPackagePanel->SetParentDlg(this);
    }

    if (MissionNavPanel)
    {
        MissionNavPanel->SetParentDlg(this);
    }

    if (MissionWepPanel)
    {
        MissionWepPanel->SetParentDlg(this);
    }
}

void UMissionBriefingDlg::NativeConstruct()
{
    Super::NativeConstruct();

    if (RootSizeBox)
    {
        RootSizeBox->SetWidthOverride(1920.f);
        RootSizeBox->SetHeightOverride(1080.f);
    }

    UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(RootSizeBox->Slot);
    if (Slot)
    {
        CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
        CanvasSlot->SetPosition(FVector2D(0.f, 0.f));
    }

    UGameInstance* GI = GetGameInstance();
    if (!GI)
    {
        UE_LOG(LogTemp, Error, TEXT("[MissionBriefingDlg] NativeConstruct: GameInstance is NULL"));
        return;
    }

    UStarshatterPlayerSubsystem* PlayerSS = GI->GetSubsystem<UStarshatterPlayerSubsystem>();
    if (!PlayerSS)
    {
        UE_LOG(LogTemp, Error, TEXT("[MissionBriefingDlg] NativeConstruct: PlayerSubsystem is NULL"));
        return;
    }

    UStarshatterGameDataSubsystem* DataSubsystem = GI->GetSubsystem<UStarshatterGameDataSubsystem>();
    if (!DataSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("[MissionBriefingDlg] NativeConstruct: GameDataSubsystem is NULL"));
        return;
    }

    if (!PlayerSS->HasLoaded())
    {
        PlayerSS->LoadPlayer();
    }

    const FS_PlayerGameInfo& PlayerInfo = PlayerSS->GetPlayerInfo();

    HandleGameTimers();

    const FS_Campaign* FoundCampaign = DataSubsystem->GetCampaignByIndex1Based(PlayerInfo.Campaign);
    if (FoundCampaign)
    {
        CurrentCampaignData = *FoundCampaign;
        bHasCurrentCampaign = true;

        UE_LOG(LogTemp, Log,
            TEXT("[MissionBriefingDlg] Loaded Campaign: %s (Index=%d)"),
            *CurrentCampaignData.Name,
            PlayerInfo.Campaign);
    }
    else
    {
        bHasCurrentCampaign = false;

        UE_LOG(LogTemp, Warning,
            TEXT("[MissionBriefingDlg] NativeConstruct: No campaign found for index %d"),
            PlayerInfo.Campaign);
    }

    if (TitleText)
    {
        TitleText->SetText(FText::FromString(TEXT("MISSION OPERATIONS")));
    }

    if (PlayerNameText)
    {
        PlayerNameText->SetText(FText::FromString(PlayerInfo.Name));

        UE_LOG(LogTemp, Log,
            TEXT("[MissionBriefingDlg] Player Name: %s"),
            *PlayerInfo.Name);
    }

    if (MissionSwitcher)
    {
        MissionSwitcher->SetActiveWidgetIndex(0);
    }

    if (bHasCurrentCampaign && CurrentLocationText)
    {
        CurrentLocationText->SetText(
            FText::FromString(CurrentCampaignData.System + TEXT(" System")).ToUpper());
    }

    if (MissionButton)
    {
        MissionButton->OnClicked.RemoveAll(this);
        MissionButton->OnClicked.AddDynamic(this, &UMissionBriefingDlg::HandleAcceptClicked);
    }

    if (ReturnButton)
    {
        ReturnButton->OnClicked.RemoveAll(this);
        ReturnButton->OnClicked.AddDynamic(this, &UMissionBriefingDlg::HandleCancelClicked);
    }

    BuildMenuButtons();
    InitializeSubPanels();

    UE_LOG(LogTemp, Warning, TEXT("[MissionBriefingDlg] NativeConstruct: UI initialized"));
}

void UMissionBriefingDlg::NativeDestruct()
{
    if (MissionButton)
    {
        MissionButton->OnClicked.RemoveAll(this);
    }

    if (ReturnButton)
    {
        ReturnButton->OnClicked.RemoveAll(this);
    }

    for (UMenuButton* Button : AllMenuButtons)
    {
        if (Button)
        {
            Button->OnSelected.RemoveAll(this);
            Button->OnHovered.RemoveAll(this);
        }
    }

    Super::NativeDestruct();
}

void UMissionBriefingDlg::ExecFrame()
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

    if (MissionTPlusText)
    {
        const double T = CampaignPtr->GetTime();

        char DayTime[32] = { 0 };
        FormatDayTime(DayTime, T);

        MissionTPlusText->SetText(FText::FromString(UTF8_TO_TCHAR(DayTime)));
    }
}
void UMissionBriefingDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (APlayerController* PC = GetOwningPlayer())
    {
        if (PC->WasInputKeyJustPressed(EKeys::Enter))
        {
            OnCommit();
        }
    }
    ExecFrame();
}

FText UMissionBriefingDlg::ToTextFromUtf8(const char* Utf8)
{
    if (!Utf8 || !Utf8[0])
    {
        return FText::GetEmpty();
    }

    return FText::FromString(FString(UTF8_TO_TCHAR(Utf8)));
}

void UMissionBriefingDlg::BuildMenuButtons()
{
    if (!MenuButtonClass || !MenuToggleGroup || !MenuButtonContainer)
    {
        return;
    }

    MenuButtonContainer->ClearChildren();
    AllMenuButtons.Empty();

    for (const FString& Item : MenuItems)
    {
        UMenuButton* NewButton = CreateWidget<UMenuButton>(this, MenuButtonClass);
        if (!NewButton)
        {
            continue;
        }

        // MAIN BRIEFING BUTTON SIZE
        NewButton->WidthOverride = 256.f;
        NewButton->HeightOverride = 42.f;
        NewButton->LabelFontSize = 16;

        if (UTextBlock* Label = Cast<UTextBlock>(NewButton->GetWidgetFromName(TEXT("Label"))))
        {
            Label->SetText(FText::FromString(Item));
        }

        NewButton->MenuOption = Item;

        MenuButtonContainer->AddChild(NewButton);
        MenuToggleGroup->RegisterButton(NewButton);

        NewButton->OnSelected.RemoveDynamic(this, &UMissionBriefingDlg::OnMenuToggleSelected);
        NewButton->OnSelected.AddDynamic(this, &UMissionBriefingDlg::OnMenuToggleSelected);

        NewButton->OnHovered.RemoveDynamic(this, &UMissionBriefingDlg::OnMenuToggleHovered);
        NewButton->OnHovered.AddDynamic(this, &UMissionBriefingDlg::OnMenuToggleHovered);

        AllMenuButtons.Add(NewButton);
    }
}

void UMissionBriefingDlg::InitializeSubPanels()
{
    if (MissionSituationPanel)
    {
        MissionSituationPanel->SetParentDlg(this);
		MissionSituationPanel->SetManager(MissionScreen);
    }

    if (MissionPackagePanel)
    {
        MissionPackagePanel->SetParentDlg(this);
        MissionPackagePanel->SetManager(MissionScreen);;
    }

    if (MissionNavPanel)
    {
        MissionNavPanel->SetParentDlg(this);
		MissionNavPanel->SetManager(MissionScreen);
    }

    if (MissionWepPanel)
    {
        MissionWepPanel->SetParentDlg(this);
		MissionWepPanel->SetManager(MissionScreen);
    }
}

void UMissionBriefingDlg::ShowMsnDlg()
{
    // Always reacquire the current campaign.
    // The dialog can be reused across campaign switches.
    CampaignPtr = Campaign::GetCampaign();
    MissionPtr = nullptr;
    PackageIndex = -1;

    int32 MissionId = -1;

    if (CampaignPtr)
    {
        MissionId = CampaignPtr->GetMissionId();

        UE_LOG(LogTemp, Warning,
            TEXT("[MissionBriefingDlg] ShowMsnDlg: Campaign=%p MissionId=%d"),
            CampaignPtr,
            MissionId);

        if (MissionId > 0)
        {
            MissionPtr = CampaignPtr->GetMission(MissionId);
        }
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[MissionBriefingDlg] ShowMsnDlg: CampaignPtr=%s MissionPtr=%s"),
        CampaignPtr ? TEXT("VALID") : TEXT("NULL"),
        MissionPtr ? TEXT("VALID") : TEXT("NULL"));

    if (MissionPtr)
    {
        const FString NameStr = ANSI_TO_TCHAR(MissionPtr->GetName());
        const FString DescStr = ANSI_TO_TCHAR(MissionPtr->GetDescription());
        const FString ObjStr = ANSI_TO_TCHAR(MissionPtr->GetObjective());
        const FString SitStr = ANSI_TO_TCHAR(MissionPtr->GetSituation());
        const FString SysStr = ANSI_TO_TCHAR(MissionPtr->GetSystem());
        const FString RegionStr = ANSI_TO_TCHAR(MissionPtr->GetRegion());
        const bool bIsScripted = MissionPtr->IsScripted();

        UE_LOG(LogTemp, Warning,
            TEXT("[MissionBriefingDlg] Mission Data: Name='%s' Desc='%s' Obj='%s' Sit='%s' Sys='%s' Region='%s' Scripted=%s Mission=%p Campaign=%p"),
            *NameStr,
            *DescStr,
            *ObjStr,
            *SitStr,
            *SysStr,
            *RegionStr,
            bIsScripted ? TEXT("true") : TEXT("false"),
            MissionPtr,
            CampaignPtr);

        if (!bIsScripted)
        {
            const bool bNeedsSitrep =
                SitStr.IsEmpty() ||
                SitStr.Equals(TEXT("Unknown"), ESearchCase::IgnoreCase) ||
                SitStr.Equals(TEXT("Mission Unknown"), ESearchCase::IgnoreCase) ||
                SitStr.Equals(TEXT("Unknown Mission"), ESearchCase::IgnoreCase) ||
                SitStr.Contains(TEXT("unknown"), ESearchCase::IgnoreCase);

            if (bNeedsSitrep)
            {
                if (CampaignPtr && MissionPtr->GetStarSystem())
                {
                    UE_LOG(LogTemp, Warning,
                        TEXT("[MissionBriefingDlg] Generating situation report for '%s'"),
                        *NameStr);

                    CampaignSituationReport Sitrep(CampaignPtr, MissionPtr);
                    Sitrep.GenerateSituationReport();

                    UE_LOG(LogTemp, Warning,
                        TEXT("[MissionBriefingDlg] Post-Sitrep: Situation='%s'"),
                        ANSI_TO_TCHAR(MissionPtr->GetSituation()));
                }
                else
                {
                    UE_LOG(LogTemp, Warning,
                        TEXT("[MissionBriefingDlg] Skipping sitrep for '%s': CampaignPtr=%s StarSystem=%s"),
                        *NameStr,
                        CampaignPtr ? TEXT("VALID") : TEXT("NULL"),
                        MissionPtr->GetStarSystem() ? TEXT("VALID") : TEXT("NULL"));
                }
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[MissionBriefingDlg] Scripted mission '%s': skipping CampaignSituationReport"),
                *NameStr);
        }
    }

    RefreshHeader();

    const bool bHasMission = (MissionPtr != nullptr);

    for (UMenuButton* Button : AllMenuButtons)
    {
        if (Button)
        {
            Button->SetIsEnabled(bHasMission);
        }
    }

    if (MissionButton)
    {
        MissionButton->SetIsEnabled(bHasMission);
    }

    if (ReturnButton)
    {
        ReturnButton->SetIsEnabled(true);
    }

    if (MissionButtonText)
    {
        MissionButtonText->SetText(FText::FromString(TEXT("ACCEPT")));
    }

    if (ReturnButtonText)
    {
        ReturnButtonText->SetText(FText::FromString(TEXT("CANCEL")));
    }

    SetMode(EMissionBriefingMode::SIT);

    if (MissionSituationPanel)
    {
        MissionSituationPanel->SetParentDlg(this);
        MissionSituationPanel->RefreshFromMission();
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[MissionBriefingDlg] MissionSituationPanel widget is null"));
    }
}

void UMissionBriefingDlg::RefreshHeader()
{
    if (MissionNameText)
    {
        MissionNameText->SetText(
            MissionPtr ? ToTextFromUtf8(MissionPtr->GetName())
            : FText::FromString(TEXT("NO MISSION")));
    }

    if (MissionSystemText)
    {
        MissionSystemText->SetText(FText::GetEmpty());

        if (MissionPtr)
        {
            const char* SystemName = MissionPtr->GetSystem();

            if (SystemName && SystemName[0])
            {
                MissionSystemText->SetText(ToTextFromUtf8(SystemName));
            }
            else if (StarSystem* Sys = MissionPtr->GetStarSystem())
            {
                MissionSystemText->SetText(ToTextFromUtf8(Sys->GetName()));
            }
        }
    }

    if (MissionSectorText)
    {
        MissionSectorText->SetText(
            MissionPtr ? ToTextFromUtf8(MissionPtr->GetRegion())
            : FText::GetEmpty());
    }

    if (MissionTimeStartText)
    {
        if (MissionPtr)
        {
            char Buf[32] = { 0 };
            FormatDayTime(Buf, MissionPtr->GetStart());
            MissionTimeStartText->SetText(ToTextFromUtf8(Buf));
        }
        else
        {
            MissionTimeStartText->SetText(FText::GetEmpty());
        }
    }

    if (MissionTimeTargetText && MissionTimeTargetLabelText)
    {
        if (bShowTimeOnTarget)
        {
            const int32 TimeOnTarget = CalcTimeOnTarget();
            if (TimeOnTarget > 0)
            {
                char Buf[32] = { 0 };
                FormatDayTime(Buf, TimeOnTarget);
                MissionTimeTargetText->SetText(ToTextFromUtf8(Buf));
                MissionTimeTargetLabelText->SetText(FText::FromString(TEXT("TARGET:")));
            }
            else
            {
                MissionTimeTargetText->SetText(FText::GetEmpty());
                MissionTimeTargetLabelText->SetText(FText::GetEmpty());
            }
        }
        else
        {
            MissionTimeTargetText->SetText(FText::GetEmpty());
            MissionTimeTargetLabelText->SetText(FText::GetEmpty());
        }
    }
}

void UMissionBriefingDlg::SetMode(EMissionBriefingMode NewMode)
{
    CurrentMode = NewMode;

    if (MissionSwitcher)
    {
        MissionSwitcher->SetActiveWidgetIndex(static_cast<int32>(CurrentMode));
    }

    RefreshMenuSelection();

    switch (CurrentMode)
    {
    case EMissionBriefingMode::SIT:
        if (MissionSituationPanel)
        {
            MissionSituationPanel->RefreshFromMission();
        }
        break;

    case EMissionBriefingMode::PKG:
        if (MissionPackagePanel)
        {
            MissionPackagePanel->RefreshFromMission();
        }
        break;

    case EMissionBriefingMode::NAV:
        if (MissionNavPanel)
        {
            MissionNavPanel->RefreshFromMission();
        }
        break;

    case EMissionBriefingMode::WEP:
        UE_LOG(LogTemp, Warning,
            TEXT("[MissionBriefingDlg] Enter WEP: this=%p MissionPtr=%p MissionWepPanel=%p"),
            this,
            GetMissionPtr(),
            MissionWepPanel);
        
        if (MissionWepPanel)
        {
            MissionWepPanel->RefreshFromMission();
        }
        break;

    default:
        break;
    }
}

void UMissionBriefingDlg::RefreshMenuSelection()
{
    const int32 ModeIndex = static_cast<int32>(CurrentMode);
    const FString ActiveLabel = MenuItems.IsValidIndex(ModeIndex)
        ? MenuItems[ModeIndex]
        : FString();

    for (UMenuButton* Button : AllMenuButtons)
    {
        if (Button)
        {
            Button->SetSelected(Button->MenuOption == ActiveLabel);
        }
    }
}

void UMissionBriefingDlg::OnMenuToggleSelected(UMenuButton* SelectedButton)
{
    if (!SelectedButton)
    {
        return;
    }

    if (USSWGameInstance* GI = Cast<USSWGameInstance>(GetGameInstance()))
    {
        GI->PlayAcceptSound(this);
    }

    const FString& Option = SelectedButton->MenuOption;

    if (Option == TEXT("SIT"))
    {
        SetMode(EMissionBriefingMode::SIT);
    }
    else if (Option == TEXT("PKG"))
    {
        SetMode(EMissionBriefingMode::PKG);
    }
    else if (Option == TEXT("NAV"))
    {
        SetMode(EMissionBriefingMode::NAV);
    }
    else if (Option == TEXT("WEP"))
    {
        SetMode(EMissionBriefingMode::WEP);
    }
}

void UMissionBriefingDlg::OnMenuToggleHovered(UMenuButton* HoveredButton)
{
    if (!HoveredButton)
    {
        return;
    }

    if (USSWGameInstance* GI = Cast<USSWGameInstance>(GetGameInstance()))
    {
        GI->PlayHoverSound(this);
    }
}

int32 UMissionBriefingDlg::CalcTimeOnTarget() const
{
    if (!MissionPtr)
    {
        return 0;
    }

    const auto& Elements = MissionPtr->GetElements();
    if (Elements.size() == 0)
    {
        return 0;
    }

    MissionElement* Element = Elements[0];
    if (!Element)
    {
        return 0;
    }

    FVector Loc(
        Element->GetLocation().X,
        Element->GetLocation().Y,
        Element->GetLocation().Z
    );

    Swap(Loc.Y, Loc.Z);

    int32 MissionTime = MissionPtr->GetStart();

    ListIter<Instruction> NavPt = Element->NavList();
    while (++NavPt)
    {
        const int32 Action = static_cast<int32>(NavPt->GetAction());

        FVector NavLoc(
            NavPt->Location().X,
            NavPt->Location().Y,
            NavPt->Location().Z
        );

        const double Dist = FVector::Dist(Loc, NavLoc);

        const double Speed = NavPt->Speed();
        const int32 ETR = (Speed > 0.0)
            ? static_cast<int32>(Dist / Speed)
            : static_cast<int32>(Dist / 500.0);

        MissionTime += ETR;
        Loc = NavLoc;

        if (Action >= static_cast<int32>(INSTRUCTION_ACTION::ESCORT))
        {
            return MissionTime;
        }
    }

    return 0;
}

void UMissionBriefingDlg::OnCommit()
{
    if (Manager)
    {
        Manager->Hide();
    }
}

void UMissionBriefingDlg::OnCancel()
{
    if (manager) {
        manager->ShowOperationsDlg();
    }
    else {
        Hide();
    }
}

void UMissionBriefingDlg::HandleAcceptClicked()
{
    OnCommit();
}

void UMissionBriefingDlg::HandleCancelClicked()
{
    OnCancel();
}

void UMissionBriefingDlg::HandleGameTimers()
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
        Timer->OnUniverseSecond.AddUObject(this, &UMissionBriefingDlg::HandleUniverseSecondTick);
        Timer->OnUniverseMinute.AddUObject(this, &UMissionBriefingDlg::HandleUniverseMinuteTick);
        Timer->OnCampaignTPlusChanged.AddUObject(this, &UMissionBriefingDlg::HandleCampaignTPlusChanged);

        const uint64 Now = Timer->GetUniverseTimeSeconds();
        HandleUniverseSecondTick(Now);

        if (SSWInstance->CampaignSave)
        {
            HandleCampaignTPlusChanged(Now, SSWInstance->CampaignSave->GetTPlusSeconds(Now));
        }
    }
}

void UMissionBriefingDlg::HandleUniverseSecondTick(uint64 UniverseSecondsNow)
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

void UMissionBriefingDlg::HandleUniverseMinuteTick(uint64 UniverseSecondsNow)
{
    USSWGameInstance* GI = Cast<USSWGameInstance>(GetGameInstance());
    if (!GI) return;
}

void UMissionBriefingDlg::HandleCampaignTPlusChanged(uint64 UniverseSecondsNow, uint64 TPlusSeconds)
{
    if (!MissionTPlusText) return;
}