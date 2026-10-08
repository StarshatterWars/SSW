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
#include "MissionDebriefDlg.h"
#include "GameScreen.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "QuitView.h"
#include "QuitMissionMenu.h"
#include "UObject/ConstructorHelpers.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Engine/LocalPlayer.h"
#include "SSWRuntimeSubsystem.h"
#include "Sim.h"
#include "TimerSubsystem.h"
#include "SimEvent.h"
#include "OptionsScreen.h"
#include "Kismet/GameplayStatics.h"

#include "MenuScreen.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Camera/CameraActor.h"
#include "TimerManager.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/PackageName.h"
#include "EngineUtils.h"
#include "PlanetActor.h"
#include "SystemSceneBuilder.h"
#include "GasGiantActor.h"
#include "CentralSun.h"
#include "ShipActor.h"
#include "Ship.h"
#include "LandingGear.h"
#include "SimRegion.h"
#include "MissionTargetOverlay.h"
#include "FighterHUDPanels.h"
#include "FighterHUDDetails.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

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
#include "CombatUnit.h"
#include "FormattingUtils.h"
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
#include "StarshatterAudioSubsystem.h"

UMissionBriefingDlg::UMissionBriefingDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    static ConstructorHelpers::FClassFinder<UQuitMissionMenu> MenuBlueprint(
        TEXT("/Game/Screens/Mission/WBP_QuitMissionMenu"));
    if (MenuBlueprint.Succeeded()) QuitMissionMenuClass = MenuBlueprint.Class;
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
    CloseEmptySectorPreview();
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

    UVerticalBox* MenuVBox = Cast<UVerticalBox>(MenuButtonContainer);
    if (!MenuVBox)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MissionBriefingDlg] MenuButtonContainer is not a VerticalBox"));
        return;
    }

    MenuVBox->ClearChildren();
    AllMenuButtons.Empty();

    for (const FString& Item : MenuItems)
    {
        UMenuButton* NewButton = CreateWidget<UMenuButton>(this, MenuButtonClass);
        if (!NewButton)
        {
            continue;
        }

        NewButton->SetLayoutMode(ELayoutMode::FillWidth);
        NewButton->SetButtonSize(0.f, 42.f);
        NewButton->SetLabelFontSizeValue(16);
        NewButton->SetMenuOption(Item);
        NewButton->SetButtonText(FText::FromString(Item).ToUpper());

        if (UVerticalBoxSlot* VBoxSlot = MenuVBox->AddChildToVerticalBox(NewButton))
        {
            VBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 0.f));
            VBoxSlot->SetHorizontalAlignment(HAlign_Fill);
            VBoxSlot->SetVerticalAlignment(VAlign_Center);
        }

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
    CloseEmptySectorPreview();
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

    if (UStarshatterAudioSubsystem* AudioSS =
        GetGameInstance()->GetSubsystem<UStarshatterAudioSubsystem>())
    {
        AudioSS->PlayAcceptSound(this);
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

    if (UStarshatterAudioSubsystem* AudioSS =
        GetGameInstance()->GetSubsystem<UStarshatterAudioSubsystem>())
    {
        AudioSS->PlayHoverSound(this);
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
            NavPt->GetLocation().X,
            NavPt->GetLocation().Y,
            NavPt->GetLocation().Z
        );

        const double Dist = FVector::Dist(Loc, NavLoc);

        const double Speed = NavPt->GetSpeed();
        const int32 ETR = (Speed > 0.0)
            ? static_cast<int32>(Dist / Speed)
            : static_cast<int32>(Dist / 500.0);

        MissionTime += ETR;
        Loc = NavLoc;

        if (Action >= static_cast<int32>(EInstruction::Escort))
        {
            return MissionTime;
        }
    }

    return 0;
}

void UMissionBriefingDlg::CloseEmptySectorPreview(bool bPreserveSimulation)
{
    if (bLiveMissionStarted)
    {
        if (auto* Runtime = GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>())
        {
            Runtime->SetPaused(true);
            Runtime->EndMissionPresentation();
        }
        if (!bPreserveSimulation)
            if (Sim* Simulation = Sim::GetSim())
                if (Simulation->GetMission() == MissionPtr) Simulation->UnloadMission();
        if (auto* Timer = GetGameInstance()->GetSubsystem<UTimerSubsystem>()) Timer->StopMissionRun();
        bLiveMissionStarted = false;
        if (!bPreserveSimulation)
            if (auto* Runtime = GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>())
                Runtime->SetGameMode(EGameMode::CMPN);
    }
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(MissionTargetCameraTimer);
        if (MissionTargetOverlay.IsValid() && World->GetGameViewport())
            World->GetGameViewport()->RemoveViewportWidgetContent(MissionTargetOverlay.ToSharedRef());
    }
    MissionTargetOverlay.Reset();
    SelectedMissionTarget.Reset();
    SelectedMissionTargetName = FText::GetEmpty();
    SelectedMissionTargetLocalBounds = FBox(ForceInit);
    DisableMissionMenuInput();
    MissionTitleRevealTime = -1.0;
    if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(MissionSunRevealTimer);
    for (const TWeakObjectPtr<AActor>& Actor : MissionSunHiddenActors)
        if (Actor.IsValid()) Actor->SetActorHiddenInGame(false);
    MissionSunHiddenActors.Empty();
    MissionCameraReadyChecks = 0;
    MissionCameraStableSince = -1.0;
    if (IsValid(MissionPreviewCamera))
    {
        if (APlayerController* PC = GetOwningPlayer())
        {
            if (PC->GetViewTarget() == MissionPreviewCamera.Get())
            {
                AActor* RestoreTarget = PreviousMissionViewTarget.Get();
                if (!RestoreTarget) RestoreTarget = PC;
                PC->SetViewTarget(RestoreTarget);
            }
        }
        MissionPreviewCamera->Destroy();
        MissionPreviewCamera = nullptr;
    }
    PreviousMissionViewTarget.Reset();
    if (MissionSystemLevel)
    {
        MissionSystemLevel->OnLevelShown.RemoveDynamic(
            this, &UMissionBriefingDlg::HandleMissionSystemLevelShown);
        MissionSystemLevel->SetShouldBeVisible(false);
        MissionSystemLevel->SetShouldBeLoaded(false);
        MissionSystemLevel->SetIsRequestingUnloadAndRemoval(true);
        MissionSystemLevel = nullptr;
    }
    MissionSystemPackage.Empty();
    if (MissionSceneCover.IsValid())
    {
        if (UWorld* World = GetWorld())
            if (UGameViewportClient* Viewport = World->GetGameViewport())
                Viewport->RemoveViewportWidgetContent(MissionSceneCover.ToSharedRef());
        MissionSceneCover.Reset();
    }
    if (EmptySectorOverlay.IsValid())
    {
        if (UWorld* World = GetWorld())
        {
            if (UGameViewportClient* Viewport = World->GetGameViewport())
                Viewport->RemoveViewportWidgetContent(EmptySectorOverlay.ToSharedRef());
        }
        EmptySectorOverlay.Reset();
    }
    for (const TWeakObjectPtr<AActor>& Actor : PreviewHiddenActors)
    {
        if (Actor.IsValid()) Actor->SetActorHiddenInGame(false);
    }
    PreviewHiddenActors.Empty();
}

void UMissionBriefingDlg::HandleMissionSystemLevelShown()
{
    UWorld* World = GetWorld();
    APlayerController* PC = GetOwningPlayer();
    if (!World || !PC || !MissionSystemLevel) return;

    // Builders can spawn into the persistent level: also check the owner chain.
    UStaticMeshComponent* StarMesh = nullptr;
    for (TActorIterator<AActor> It(World); It && !StarMesh; ++It)
    {
        bool bBelongsToMission = false;
        for (AActor* Owner = *It; Owner; Owner = Owner->GetOwner())
        {
            if (Owner->GetLevel() == MissionSystemLevel->GetLoadedLevel())
            {
                bBelongsToMission = true;
                break;
            }
        }
        if (!bBelongsToMission) continue;
        TArray<UStaticMeshComponent*> Meshes;
        It->GetComponents<UStaticMeshComponent>(Meshes, true);
        for (UStaticMeshComponent* Mesh : Meshes)
        {
            if (IsValid(Mesh) && Mesh->GetFName() == FName(TEXT("SM_Star")) &&
                Mesh->GetStaticMesh())
            {
                StarMesh = Mesh;
                break;
            }
        }
    }
    if (!StarMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MissionBriefing] No mission SM_Star mesh to frame"));
        return;
    }

    // OnLevelShown runs before this level's first rendered frame. Hide the
    // complete star actor tree, including its corona and particle effects.
    AActor* SunRoot = StarMesh->GetOwner();
    while (SunRoot && SunRoot->GetParentActor()) SunRoot = SunRoot->GetParentActor();
    TArray<AActor*> SunActors;
    if (SunRoot)
    {
        SunActors.Add(SunRoot);
        SunRoot->GetAllChildActors(SunActors, true);
        SunActors.AddUnique(SunRoot);
    }
    for (AActor* Actor : SunActors)
    {
        if (IsValid(Actor) && !Actor->IsHidden())
        {
            MissionSunHiddenActors.AddUnique(Actor);
            Actor->SetActorHiddenInGame(true);
        }
    }

    FBoxSphereBounds Bounds = StarMesh->CalcBounds(StarMesh->GetComponentTransform());
    FString FocusName = TEXT("primary star");
    bool bFocusPlanet = false;
    const FString RegionName = MissionPtr
        ? ToTextFromUtf8(MissionPtr->GetRegion()).ToString().TrimStartAndEnd() : FString();
    for (TActorIterator<ASystemSceneBuilder> It(World); It; ++It)
    {
        if (It->GetLevel() != MissionSystemLevel->GetLoadedLevel()) continue;
        FSpawnedSystemRegion Region;
        if (!It->GetRegionByName(RegionName, Region) &&
            !It->GetRegionByName(RegionName + TEXT("_REGION"), Region)) continue;
        for (const FSpawnedSystemBody& Body : It->GetSpawnedBodies())
        {
            if (Body.bIsOrbit || !IsValid(Body.Actor)) continue;
            if (!Body.BodyName.Equals(Region.AnchorBodyName, ESearchCase::IgnoreCase) &&
                Body.Actor != Region.ParentActor) continue;
            FocusName = Body.BodyName;
            bFocusPlanet = Body.ParentActor != nullptr || Body.bIsMoon;
            if (bFocusPlanet)
            {
                // Use the physical surface, excluding orbit rings and effects.
                UStaticMeshComponent* Surface = nullptr;
                if (APlanetActor* Planet = Cast<APlanetActor>(Body.Actor.Get()))
                    Surface = Planet->GetPlanetMeshComponent();
                if (Surface && Surface->GetStaticMesh())
                    Bounds = Surface->CalcBounds(Surface->GetComponentTransform());
                else
                {
                    const double R = FMath::Max(1.0f, Body.VisualRadiusUnits);
                    Bounds = FBoxSphereBounds(Body.Actor->GetActorLocation(), FVector(R), R);
                }
                TArray<AActor*> FocusActors;
                Body.Actor->GetAllChildActors(FocusActors, true);
                FocusActors.AddUnique(Body.Actor.Get());
                for (AActor* Actor : FocusActors)
                {
                    if (IsValid(Actor) && !Actor->IsHidden())
                    {
                        MissionSunHiddenActors.AddUnique(Actor);
                        Actor->SetActorHiddenInGame(true);
                    }
                }
            }
            break;
        }
        break;
    }
    if (!StartLiveMission())
    {
        UE_LOG(LogTemp, Error, TEXT("[MissionBriefing] Live mission startup failed; returning to briefing"));
        CloseEmptySectorPreview();
        if (manager) manager->ShowMissionDlg();
        return;
    }
    const double Radius = Bounds.BoxExtent.GetMax();
    int32 Width = 0, Height = 0;
    PC->GetViewportSize(Width, Height);
    if (Radius <= KINDA_SMALL_NUMBER || Width <= 0 || Height <= 0) return;

    if (!IsValid(MissionPreviewCamera))
    {
        MissionPreviewCamera = World->SpawnActor<ACameraActor>();
        if (!MissionPreviewCamera) return;
        PreviousMissionViewTarget = PC->GetViewTarget();
    }
    UCameraComponent* Camera = MissionPreviewCamera->GetCameraComponent();
    Camera->SetFieldOfView(60.0f);
    Camera->bConstrainAspectRatio = false;
    Camera->bOverrideAspectRatioAxisConstraint = true;
    Camera->AspectRatioAxisConstraint = AspectRatio_MaintainXFOV;
    const double FocalPixels = Width / (2.0 * FMath::Tan(FMath::DegreesToRadians(30.0)));
    const double DiameterPixels = bFocusPlanet ? 128.0 : 64.0;
    const double Ratio = FocalPixels / (DiameterPixels * 0.5);
    const double Distance = Radius * FMath::Sqrt(1.0 + Ratio * Ratio);
    const FRotator Rotation = PC->PlayerCameraManager
        ? PC->PlayerCameraManager->GetCameraRotation() : FRotator::ZeroRotator;
    MissionPreviewCamera->SetActorLocationAndRotation(
        Bounds.Origin - Rotation.Vector() * Distance, Rotation);
    PC->SetViewTarget(MissionPreviewCamera.Get());
    // Warm up all visuals behind the cover, including the star's effects.
    for (const TWeakObjectPtr<AActor>& Actor : MissionSunHiddenActors)
        if (Actor.IsValid()) Actor->SetActorHiddenInGame(false);
    MissionSunHiddenActors.Empty();
    MissionCameraReadyChecks = 0;
    MissionCameraStableSince = -1.0;
    World->GetTimerManager().SetTimer(MissionSunRevealTimer, this,
        &UMissionBriefingDlg::RevealMissionSunWhenCameraReady, 0.05f, true);
    if (PC->PlayerCameraManager) PC->PlayerCameraManager->SetGameCameraCutThisFrame();
    UE_LOG(LogTemp, Log, TEXT("[MissionBriefing] Framed region object %s at distance %.2f"),
        *FocusName, Distance);
}

void UMissionBriefingDlg::RevealMissionSunWhenCameraReady()
{
    UWorld* World = GetWorld();
    APlayerController* PC = GetOwningPlayer();
    if (!World) return;
    if (!MissionSystemLevel || !IsValid(MissionPreviewCamera))
    {
        World->GetTimerManager().ClearTimer(MissionSunRevealTimer);
        return;
    }
    const bool bReady = PC && PC->PlayerCameraManager &&
        PC->GetViewTarget() == MissionPreviewCamera.Get() &&
        PC->PlayerCameraManager->GetCameraLocation().Equals(
            MissionPreviewCamera->GetActorLocation(), 1.0f) &&
        PC->PlayerCameraManager->GetCameraRotation().Equals(
            MissionPreviewCamera->GetActorRotation(), 0.1f) &&
        FMath::IsNearlyEqual(PC->PlayerCameraManager->GetFOVAngle(),
            MissionPreviewCamera->GetCameraComponent()->FieldOfView, 0.1f);
    MissionCameraReadyChecks = bReady ? MissionCameraReadyChecks + 1 : 0;
    // Allow 1.5 seconds of stable camera time for the fully rendered scene to settle.
    // Measure elapsed time, since timer callbacks can catch up within one frame.
    if (!bReady)
    {
        MissionCameraStableSince = -1.0;
        return;
    }
    const double Now = World->GetRealTimeSeconds();
    if (MissionCameraStableSince < 0.0) MissionCameraStableSince = Now;
    if (MissionCameraReadyChecks < 2 || Now - MissionCameraStableSince < 1.5) return;
    if (bLiveMissionStarted)
    {
        if (auto* Timer = GetGameInstance()->GetSubsystem<UTimerSubsystem>()) Timer->StartMissionRun(true);
        if (auto* Runtime = GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>()) Runtime->SetPaused(false);
    }
    for (const TWeakObjectPtr<AActor>& Actor : MissionSunHiddenActors)
        if (Actor.IsValid()) Actor->SetActorHiddenInGame(false);
    MissionSunHiddenActors.Empty();
    MissionTitleRevealTime = World->GetTimeSeconds();
    if (MissionSceneCover.IsValid())
    {
        if (UGameViewportClient* Viewport = World->GetGameViewport())
            Viewport->RemoveViewportWidgetContent(MissionSceneCover.ToSharedRef());
        MissionSceneCover.Reset();
    }
    World->GetTimerManager().ClearTimer(MissionSunRevealTimer);
}

bool UMissionBriefingDlg::EnableMissionMenuInput()
{
    APlayerController* PC = GetOwningPlayer();
    ULocalPlayer* LP = PC ? PC->GetLocalPlayer() : nullptr;
    auto* Input = LP ? LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
    if (!Input || !manager) return false;
    MissionMenuContext = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Input/IMC_Game.IMC_Game"));
    UInputAction* Action = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_Quit.IA_Quit"));
    if (!MissionMenuContext || !Action) return false;
    MissionMenuInput = NewObject<UEnhancedInputComponent>(PC);
    MissionMenuInput->RegisterComponent();
    MissionMenuInput->Priority = 1000;
    GearToggleAction = LoadObject<UInputAction>(nullptr,TEXT("/Game/Input/IA_GearToggle.IA_GearToggle"));
    if (GearToggleAction && GearToggleAction->ValueType == EInputActionValueType::Boolean)
        MissionMenuInput->BindAction(GearToggleAction.Get(),ETriggerEvent::Started,this,&UMissionBriefingDlg::ToggleMissionGear);
    else UE_LOG(LogTemp,Warning,TEXT("[GearInput] Missing Boolean IA_GearToggle."));

    RadioMenuAction = LoadObject<UInputAction>(nullptr,TEXT("/Game/Input/IA_RadioMenu.IA_RadioMenu"));
    if (RadioMenuAction) MissionMenuInput->BindAction(RadioMenuAction.Get(),ETriggerEvent::Started,this,&UMissionBriefingDlg::ToggleFighterRadio);
    RadioChoiceContext = NewObject<UInputMappingContext>(this,NAME_None,RF_Transient);
    RadioChoiceActions.Reset();
    {
        UInputAction* Choice=NewObject<UInputAction>(this,NAME_None,RF_Transient);
        Choice->ValueType=EInputActionValueType::Boolean;
        RadioChoiceActions.Add(Choice);
        RadioChoiceContext->MapKey(Choice,EKeys::Zero);
        MissionMenuInput->BindAction(Choice,ETriggerEvent::Started,this,&UMissionBriefingDlg::SelectFighterRadio0);
    }
    {
        UInputAction* Choice=NewObject<UInputAction>(this,NAME_None,RF_Transient);
        Choice->ValueType=EInputActionValueType::Boolean;
        RadioChoiceActions.Add(Choice);
        RadioChoiceContext->MapKey(Choice,EKeys::One);
        MissionMenuInput->BindAction(Choice,ETriggerEvent::Started,this,&UMissionBriefingDlg::SelectFighterRadio1);
    }
    {
        UInputAction* Choice=NewObject<UInputAction>(this,NAME_None,RF_Transient);
        Choice->ValueType=EInputActionValueType::Boolean;
        RadioChoiceActions.Add(Choice);
        RadioChoiceContext->MapKey(Choice,EKeys::Two);
        MissionMenuInput->BindAction(Choice,ETriggerEvent::Started,this,&UMissionBriefingDlg::SelectFighterRadio2);
    }
    {
        UInputAction* Choice=NewObject<UInputAction>(this,NAME_None,RF_Transient);
        Choice->ValueType=EInputActionValueType::Boolean;
        RadioChoiceActions.Add(Choice);
        RadioChoiceContext->MapKey(Choice,EKeys::Three);
        MissionMenuInput->BindAction(Choice,ETriggerEvent::Started,this,&UMissionBriefingDlg::SelectFighterRadio3);
    }
    {
        UInputAction* Choice=NewObject<UInputAction>(this,NAME_None,RF_Transient);
        Choice->ValueType=EInputActionValueType::Boolean;
        RadioChoiceActions.Add(Choice);
        RadioChoiceContext->MapKey(Choice,EKeys::Four);
        MissionMenuInput->BindAction(Choice,ETriggerEvent::Started,this,&UMissionBriefingDlg::SelectFighterRadio4);
    }
    {
        UInputAction* Choice=NewObject<UInputAction>(this,NAME_None,RF_Transient);
        Choice->ValueType=EInputActionValueType::Boolean;
        RadioChoiceActions.Add(Choice);
        RadioChoiceContext->MapKey(Choice,EKeys::Five);
        MissionMenuInput->BindAction(Choice,ETriggerEvent::Started,this,&UMissionBriefingDlg::SelectFighterRadio5);
    }

    // Transient context remains for the wheel and existing HUD shortcuts only.
    MissionTargetContext = NewObject<UInputMappingContext>(this, NAME_None, RF_Transient);
    // Use the editor-authored actions and IMC mappings, including chord triggers.
    PreviousMissionTargetAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_PreviousTarget.IA_PreviousTarget"));
    NextMissionTargetAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_NextTarget.IA_NextTarget"));
    bPreviousTargetHeld = bNextTargetHeld = false;
    if (PreviousMissionTargetAction && PreviousMissionTargetAction->ValueType == EInputActionValueType::Boolean)
    {
        MissionMenuInput->BindAction(PreviousMissionTargetAction.Get(), ETriggerEvent::Triggered, this, &UMissionBriefingDlg::PreviousMissionTarget);
        MissionMenuInput->BindAction(PreviousMissionTargetAction.Get(), ETriggerEvent::Completed, this, &UMissionBriefingDlg::ReleasePreviousMissionTarget);
        MissionMenuInput->BindAction(PreviousMissionTargetAction.Get(), ETriggerEvent::Canceled, this, &UMissionBriefingDlg::ReleasePreviousMissionTarget);
    }
    else UE_LOG(LogTemp, Warning, TEXT("[MissionInput] Missing Boolean IA_PreviousTarget."));
    if (NextMissionTargetAction && NextMissionTargetAction->ValueType == EInputActionValueType::Boolean)
    {
        MissionMenuInput->BindAction(NextMissionTargetAction.Get(), ETriggerEvent::Triggered, this, &UMissionBriefingDlg::NextMissionTarget);
        MissionMenuInput->BindAction(NextMissionTargetAction.Get(), ETriggerEvent::Completed, this, &UMissionBriefingDlg::ReleaseNextMissionTarget);
        MissionMenuInput->BindAction(NextMissionTargetAction.Get(), ETriggerEvent::Canceled, this, &UMissionBriefingDlg::ReleaseNextMissionTarget);
    }
    else UE_LOG(LogTemp, Warning, TEXT("[MissionInput] Missing Boolean IA_NextTarget."));
    MissionMenuInput->BindAction(Action, ETriggerEvent::Started, this, &UMissionBriefingDlg::ToggleMissionMenu);
    // Use the transient wheel context so asset paths, Hold triggers,
    // or boolean action settings cannot suppress the one-frame wheel pulse.
    MissionTargetZoomAction = NewObject<UInputAction>(this, NAME_None, RF_Transient);
    MissionTargetZoomAction->ValueType = EInputActionValueType::Axis1D;
    MissionTargetZoomAction->bConsumeInput = true;
    MissionTargetContext->MapKey(MissionTargetZoomAction.Get(), EKeys::MouseWheelAxis);
    MissionMenuInput->BindAction(MissionTargetZoomAction.Get(), ETriggerEvent::Triggered,
        this, &UMissionBriefingDlg::OnTargetZoom);
    FighterHUDToggleAction = NewObject<UInputAction>(this, NAME_None, RF_Transient);
    FighterHUDToggleAction->ValueType = EInputActionValueType::Boolean;
    MissionTargetContext->MapKey(FighterHUDToggleAction.Get(), EKeys::H);
    MissionMenuInput->BindAction(FighterHUDToggleAction.Get(), ETriggerEvent::Started,
        this, &UMissionBriefingDlg::ToggleFighterHUD);
    bFighterHUDVisible = true;
    if (!FighterHUDPanels.IsValid() && GetWorld() && GetWorld()->GetGameViewport())
    {
        TWeakObjectPtr<UMissionBriefingDlg> HUDOwner(this);
        FighterHUDPanels = SNew(SFighterHUDPanels).ShowPanels_Lambda([HUDOwner]()
        {
            return HUDOwner.IsValid() && HUDOwner->bLiveMissionStarted &&
                HUDOwner->bFighterHUDVisible && !HUDOwner->MissionSceneCover.IsValid() &&
                !HUDOwner->bMissionControlsOpen &&
                !(HUDOwner->MissionQuitMenu && HUDOwner->MissionQuitMenu->IsMenuShown());
        });
        GetWorld()->GetGameViewport()->AddViewportWidgetContent(FighterHUDPanels.ToSharedRef(), 800);
    }
    FighterInputContext = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Input/IMC_Fighter.IMC_Fighter"));
    // Compatibility with the existing asset's current name.
    if (!FighterInputContext)
        FighterInputContext = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Input/IMC_FighterHUD.IMC_FighterHUD"));
    bAddedFighterInputContext = FighterInputContext && !Input->HasMappingContext(FighterInputContext.Get());
    if (bAddedFighterInputContext) Input->AddMappingContext(FighterInputContext.Get(), 1000);
    if (!FighterInputContext)
        UE_LOG(LogTemp, Warning, TEXT("[FighterHUD] Missing IMC_Fighter (or IMC_FighterHUD) in /Game/Input."));
    WeaponsPanelAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_WeaponsPanel.IA_WeaponsPanel"));
    if (WeaponsPanelAction && WeaponsPanelAction->ValueType == EInputActionValueType::Boolean)
        MissionMenuInput->BindAction(WeaponsPanelAction.Get(), ETriggerEvent::Started, this, &UMissionBriefingDlg::ToggleFighterWeaponsPanel);
    else UE_LOG(LogTemp, Warning, TEXT("[FighterHUD] Missing Boolean IA_WeaponsPanel in /Game/Input."));
    HUDWarningsAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_HUDWarnings.IA_HUDWarnings"));
    if (HUDWarningsAction && HUDWarningsAction->ValueType == EInputActionValueType::Boolean)
        MissionMenuInput->BindAction(HUDWarningsAction.Get(), ETriggerEvent::Started, this, &UMissionBriefingDlg::ToggleFighterCautionPanel);
    else UE_LOG(LogTemp, Warning, TEXT("[FighterHUD] Missing Boolean IA_HUDWarnings in /Game/Input."));
    FighterMFDLeftAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_MFDLeftCycle.IA_MFDLeftCycle"));
    FighterMFDRightAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_MFDRightCycle.IA_MFDRightCycle"));
    if (FighterMFDLeftAction && FighterMFDLeftAction->ValueType == EInputActionValueType::Boolean)
        MissionMenuInput->BindAction(FighterMFDLeftAction.Get(), ETriggerEvent::Started, this, &UMissionBriefingDlg::CycleFighterMFDLeft);
    else UE_LOG(LogTemp, Warning, TEXT("[FighterHUD] IA_MFDLeftCycle is missing or is not Boolean."));
    if (FighterMFDRightAction && FighterMFDRightAction->ValueType == EInputActionValueType::Boolean)
        MissionMenuInput->BindAction(FighterMFDRightAction.Get(), ETriggerEvent::Started, this, &UMissionBriefingDlg::CycleFighterMFDRight);
    else UE_LOG(LogTemp, Warning, TEXT("[FighterHUD] IA_MFDRightCycle is missing or is not Boolean."));
    FighterWeaponCycleAction = NewObject<UInputAction>(this, NAME_None, RF_Transient);
    FighterWeaponCycleAction->ValueType = EInputActionValueType::Boolean;
    MissionTargetContext->MapKey(FighterWeaponCycleAction.Get(), EKeys::BackSpace);
    MissionMenuInput->BindAction(FighterWeaponCycleAction.Get(), ETriggerEvent::Started, this, &UMissionBriefingDlg::CycleFighterWeapon);
    if (!FighterHUDDetails.IsValid() && GetWorld() && GetWorld()->GetGameViewport())
    {
        TWeakObjectPtr<UMissionBriefingDlg> Owner(this);
        FighterHUDDetails = SNew(SFighterHUDDetails)
            .ShowHUD_Lambda([Owner]() { return Owner.IsValid() && Owner->bFighterHUDVisible && Owner->CanUseFighterHUD(); })
            .SelectedTarget_Lambda([Owner]() { return Owner.IsValid() ? Owner->SelectedMissionTarget : TWeakObjectPtr<AActor>(); })
            .SelectedName_Lambda([Owner]() { return Owner.IsValid() ? Owner->SelectedMissionTargetName : FText::GetEmpty(); });
        GetWorld()->GetGameViewport()->AddViewportWidgetContent(FighterHUDDetails.ToSharedRef(), 810);
    }
    Input->AddMappingContext(MissionTargetContext.Get(), 1001);
    UE_LOG(LogTemp, Log, TEXT("[MissionZoom] MouseWheelAxis bound in mission context (priority 1001)."));

    bAddedMissionMenuContext = !Input->HasMappingContext(MissionMenuContext.Get());
    if (bAddedMissionMenuContext) Input->AddMappingContext(MissionMenuContext.Get(), 1000);
    PC->PushInputComponent(MissionMenuInput.Get());
    // Existing briefing Blueprint defaults may still have None or the native
    // class saved. Resolve the designed menu rather than displaying plain Slate.
    if (!QuitMissionMenuClass || QuitMissionMenuClass == UQuitMissionMenu::StaticClass())
    {
        QuitMissionMenuClass = LoadClass<UQuitMissionMenu>(nullptr,
            TEXT("/Game/Screens/Mission/WBP_QuitMissionMenu.WBP_QuitMissionMenu_C"));
        if (!QuitMissionMenuClass)
        {
            UE_LOG(LogTemp, Error, TEXT("[MissionBriefing] WBP_QuitMissionMenu could not load. Compile/save it and verify its parent is QuitMissionMenu."));
        }
    }
    UE_LOG(LogTemp, Log, TEXT("[MissionBriefing] Quit menu widget class: %s"), *GetNameSafe(QuitMissionMenuClass.Get()));
    MissionQuitMenu = new QuitView(nullptr);
    TWeakObjectPtr<UMissionBriefingDlg> WeakThis(this);
    MissionQuitMenu->Configure(PC, [WeakThis](uintptr_t Choice)
    {
        if (WeakThis.IsValid()) WeakThis->HandleMissionMenuAction(Choice);
    }, QuitMissionMenuClass);
    return true;
}
void UMissionBriefingDlg::DisableMissionMenuInput()
{
    if (FighterHUDDetails.IsValid() && GetWorld() && GetWorld()->GetGameViewport())
        GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(FighterHUDDetails.ToSharedRef());
    FighterHUDDetails.Reset();
    WeaponsPanelAction = nullptr;
    HUDWarningsAction = nullptr;
    FighterMFDLeftAction = nullptr;
    FighterMFDRightAction = nullptr;
    FighterWeaponCycleAction = nullptr;
    if (FighterHUDPanels.IsValid() && GetWorld() && GetWorld()->GetGameViewport())
        GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(FighterHUDPanels.ToSharedRef());
    FighterHUDPanels.Reset();
    FighterHUDToggleAction = nullptr;
    bFighterHUDVisible = true;
    if (manager) manager->MissionOptionsReturn = nullptr;
    if (bMissionControlsOpen)
    {
        if (manager) manager->HideOptionsScreen();
        UGameplayStatics::SetGamePaused(GetWorld(), bControlsPreviousWorldPaused);
        if (auto* Runtime = GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>())
            Runtime->SetPaused(bControlsPreviousRuntimePaused);
        bMissionControlsOpen = false;
    }
    if (MissionQuitMenu) { delete MissionQuitMenu; MissionQuitMenu = nullptr; }
    if (APlayerController* PC = GetOwningPlayer())
    {
        if (MissionMenuInput) PC->PopInputComponent(MissionMenuInput.Get());
        if (ULocalPlayer* LP = PC->GetLocalPlayer())
            if (auto* Input = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
            {
                if (RadioChoiceContext) Input->RemoveMappingContext(RadioChoiceContext.Get());
                if (MissionTargetContext) Input->RemoveMappingContext(MissionTargetContext.Get());
                if (bAddedFighterInputContext && FighterInputContext)
                    Input->RemoveMappingContext(FighterInputContext.Get());
                if (bAddedMissionMenuContext && MissionMenuContext)
                    Input->RemoveMappingContext(MissionMenuContext.Get());
            }
    }
    if (MissionMenuInput) MissionMenuInput->DestroyComponent();
    RadioChoiceActions.Reset(); RadioChoiceContext = nullptr; RadioMenuAction = nullptr;
    GearToggleAction = nullptr;
    MissionMenuInput = nullptr;
    MissionMenuContext = nullptr;
    MissionTargetContext = nullptr;
    PreviousMissionTargetAction = nullptr;
    NextMissionTargetAction = nullptr;
    MissionTargetZoomAction = nullptr;
    MissionTargetZoomDistance = 1.0;
    bAddedMissionMenuContext = false;
    FighterInputContext = nullptr;
    bAddedFighterInputContext = false;
}
void UMissionBriefingDlg::PreviousMissionTarget()
{
    if (bPreviousTargetHeld) return;
    bPreviousTargetHeld = true;
    CycleMissionTarget(-1);
}
void UMissionBriefingDlg::ReleasePreviousMissionTarget() { bPreviousTargetHeld = false; }
void UMissionBriefingDlg::NextMissionTarget()
{
    if (bNextTargetHeld) return;
    bNextTargetHeld = true;
    // Do not also advance when the user invokes Shift+T for reverse targeting.
    APlayerController* PC = GetOwningPlayer();
    if (PC && (PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift))) return;
    CycleMissionTarget(1);
}
void UMissionBriefingDlg::ReleaseNextMissionTarget() { bNextTargetHeld = false; }
void UMissionBriefingDlg::CycleMissionTarget(int32 Direction)
{
    if (!bLiveMissionStarted || MissionSceneCover.IsValid() || bMissionControlsOpen ||
        (MissionQuitMenu && MissionQuitMenu->IsMenuShown()) || !IsValid(MissionPreviewCamera)) return;
    Sim* Simulation = Sim::GetSim();
    SimRegion* Region = Simulation ? Simulation->GetActiveRegion() : nullptr;
    if (!Region) return;
    TArray<AShipActor*> Candidates;
    for (TActorIterator<AShipActor> It(GetWorld()); It; ++It)
    {
        AShipActor* Actor = *It;
        if (!IsValid(Actor)) continue;
        Ship* ShipData = Actor->GetRuntimeShip();
        const bool bInRegion = ShipData && Region->GetShips().contains(ShipData);
        // Never dereference an unowned runtime pointer for diagnostics.
        const bool bDead = bInRegion && ShipData->IsDead();
        const bool bDestroying = Actor->IsActorBeingDestroyed();
        const bool bHidden = Actor->IsHidden();
        const bool bEligible = bInRegion && !bDead && !bDestroying && !bHidden;
        UE_LOG(LogTemp, Display,
            TEXT("[MissionTargetCycle] Actor='%s' Ship='%s' ActiveRegion='%hs' Bound=%d InRegion=%d Hidden=%d Destroying=%d Dead=%d Eligible=%d"),
            *Actor->GetName(), bInRegion ? ANSI_TO_TCHAR(ShipData->GetName()) : TEXT("<unbound or outside active region>"),
            Region->GetName(), ShipData != nullptr, bInRegion, bHidden, bDestroying, bDead, bEligible);
        if (bEligible) Candidates.Add(Actor);
    }
    Candidates.Sort([](const AShipActor& A, const AShipActor& B)
    {
        const FString AName = ANSI_TO_TCHAR(A.GetRuntimeShip()->GetName());
        const FString BName = ANSI_TO_TCHAR(B.GetRuntimeShip()->GetName());
        return AName == BName ? A.GetUniqueID() < B.GetUniqueID() : AName < BName;
    });
    if (Candidates.IsEmpty())
    {
        SelectedMissionTarget.Reset();
        return;
    }
    const int32 Current = Candidates.IndexOfByPredicate([this](AShipActor* Actor) { return Actor == SelectedMissionTarget.Get(); });
    const int32 Index = Current == INDEX_NONE ? (Direction > 0 ? 0 : Candidates.Num()-1)
        : (Current + (Direction > 0 ? 1 : -1) + Candidates.Num()) % Candidates.Num();
    AShipActor* Selected = Candidates[Index];
    UE_LOG(LogTemp, Display, TEXT("[MissionTargetCycle] Selected='%hs' Index=%d Count=%d"),
        Selected->GetRuntimeShip()->GetName(), Index, Candidates.Num());
    SelectedMissionTarget = Selected;
    MissionTargetZoomDistance = 1.0;
    SelectedMissionTargetLocalBounds = MissionTargetLocalBounds(Selected);
    Ship* SelectedShip = Selected->GetRuntimeShip();
    FString FullName = ANSI_TO_TCHAR(SelectedShip->GetName());
    if (CombatUnit* Unit = SelectedShip->GetCombatUnit())
    {
        const FString Abbreviation = UFormattingUtils::GetUnitDesignIndicator(Unit);
        FString Registry = FString(ANSI_TO_TCHAR(Unit->GetRegistryNumber().data())).TrimStartAndEnd();
        // Normalize CV6, CV-6, and a numeric registry to the same CV-6 prefix.
        if (Registry.StartsWith(Abbreviation, ESearchCase::IgnoreCase))
            Registry.RightChopInline(Abbreviation.Len());
        Registry.TrimStartAndEndInline();
        while (Registry.StartsWith(TEXT("-")))
        {
            Registry.RightChopInline(1);
            Registry.TrimStartInline();
        }
        const FString Prefix = Registry.IsEmpty() ? Abbreviation : Abbreviation + TEXT("-") + Registry;
        if (!FullName.StartsWith(Prefix + TEXT(" "), ESearchCase::IgnoreCase))
            FullName = Prefix + TEXT(" ") + FullName;
    }
    SelectedMissionTargetName = FText::FromString(FullName);
    if (!MissionTargetOverlay.IsValid())
    {
        TWeakObjectPtr<UMissionBriefingDlg> WeakThis(this);
        MissionTargetOverlay = SNew(SMissionTargetOverlay).Controller(GetOwningPlayer())
            .Target_Lambda([WeakThis]() { return WeakThis.IsValid() ? WeakThis->SelectedMissionTarget : TWeakObjectPtr<AActor>(); })
            .LocalBounds_Lambda([WeakThis]() { return WeakThis.IsValid() ? WeakThis->SelectedMissionTargetLocalBounds : FBox(ForceInit); })
            .TargetColor_Lambda([WeakThis]()
            {
                const FLinearColor Neutral = FLinearColor::Yellow;
                if (!WeakThis.IsValid()) return Neutral;
                AShipActor* Actor = Cast<AShipActor>(WeakThis->SelectedMissionTarget.Get());
                Sim* CurrentSim = Sim::GetSim();
                SimRegion* Region = CurrentSim ? CurrentSim->GetActiveRegion() : nullptr;
                Ship* Data = IsValid(Actor) ? Actor->GetRuntimeShip() : nullptr;
                if (!Data || !Region || !Region->GetShips().contains(Data)) return Neutral;
                const int32 IFF = Data->GetIFF();
                if (IFF <= 0) return Neutral;
                Mission* CurrentMission = CurrentSim->GetMission();
                if (!CurrentMission) return Neutral;
                return IFF == CurrentMission->GetTeam()
                    ? FLinearColor(0.25f, 0.65f, 1.0f, 1.0f)
                    : FLinearColor::Red;
            })
            .TargetName_Lambda([WeakThis]() { return WeakThis.IsValid() ? WeakThis->SelectedMissionTargetName : FText::GetEmpty(); });
        GetWorld()->GetGameViewport()->AddViewportWidgetContent(MissionTargetOverlay.ToSharedRef(), 900);
    }
    MissionTargetLastUpdate = GetWorld()->GetTimeSeconds();
    FTimerManagerTimerParameters TimerParameters;
    TimerParameters.bLoop = true;
    TimerParameters.bMaxOncePerFrame = true;
    GetWorld()->GetTimerManager().SetTimer(MissionTargetCameraTimer, this,
        &UMissionBriefingDlg::UpdateMissionTargetCamera, 1.0f/60.0f, TimerParameters);
}
bool UMissionBriefingDlg::CanUseFighterHUD() const
{
    return bLiveMissionStarted && !MissionSceneCover.IsValid() && !bMissionControlsOpen &&
        !(MissionQuitMenu && MissionQuitMenu->IsMenuShown());
}
void UMissionBriefingDlg::CycleFighterMFDLeft()
{
    if (CanUseFighterHUD() && FighterHUDDetails.IsValid()) FighterHUDDetails->CycleMFD(0);
}
void UMissionBriefingDlg::CycleFighterMFDRight()
{
    if (CanUseFighterHUD() && FighterHUDDetails.IsValid()) FighterHUDDetails->CycleMFD(1);
}
void UMissionBriefingDlg::CycleFighterWeapon()
{
    if (!CanUseFighterHUD() || !FighterHUDDetails.IsValid()) return;
    APlayerController* PC = GetOwningPlayer();
    const bool Primary = PC && (PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift));
    FighterHUDDetails->CycleWeapon(Primary);
}

void UMissionBriefingDlg::ToggleFighterWeaponsPanel()
{
    if (CanUseFighterHUD() && FighterHUDDetails.IsValid()) FighterHUDDetails->ToggleWeaponsPanel();
}

void UMissionBriefingDlg::ToggleFighterCautionPanel()
{
    if (CanUseFighterHUD() && FighterHUDDetails.IsValid()) FighterHUDDetails->ToggleCautionPanel();
}

void UMissionBriefingDlg::ToggleFighterHUD()
{
    if (!bLiveMissionStarted || MissionSceneCover.IsValid() || bMissionControlsOpen ||
        (MissionQuitMenu && MissionQuitMenu->IsMenuShown())) return;
    APlayerController* PC = GetOwningPlayer();
    if (PC && (PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift))) return;
    bFighterHUDVisible = !bFighterHUDVisible;
    if (!bFighterHUDVisible && FighterHUDDetails.IsValid())
    {
        FighterHUDDetails->CloseRadio();
        UpdateFighterRadioInput();
    }
}

void UMissionBriefingDlg::OnTargetZoom(const FInputActionValue& Value)
{
    UE_LOG(LogTemp, Log, TEXT("[MissionZoom] Wheel=%.3f Live=%d Target=%s Cover=%d Controls=%d Menu=%d"),
        Value.Get<float>(), bLiveMissionStarted, *GetNameSafe(SelectedMissionTarget.Get()),
        MissionSceneCover.IsValid(), bMissionControlsOpen,
        MissionQuitMenu && MissionQuitMenu->IsMenuShown());
    if (!bLiveMissionStarted || !SelectedMissionTarget.IsValid() ||
        !IsValid(MissionPreviewCamera) || MissionSceneCover.IsValid() || bMissionControlsOpen ||
        (MissionQuitMenu && MissionQuitMenu->IsMenuShown())) return;
    const float Wheel = Value.Get<float>();
    if (!FMath::IsFinite(Wheel) || FMath::IsNearlyZero(Wheel)) return;
    // Positive wheel input moves closer; the existing camera timer smooths motion.
    MissionTargetZoomDistance = FMath::Clamp(
        MissionTargetZoomDistance * FMath::Pow(0.85, double(FMath::Clamp(Wheel, -10.0f, 10.0f))),
        0.5, 3.0);
    UE_LOG(LogTemp, Log, TEXT("[MissionZoom] Distance multiplier=%.3f"), MissionTargetZoomDistance);
}

void UMissionBriefingDlg::UpdateMissionTargetCamera()
{
    AActor* Target = SelectedMissionTarget.Get();
    if (!IsValid(Target) || Target->IsActorBeingDestroyed() || !IsValid(MissionPreviewCamera))
    {
        SelectedMissionTarget.Reset();
        GetWorld()->GetTimerManager().ClearTimer(MissionTargetCameraTimer);
        return;
    }
    AShipActor* TargetShip = Cast<AShipActor>(Target);
    Sim* Simulation = Sim::GetSim();
    SimRegion* Region = Simulation ? Simulation->GetActiveRegion() : nullptr;
    Ship* Data = TargetShip ? TargetShip->GetRuntimeShip() : nullptr;
    if (!Data || !Region || !Region->GetShips().contains(Data) || Data->IsDead())
    {
        SelectedMissionTarget.Reset();
        GetWorld()->GetTimerManager().ClearTimer(MissionTargetCameraTimer);
        return;
    }
    const double Now = GetWorld()->GetTimeSeconds();
    const double Delta = FMath::Clamp(Now - MissionTargetLastUpdate, 0.0, 0.1);
    MissionTargetLastUpdate = Now;
    if ((MissionQuitMenu && MissionQuitMenu->IsMenuShown()) || bMissionControlsOpen) return;
    int32 Width=0, Height=0;
    APlayerController* PC = GetOwningPlayer();
    if (!PC) return;
    PC->GetViewportSize(Width, Height);
    if (Width <= 0 || Height <= 0) return;
    const FBox Bounds = SelectedMissionTargetLocalBounds.TransformBy(Target->GetActorTransform());
    const double Radius = FMath::Max(10.0, Bounds.GetExtent().Size());
    const double Focal = Width / (2.0 * FMath::Tan(FMath::DegreesToRadians(
        MissionPreviewCamera->GetCameraComponent()->FieldOfView * 0.5)));
    const double PixelRadius = FMath::Min(Width,Height) * 0.20;
    const double Ratio = Focal / PixelRadius;
    const double BaseDistance = Radius * FMath::Sqrt(1.0 + Ratio * Ratio);
    // Keep the camera outside the target bounds even at maximum zoom.
    const double Distance = FMath::Max(Radius + 100.0, BaseDistance * MissionTargetZoomDistance);
    const FVector Desired = Bounds.GetCenter() - MissionPreviewCamera->GetActorForwardVector() * Distance;
    const double Alpha = 1.0 - FMath::Exp(-6.0 * Delta);
    MissionPreviewCamera->SetActorLocation(FMath::Lerp(MissionPreviewCamera->GetActorLocation(), Desired, Alpha));
}

void UMissionBriefingDlg::ToggleMissionMenu()
{
    if (FighterHUDDetails.IsValid()) FighterHUDDetails->CloseRadio();
    UpdateFighterRadioInput();
    if (!MissionQuitMenu || !MissionSystemLevel || MissionSceneCover.IsValid()) return;
    if (MissionQuitMenu->IsMenuShown()) MissionQuitMenu->CloseMenu();
    else
    {
        TArray<UUserWidget*> Screens;
        UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetWorld(), Screens, UGameScreen::StaticClass(), false);
        for (UUserWidget* Widget : Screens)
            if (auto* Screen = Cast<UGameScreen>(Widget))
                if (Screen->IsShown() && Screen->CloseTopmost()) return;
        MissionQuitMenu->ShowMenu();
    }
}
void UMissionBriefingDlg::HandleMissionMenuAction(uintptr_t Action)
{
    if (Action == QuitView::Resume) return;
    if (Action == QuitView::Controls)
    {
        if (!manager) return;
        bMissionControlsOpen = true;
        bControlsPreviousWorldPaused = UGameplayStatics::IsGamePaused(GetWorld());
        if (auto* Runtime = GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>())
            bControlsPreviousRuntimePaused = Runtime->IsPaused();
        // Keep gameplay paused while the existing options UI is active.
        UGameplayStatics::SetGamePaused(GetWorld(), true);
        if (auto* Runtime = GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>()) Runtime->SetPaused(true);
        TWeakObjectPtr<UMissionBriefingDlg> WeakThis(this);
        manager->MissionOptionsReturn = [WeakThis]()
        {
            if (!WeakThis.IsValid()) return;
            auto* Self = WeakThis.Get();
            UGameplayStatics::SetGamePaused(Self->GetWorld(), Self->bControlsPreviousWorldPaused);
            if (auto* Runtime = Self->GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>()) Runtime->SetPaused(Self->bControlsPreviousRuntimePaused);
            Self->bMissionControlsOpen = false;
            if (Self->MissionQuitMenu) Self->MissionQuitMenu->ShowMenu();
        };
        manager->ShowOptionsScreen();
        if (auto* Options = manager->GetOptionsScreen()) Options->ShowCtlDlg();
        else { auto Return = MoveTemp(manager->MissionOptionsReturn); if (Return) Return(); }
        return;
    }
    // Defer cleanup so we never delete QuitView during its button callback.
    TWeakObjectPtr<UMissionBriefingDlg> WeakThis(this);
    GetWorld()->GetTimerManager().SetTimerForNextTick([WeakThis, Action]()
    {
        if (!WeakThis.IsValid()) return;
        auto* Self = WeakThis.Get();
        Sim* Simulation = Sim::GetSim();
        auto* Runtime = Self->GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>();
        if (Runtime) Runtime->SetTimeCompression(1);
        if (Action == QuitView::Accept && Self->Manager && Self->Manager->GetDebriefDlg())
        {
            Self->CloseEmptySectorPreview(true);
            if (Runtime) Runtime->SetGameMode(EGameMode::PLAN);
            Self->Manager->ShowDebriefDlg();
            auto* Debrief = Self->Manager->GetDebriefDlg();
            Debrief->Show();
            if (APlayerController* PC = Self->GetOwningPlayer())
            {
                PC->bShowMouseCursor = true;
                FInputModeGameAndUI Mode;
                Mode.SetWidgetToFocus(Debrief->TakeWidget());
                Mode.SetHideCursorDuringCapture(false);
                PC->SetInputMode(Mode);
            }
            return;
        }
        if (Action == QuitView::Accept)
        {
            if (Simulation) Simulation->CommitMission();
        }
        // Visuals must release Ship* bindings before Sim tears down elements/regions.
        if (Runtime)
        {
            Runtime->SetPaused(true);
            Runtime->EndMissionPresentation();
        }
        TArray<AShipActor*> OldVisuals;
        for (TActorIterator<AShipActor> It(Self->GetWorld()); It; ++It)
            if (It->HasRuntimeShip()) OldVisuals.Add(*It);
        for (AShipActor* Actor : OldVisuals)
        {
            Actor->SetActorTickEnabled(false);
            Actor->BindRuntimeShip(nullptr);
            Actor->Destroy();
        }
        if (Simulation && Simulation->GetMission()) Simulation->UnloadMission();
        else ShipStats::Initialize();
        Self->CloseEmptySectorPreview();
        Self->MissionPtr = nullptr;
        Self->InfoPtr = nullptr;
        if (Action == QuitView::Abort)
            if (Campaign* Camp = Campaign::GetCampaign())
                if (Camp->GetCampaignId() < Campaign::SINGLE_MISSIONS) Camp->RollbackMission();
        if (Runtime) Runtime->SetGameMode(EGameMode::CMPN);
        if (Self->manager) Self->manager->ShowOperationsMissionsDlg();
    });
}

bool UMissionBriefingDlg::StartLiveMission()
{
    if (bLiveMissionStarted) return true;
    if (!MissionPtr || !MissionSystemLevel || !GetGameInstance()) return false;
    auto* Runtime = GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>();
    auto* Environment = GetGameInstance()->GetSubsystem<UStarshatterEnvironmentSubsystem>();
    if (!Runtime || !Environment) return false;
    ASystemSceneBuilder* Builder = nullptr;
    for (TActorIterator<ASystemSceneBuilder> It(GetWorld()); It; ++It)
        if (It->GetLevel() == MissionSystemLevel->GetLoadedLevel()) { Builder = *It; break; }
    if (!Builder || Builder->GetSpawnedRegions().IsEmpty()) return false;
    if (!Sim::GetSim()) Runtime->CreateWorld();
    Sim* Simulation = Sim::GetSim();
    if (!Simulation) return false;
    Runtime->SetPaused(true);
    // Campaign previews can leave a Sim mission loaded. An explicit launch
    // replaces that session; detach visual proxies before Sim deletes its ships.
    if (Simulation->GetMission())
    {
        UE_LOG(LogTemp, Log, TEXT("[MissionBriefing] Clearing previous simulation before selected mission launch"));
        Runtime->EndMissionPresentation();
        TArray<AShipActor*> PreviousVisuals;
        for (TActorIterator<AShipActor> It(GetWorld()); It; ++It)
            if (It->HasRuntimeShip()) PreviousVisuals.Add(*It);
        for (AShipActor* Actor : PreviousVisuals)
        {
            Actor->SetActorTickEnabled(false);
            Actor->BindRuntimeShip(nullptr);
            Actor->Destroy();
        }
        Simulation->UnloadMission();
    }
    Environment->BuildSimRegionsForSim(Simulation);
    Runtime->BeginMissionPresentation(Builder);
    // Use the selected/generated campaign Mission with its current loadout and orders.
    MissionPtr->SetComplete(false);
    Simulation->LoadMission(MissionPtr, false);
    bLiveMissionStarted = true;
    Simulation->ExecMission();
    if (!Simulation->GetActiveRegion()) return false;
    Runtime->SetGameMode(EGameMode::PLAY);
    Runtime->SetPaused(true); // Remain behind the scene cover until camera warmup completes.
    UE_LOG(LogTemp, Log, TEXT("[MissionBriefing] Live mission started: %hs"), MissionPtr->GetName());
    return true;
}

void UMissionBriefingDlg::OnCommit()
{
    // Ignore a repeated launch while this preview is loading or visible.
    if (MissionSystemLevel) return;
    UWorld* World = GetWorld();
    UGameViewportClient* Viewport = World ? World->GetGameViewport() : nullptr;
    if (!MissionPtr || !Viewport)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MissionBriefing] No mission/viewport for sector preview"));
        return;
    }

    FString SystemName = ToTextFromUtf8(MissionPtr->GetSystem()).ToString();
    if (SystemName.IsEmpty())
    {
        if (StarSystem* System = MissionPtr->GetStarSystem())
            SystemName = ToTextFromUtf8(System->GetName()).ToString();
    }
    SystemName.TrimStartAndEndInline();
    const FString PackagePath = FString::Printf(TEXT("/Game/Maps/%s"), *SystemName);
    FString PackageFilename;
    if (SystemName.IsEmpty() || !FPackageName::IsValidLongPackageName(PackagePath) ||
        !FPackageName::DoesPackageExist(PackagePath, &PackageFilename))
    {
        UE_LOG(LogTemp, Error, TEXT("[MissionBriefing] System map not found: %s"), *PackagePath);
        return; // Leave the briefing available when the map cannot be resolved.
    }
    const FString RegionName = ToTextFromUtf8(MissionPtr->GetRegion()).ToString();
    const FText SectorTitle = FText::FromString(FString::Printf(
        TEXT("%s - %s"),
        SystemName.IsEmpty() ? TEXT("Unknown system") : *SystemName,
        RegionName.IsEmpty() ? TEXT("Unknown region") : *RegionName));

    CloseEmptySectorPreview();
    // Keep the world rendering behind an opaque Slate cover so materials,
    // exposure and the nebula can settle without showing their startup frames.
    MissionSceneCover = SNew(SBorder)
        .Visibility(EVisibility::HitTestInvisible)
        .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
        .BorderBackgroundColor(FLinearColor::Black);
    Viewport->AddViewportWidgetContent(MissionSceneCover.ToSharedRef(), 999);
    bool bLoadStarted = false;
    MissionSystemLevel = ULevelStreamingDynamic::LoadLevelInstance(
        World, PackagePath, FVector::ZeroVector, FRotator::ZeroRotator, bLoadStarted);
    if (!bLoadStarted || !MissionSystemLevel)
    {
        CloseEmptySectorPreview();
        UE_LOG(LogTemp, Error, TEXT("[MissionBriefing] Could not start system level load: %s"),
            *PackagePath);
        return;
    }
    if (!EnableMissionMenuInput())
    {
        UE_LOG(LogTemp, Error, TEXT("[MissionBriefing] Missing IMC_Game or IA_Quit"));
        CloseEmptySectorPreview();
        return;
    }
    MissionSystemPackage = PackagePath;
    MissionSystemLevel->OnLevelShown.AddDynamic(
        this, &UMissionBriefingDlg::HandleMissionSystemLevelShown);
    MissionSystemLevel->SetShouldBeLoaded(true);
    MissionSystemLevel->SetShouldBeVisible(true);

    // Hide all menu-owned dialogs, including the briefing's outer host.
    if (manager) manager->Hide();
    if (Manager) Manager->Hide();
    Hide();

    // Stream the authored system map; do not explicitly start the mission simulation.
    // Retain starfield actors and restore visible scene objects on return.
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (MissionSystemLevel && Actor->GetLevel() == MissionSystemLevel->GetLoadedLevel()) continue;
        if (Actor->IsA<APlanetActor>() || Actor->IsA<AGasGiantActor>() ||
            Actor->IsA<ACentralSun>() || Actor->IsA<AShipActor>())
        {
            TArray<AActor*> VisualActors;
            VisualActors.Add(Actor);
            TArray<AActor*> Children;
            Actor->GetAllChildActors(Children, true);
            VisualActors.Append(Children);
            for (AActor* Visual : VisualActors)
            {
                if (IsValid(Visual) && !Visual->IsHidden())
                {
                    PreviewHiddenActors.AddUnique(Visual);
                    Visual->SetActorHiddenInGame(true);
                }
            }
        }
    }

    const auto MissionTitleOpacity = [WeakThis = TWeakObjectPtr<UMissionBriefingDlg>(this)]()
    {
        if (!WeakThis.IsValid()) return 0.0f;
        UWorld* TitleWorld = WeakThis->GetWorld();
        const double Start = WeakThis->MissionTitleRevealTime;
        if (!TitleWorld || Start < 0.0) return 1.0f;
        const double Elapsed = TitleWorld->GetTimeSeconds() - Start;
        return static_cast<float>(1.0 - FMath::Clamp(Elapsed - 10.0, 0.0, 1.0));
    };

    EmptySectorOverlay = SNew(SOverlay)
        .Visibility(EVisibility::HitTestInvisible)
        + SOverlay::Slot()
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Top)
        .Padding(FMargin(32.0f, 24.0f))
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
            [
                SNew(STextBlock)
                .Text(SectorTitle)
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 22))
                .ColorAndOpacity(FLinearColor::White)
                .ShadowOffset(FVector2D(1.0f, 1.0f))
                .ShadowColorAndOpacity(FLinearColor::Black)
            ]
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 6.0f)
            [
                SNew(STextBlock)
                .Text(ToTextFromUtf8(MissionPtr->GetName()))
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 18))
                .ColorAndOpacity_Lambda([MissionTitleOpacity]()
                {
                    return FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, MissionTitleOpacity()));
                })
                .ShadowOffset(FVector2D(1.0f, 1.0f))
                .ShadowColorAndOpacity_Lambda([MissionTitleOpacity]()
                {
                    return FLinearColor(0.0f, 0.0f, 0.0f, MissionTitleOpacity());
                })

            ]
        ];
    Viewport->AddViewportWidgetContent(EmptySectorOverlay.ToSharedRef(), 1000);

    if (APlayerController* PC = GetOwningPlayer())
    {
        FInputModeGameOnly Input;
        PC->SetInputMode(Input);
        PC->bShowMouseCursor = true;
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
void UMissionBriefingDlg::UpdateFighterRadioInput()
{
    APlayerController* PC=GetOwningPlayer();
    ULocalPlayer* LP=PC?PC->GetLocalPlayer():nullptr;
    auto* Input=LP?LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>():nullptr;
    if (!Input || !RadioChoiceContext) return;
    if (FighterHUDDetails.IsValid() && FighterHUDDetails->IsRadioOpen())
    {
        if (!Input->HasMappingContext(RadioChoiceContext.Get())) Input->AddMappingContext(RadioChoiceContext.Get(),1100);
    }
    else Input->RemoveMappingContext(RadioChoiceContext.Get());
}
void UMissionBriefingDlg::ToggleFighterRadio()
{
    if (!CanUseFighterHUD() || !FighterHUDDetails.IsValid()) return;
    bFighterHUDVisible=true;
    FighterHUDDetails->ToggleRadio();
    UpdateFighterRadioInput();
}
void UMissionBriefingDlg::SelectFighterRadio(int32 Number)
{
    if (CanUseFighterHUD() && bFighterHUDVisible && FighterHUDDetails.IsValid()) FighterHUDDetails->RadioSelect(Number);
    UpdateFighterRadioInput();
}
void UMissionBriefingDlg::SelectFighterRadio0() { SelectFighterRadio(0); }
void UMissionBriefingDlg::SelectFighterRadio1() { SelectFighterRadio(1); }
void UMissionBriefingDlg::SelectFighterRadio2() { SelectFighterRadio(2); }
void UMissionBriefingDlg::SelectFighterRadio3() { SelectFighterRadio(3); }
void UMissionBriefingDlg::SelectFighterRadio4() { SelectFighterRadio(4); }
void UMissionBriefingDlg::SelectFighterRadio5() { SelectFighterRadio(5); }

void UMissionBriefingDlg::ToggleMissionGear()
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* S=Sim::GetSim();
    Ship* P=S?S->GetPlayerShip():nullptr;
    SimRegion* R=S?S->GetActiveRegion():nullptr;
    if (!P || !R || !R->GetShips().contains(P) || P->IsDead() || P->IsDying()) return;
    LandingGear* Gear=P->GetGear();
    if (!Gear) { UE_LOG(LogTemp,Warning,TEXT("[GearInput] Player has no authored landing gear.")); return; }
    const int32 Before=int32(Gear->GetState());
    P->ToggleGear();
    UE_LOG(LogTemp,Display,TEXT("[GearInput] Ship='%hs' State=%d -> %d (0 UP, 1 LOWERING, 2 DOWN, 3 RAISING)"),
        P->GetName(),Before,int32(Gear->GetState()));
}
