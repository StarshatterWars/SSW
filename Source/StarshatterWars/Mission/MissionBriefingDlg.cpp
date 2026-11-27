#include "MissionBriefingDlg.h"
#include "CombatDebugDamage.h"
#include "MissionSceneIsolation.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Layout/SSpacer.h"
#include "TrainingReplay.h"
#include "Widgets/SNullWidget.h"
#include "SimDataEvents.h"
#include "AgentAI.h"
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


#include "PanelCloseIcon.h"
#include "MissionViewCamera.h"
#include "StarshatterInputBindings.h"
#include "QuantumDrive.h"
#include "QuantumTravel.h"
#include "QuantumTravelPopup.h"
#include "TravelTransition.h"
#include "QuantumCameraPresentation.h"
#include "AwardsDlg.h"
#include "TransactionDlg.h"
#include "GuildsDlg.h"
#include "MiningDronesDlg.h"
#include "Blueprint/WidgetTree.h"
#include "MissionUIStyle.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ButtonSlot.h"
#include "Components/ScaleBox.h"
#include "Components/CanvasPanel.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "FlightDeck.h"
#include "MissionObjectTarget.h"

#include "Sensor.h"
#include "RadioView.h"
#include "ShipDesign.h"
#include "SSWBootSubsystem.h"
#include "StarshatterControlsSettings.h"
#include "WeaponGroup.h"

#include "EngineeringPopup.h"
#include "EngineeringDlg.h"
#include "WeaponsDlg.h"
#include "ContactsDlg.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/SOverlay.h"
#include "WeaponsPopup.h"
#include "ObjectivesDlg.h"
#include "FlightOpsDlg.h"
#include "NavigationPopup.h"
#include "NavigationDlg.h"
#include "MissionCameraRig.h"
#include "HangarDlg.h"
#include "ShipSkinDlg.h"
#include "SkillsDlg.h"
#include "LoadoutsDlg.h"
#include "ShipFitDlg.h"
#include "ShipInsuranceSubsystem.h"
#include "InsuranceDlg.h"
#include "InputTriggers.h"
#include "InputModifiers.h"
#include "SimContact.h"
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
#include "Game.h"
#include "Sim.h"
#include "TimerSubsystem.h"
#include "SimEvent.h"
#include "FlightTacticalReferenceDlg.h"
#include "CmdIntelDlg.h"
#include "StarshatterAssetRegistrySubsystem.h"
#include "LiveOptionsDlg.h"
#include "CargoManifestDlg.h"
#include "MarketplaceDlg.h"
#include "ProductionDlg.h"
#include "StationServicesDlg.h"
#include "StationServices.h"
#include "StationServicesAI.h"
#include "ShipStoreDlg.h"
#include "ContractsDlg.h"
#include "AgentsDlg.h"
#include "FlightLogbookDlg.h"
#include "Kismet/GameplayStatics.h"

#include "MenuScreen.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Camera/CameraActor.h"
#include "TimerManager.h"
#include "Templates/UnrealTemplate.h"
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
#include "Shield.h"
#include "HUDSounds.h"
#include "HUDView.h"
#include "SimRegion.h"
#include "MissionTargetOverlay.h"
#include "FighterHUDPanels.h"
#include "FighterHUDDetails.h"
#include "FlightPanelStyle.h"
#include "ShipSystemIcon.h"
#include "InMissionButtonStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SViewport.h"
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
#include "WaypointActor.h"
#include "SimElement.h"
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
#include "GameFramework/PlayerInput.h"

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
    Super::NativePreConstruct();
    ApplyBriefingPresentation();

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

TSharedRef<SWidget> UMissionBriefingDlg::RebuildWidget()
{
    // Retain the live mission controller and its timers, but never render the
    // retired Blueprint preparation page, even if another caller reveals it.
    return SNew(SSpacer);
}

void UMissionBriefingDlg::NativeConstruct()
{
    Super::NativeConstruct();
    FSimDataEvents::Changed().Remove(PanelDataHandle);
    AgentAI::OnJournalChanged().Remove(PanelAgentHandle);
    PanelDataHandle=FSimDataEvents::Changed().AddUObject(this,&UMissionBriefingDlg::OnPanelDataChanged);
    PanelAgentHandle=AgentAI::OnJournalChanged().AddWeakLambda(this,[this](){bPanelAttentionDirty=true;});
    bPanelAttentionDirty=true;

    if (RootSizeBox)
    {
        RootSizeBox->SetWidthOverride(1920.f);
        RootSizeBox->SetHeightOverride(1080.f);
    }

    UCanvasPanelSlot* CanvasSlot = RootSizeBox ? Cast<UCanvasPanelSlot>(RootSizeBox->Slot) : nullptr;
    if (CanvasSlot)
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
        PlayerNameText->SetText(FText::FromString(PlayerInfo.GetRankedDisplayName()));

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

    ApplyBriefingPresentation();
    BuildMenuButtons();
    InitializeSubPanels();
    BuildBriefingChrome();
    RefreshHeader();

    UE_LOG(LogTemp, Warning, TEXT("[MissionBriefingDlg] NativeConstruct: UI initialized"));
}

void UMissionBriefingDlg::NativeDestruct()
{
    FSimDataEvents::Changed().Remove(PanelDataHandle);PanelDataHandle.Reset();
    AgentAI::OnJournalChanged().Remove(PanelAgentHandle);PanelAgentHandle.Reset();

    if (Sim* Simulation = Sim::GetSim()) Simulation->OnMissionUnloading.RemoveAll(this);
    if (UGameInstance* GI = GetGameInstance()) {
        if (UTimerSubsystem* Timer = GI->GetSubsystem<UTimerSubsystem>()) {
            Timer->OnUniverseSecond.RemoveAll(this);
            Timer->OnUniverseMinute.RemoveAll(this);
            Timer->OnCampaignTPlusChanged.RemoveAll(this);
        }
    }
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

void UMissionBriefingDlg::ExecFrame(double /*DeltaTime*/)
{
    if (bStandaloneMission)
        return; // No story campaign score, unit or T+ for standalone missions.

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
        PlayerScoreText->SetText(FText::AsNumber(TeamScore));
    }
}
void UMissionBriefingDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    // Flight updates use the world timer even while this menu widget is collapsed.
    if (!bLiveMissionStarted)
    {
        ClearMissionRotationInput();
        if (APlayerController* PC=GetOwningPlayer())
            if (PC->WasInputKeyJustPressed(StarshatterInputBindings::ResolvedKey(TEXT("SSW_BriefingAccept"),EKeys::Enter))) OnCommit();
        ExecFrame();
    }
}

static SimElement* FindTargetPracticeWave(Sim* Simulation)
{
    const Mission* MissionDef = Simulation ? Simulation->GetMission() : nullptr;
    if (!MissionDef || MissionDef->GetIdentity() != 102 ||
        MissionDef->GetMissionType() != int(EMissionType::TRAINING))
        return nullptr;

    for (const char* Name : {"Alpha", "Bravo", "Delta"})
    {
        SimElement* Group = Simulation->FindElement(Name);
        for (int Index = 1; Group && Index <= Group->GetNumShips(); ++Index)
        {
            const Ship* Craft = Group->GetShip(Index);
            if (Craft && !Craft->IsDead() && !Craft->IsDying())
                return Group;
        }
    }
    return nullptr;
}

void UMissionBriefingDlg::OnPanelDataChanged(const FEconomyState& State)
{if(const auto* Simulation=Sim::GetSim();Simulation && &Simulation->ReadEconomy()==&State)bPanelAttentionDirty=true;}

void UMissionBriefingDlg::UpdatePanelAttention()
{
    if(!bPanelAttentionDirty || FPlatformTime::Seconds()<NextPanelAttentionDispatch)return;
    bPanelAttentionDirty=false;NextPanelAttentionDispatch=FPlatformTime::Seconds()+.5;
    auto* Players=GetGameInstance()?GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>():nullptr;
    auto* Simulation=Sim::GetSim();
    if(!Players || !Simulation)return;
    const auto& Info=Players->GetPlayerInfo();
    PanelAttention.BeginSession(FString::Printf(TEXT("%s|%d|%s"),*Players->GetSlotName(),Info.Id,*Info.Name));
    const auto& State=Simulation->ReadEconomy();
    const auto* Craft=Simulation->GetPlayerShip();
    const FName Owner=FEconomyLedger::Pilot();
    auto Sample=[this](const TCHAR* Name,uint32 Revision,const TSharedPtr<SWidget>& Panel)
    {PanelAttention.Observe(FName(Name),Revision,Panel.IsValid());};
    uint32 Wallet=GetTypeHash(State.Wallets.FindRef(Owner));
    for(const auto& Entry:State.Journal)
        if(Entry.From==Owner || Entry.To==Owner || Entry.WalletDeltas.Contains(Owner))Wallet=HashCombine(Wallet,GetTypeHash(Entry.Id));
    uint32 Skills=0;
    for(const auto& Pair:Info.Skills)Skills^=HashCombine(GetTypeHash(Pair.Key),HashCombine(GetTypeHash(Pair.Value.TrainedLevel),GetTypeHash(Pair.Value.bAdvancedBookOwned)));
    for(const auto& Entry:Info.SkillQueue)Skills^=HashCombine(GetTypeHash(Entry.Id),GetTypeHash(Entry.TargetLevel));
    Sample(TEXT("Player Logbook"),HashCombine(Wallet,HashCombine(Skills,GetTypeHash(Info.HangarShips.Num()))),PlayersPopup);
    uint32 Cargo=0;
    if(Craft)if(const auto* Inventory=State.Inventories.Find(Craft->GetCargoInventoryKey()))
        for(const auto& Item:Inventory->Items)Cargo^=HashCombine(GetTypeHash(Item.Id),HashCombine(GetTypeHash(Item.Type),GetTypeHash(Item.Quantity)));
    Sample(TEXT("Cargo Manifest"),Cargo,CargoManifestPopup);
    uint32 Jobs=0;
    for(const auto& Job:State.Jobs)Jobs^=HashCombine(GetTypeHash(Job.Id),GetTypeHash(static_cast<uint8>(Job.State)));
    Sample(TEXT("Production Facilities"),Jobs,ProductionPopup);
    uint32 Contracts=0;
    for(const auto& Pair:State.Contracts)Contracts^=HashCombine(GetTypeHash(Pair.Key),HashCombine(GetTypeHash(static_cast<uint8>(Pair.Value.State)),GetTypeHash(Pair.Value.HighBid)));
    Sample(TEXT("Contracts"),Contracts,ContractsPopup);
    const uint32 Missions=HashCombine(Skills,HashCombine(GetTypeHash(Info.AgentJournalJson),HashCombine(GetTypeHash(Info.DynamicAssignmentsJson),GetTypeHash(Info.CompletedMissions.Num()))));
    Sample(TEXT("Agents"),Missions,AgentsPopup);Sample(TEXT("Mission"),Missions,MissionPopup);Sample(TEXT("Objectives"),Missions,ObjectivesPopup);
    const auto* Story=Campaign::GetCampaign();
    Sample(TEXT("Intel"),Story?HashCombine(GetTypeHash(Story->GetCampaignId()),GetTypeHash(Story->CountNewEvents())):0,IntelPopup);
}

void UMissionBriefingDlg::UpdateLiveMission()
{
    if (!bLiveMissionStarted) return;
    SyncMissionNavigationTarget();
    UpdatePanelAttention();

#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
    if (GetDefault<USSWDiagnosticsSettings>()->bEnableCombatDebugMode &&
        GetWorld() && !GetWorld()->IsPaused())
    {
        APlayerController* PC = GetOwningPlayer();
        if (PC && PC->WasInputKeyJustPressed(EKeys::X) &&
            (PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift)))
        {
            Sim* Simulation = Sim::GetSim();
            const Ship* Pilot = Simulation ? Simulation->GetPlayerShip() : nullptr;
            SimObject* Target = Pilot ? Pilot->GetTarget() : nullptr;
            Ship* Enemy = Target && Target->GetType() == ESimObject::SHIP
                ? static_cast<Ship*>(Target) : nullptr;
            const auto IsHostileContact = [Pilot](const Ship* Contact) {
                return Pilot && Contact && Contact->GetIFF() > 0 &&
                    Contact->GetIFF() != Pilot->GetIFF() && Contact->GetRegion() == Pilot->GetRegion() &&
                    !Contact->IsDead() && !Contact->IsDying() && Contact->Design();
            };
            if (!IsHostileContact(Enemy))
            {
                // Ground contacts can be selected on the HUD without a weapons lock.
                const AShipActor* Selected = Cast<AShipActor>(SelectedMissionTarget.Get());
                Enemy = Selected ? Selected->GetRuntimeShip() : nullptr;
            }
            if (IsHostileContact(Enemy))
            {
                ApplyCombatDebugDamage(Pilot, Enemy, Enemy->Design()->integrity * 0.5);
                RadioView::Message("Combat debug damage applied.");
            }
            else
            {
                RadioView::Message("Combat debug: select a hostile contact first.");
            }
        }
    }
#endif

    // Fallback for a plain mapped Boolean key when another action consumes
    // IA_AutoNav before its callback. The callback and this path are deduplicated.
    // Complex/chorded mappings remain owned by Enhanced Input.
    if (CanUseFighterHUD() && AutoNavInputContext && GetWorld() && !GetWorld()->IsPaused()) {
        if (APlayerController* AutoPC = GetOwningPlayer()) {
            for (const FEnhancedActionKeyMapping& AutoMapping : AutoNavInputContext->GetMappings()) {
                if (AutoMapping.Action && AutoMapping.Action->GetName().Equals(TEXT("IA_AutoNav"), ESearchCase::IgnoreCase) &&
                    AutoMapping.Triggers.Num() == 0 && AutoMapping.Modifiers.Num() == 0 &&
                    AutoMapping.Key.IsValid() && AutoPC->WasInputKeyJustPressed(AutoMapping.Key)) {
                    ToggleMissionAutoNav();
                    break;
                }
            }
        }
    }

    // Match the autonav fallback for a plain key consumed by another flight action.
    if (CanOpenFlightPanel() && RadioMenuAction && AutoNavInputContext && GetWorld() && !GetWorld()->IsPaused() &&
        RadioMenuAction->Triggers.IsEmpty() && RadioMenuAction->Modifiers.IsEmpty()) {
        if (APlayerController* PC = GetOwningPlayer()) {
            for (const FEnhancedActionKeyMapping& Mapping : AutoNavInputContext->GetMappings()) {
                if (Mapping.Action.Get() == RadioMenuAction.Get() && Mapping.Triggers.IsEmpty() &&
                    Mapping.Modifiers.IsEmpty() && Mapping.Key.IsValid() && PC->WasInputKeyJustPressed(Mapping.Key)) {
                    ToggleFighterRadio();
                    break;
                }
            }
        }
    }

    // Resolve hold state from the configured key, independent of action-event
    // consumption or one-shot Hold triggers. Tab is also an explicit temporary hold fallback.
    if (CanUseFighterHUD() && GetWorld() && !GetWorld()->IsPaused())
    {
        if (APlayerController* PC=GetOwningPlayer())
        {
            bool bHeld=PC->IsInputKeyDown(EKeys::Tab);
            if (AutoNavInputContext) for(const auto& Mapping:AutoNavInputContext->GetMappings())
            {
                if(!Mapping.Action || Mapping.Action->GetName()!=TEXT("IA_Augmenter") || Mapping.Modifiers.Num()!=0)continue;
                bool bSimple=true;
                for(const auto& Trigger:Mapping.Triggers)
                    if(Trigger && !Trigger->IsA<UInputTriggerHold>())bSimple=false;
                if(!bSimple || !Mapping.Key.IsValid())continue;
                bHeld|=PC->IsInputKeyDown(Mapping.Key);
            }
            ApplyMissionAugmenter(bHeld && !StarshatterInputBindings::IsModifierDown(PC,TEXT("Shift")));
        }
    }

    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) ClearMissionRotationInput();

    ExecFrame();
    if (bLiveMissionStarted && !bMissionCompleteHandedOff)
    {
        Sim* Simulation = Sim::GetSim();
        Ship* Pilot = Simulation ? Simulation->GetPlayerShip() : nullptr;
        SimElement* Flight = Pilot ? Pilot->GetElement() : nullptr;
        if (!bPrimaryObjectivesAnnounced && Flight && !Pilot->IsDead() && !Pilot->IsDying())
        {
            bool bHasPrimary = false;
            bool bComplete = true;
            for (int32 Index = 0; Index < Flight->NumObjectives(); ++Index)
            {
                const Instruction* Objective = Flight->GetObjective(Index);
                if (!Objective || Objective->GetPriority() != Instruction::PRIMARY) continue;
                bHasPrimary = true;
                bComplete &= Objective->GetStatus() == INSTRUCTION_STATUS::COMPLETE;
            }
            if (bHasPrimary && bComplete)
            {
                bPrimaryObjectivesAnnounced = true;
                RadioView::Message("Mission objectives complete.");
                if (Instruction* Return = Pilot->GetPendingRTBWaypoint();
                    Return && Pilot->CanRequestAutoLand() && !Pilot->GetInbound())
                {
                    // The wingman follows the same flight plan. Skip remaining
                    // outbound legs without reporting them as reached.
                    ListIter<Instruction> Route = Flight->GetFlightPlan();
                    while (++Route && Route.value() != Return)
                        if (Route->GetStatus() <= INSTRUCTION_STATUS::ACTIVE)
                            Route->SetStatus(INSTRUCTION_STATUS::SKIPPED);
                    Flight->SetHoldTime(0);
                    Pilot->SetAutoLand(true);
                }
            }
        }
        // A recovered player stays parked until the debrief has been presented.
        if (Simulation && MissionPtr && !MissionPtr->IsFreeFlight() &&
            (Simulation->IsComplete() || Simulation->HasPlayerDocked()))
        {
            UMissionDebriefDlg* Debrief = Manager ? Manager->GetDebriefDlg()
                : manager ? manager->GetMissionDebriefDlg() : nullptr;
            if (!Debrief) return;
            if (MissionCameraRig) MissionCameraRig->HoldRecoveryView(GetOwningPlayer(),MissionPreviewCamera.Get());
            bMissionCompleteHandedOff = true;
            DisableMissionMenuInput();
            if (auto* Runtime = GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>())
            {
                Runtime->SetPaused(true);
                Runtime->SetGameMode(EGameMode::PLAN);
            }
            if (auto* Timer = GetGameInstance()->GetSubsystem<UTimerSubsystem>()) Timer->StopMissionRun();
            if (Manager)
            {
                Manager->ShowDebriefDlg(CampaignPtr && CampaignPtr->IsTraining() ? CampaignPtr : nullptr);
                Debrief->Show();
            }
            else manager->ShowMissionDebriefDlg(CampaignPtr && CampaignPtr->IsTraining() ? CampaignPtr : nullptr);
            UE_LOG(LogTemp,Display,TEXT("[MissionRecovery] Debrief opened after completion/recovery"));
            if (APlayerController* PC = GetOwningPlayer())
            {
                PC->bShowMouseCursor = true;
                FInputModeGameAndUI Mode;
                Mode.SetWidgetToFocus(Debrief->TakeWidget());
                Mode.SetHideCursorDuringCapture(false);
                PC->SetInputMode(Mode);
            }
            // Give Slate a frame to display the debrief before removing the
            // landed fighter/camera/HUD. Retain simulation data for debrief stats
            // until its accept action unloads the mission.
            const Mission* FinishedMission = Simulation->GetMission();
            TWeakObjectPtr<UMissionBriefingDlg> WeakThis(this);
            TWeakObjectPtr<UMissionDebriefDlg> WeakDebrief(Debrief);
            GetWorld()->GetTimerManager().SetTimerForNextTick([WeakThis,WeakDebrief,FinishedMission]()
            {
                Sim* LiveSim = Sim::GetSim();
                if (!WeakThis.IsValid() || !WeakDebrief.IsValid() || !LiveSim
                    || LiveSim->GetMission()!=FinishedMission || !WeakDebrief->IsInViewport()
                    || WeakDebrief->GetVisibility()!=ESlateVisibility::Visible) return;
                WeakThis->CloseEmptySectorPreview(true);
                LiveSim->CommitMission();
                UE_LOG(LogTemp,Display,TEXT("[MissionRecovery] Flight presentation removed after visible debrief"));
            });
        }
    }
}

FText UMissionBriefingDlg::ToTextFromUtf8(const char* Utf8)
{
    if (!Utf8 || !Utf8[0])
    {
        return FText::GetEmpty();
    }

    return FText::FromString(FString(UTF8_TO_TCHAR(Utf8)));
}

void UMissionBriefingDlg::ApplyBriefingPresentation()
{
    if (BriefingFrame || !bModernBriefingPresentation) return;

    // Keep Blueprint fonts and geometry; adjust only the briefing's visual hierarchy.
    auto StyleText = [](UTextBlock* Text, int32 Size, const FLinearColor& Color)
    {
        if (!Text) return;
        FSlateFontInfo Font = Text->GetFont();
        Font.Size = Size;
        Text->SetFont(Font);
        Text->SetColorAndOpacity(FSlateColor(Color));
    };
    StyleText(TitleText, 24, BriefingAccentColor);
    StyleText(MissionNameText, 28, BriefingTextColor);
    for (UTextBlock* Text : { MissionSystemText, MissionSectorText,
        MissionTimeStartText, MissionTimeTargetText })
    {
        StyleText(Text, 18, BriefingTextColor);
    }
    StyleText(MissionTimeTargetLabelText, 16, BriefingAccentColor);

    if (MissionButton)
        MissionButton->SetToolTipText(NSLOCTEXT("MissionBriefing", "AcceptTip", "Accept the mission using the current briefing and loadout."));
    if (ReturnButton)
        ReturnButton->SetToolTipText(NSLOCTEXT("MissionBriefing", "CancelTip", "Return without launching the mission."));
}


void UMissionBriefingDlg::BuildBriefingChrome()
{
    if (!WidgetTree || !MissionSwitcher) return;
    if (BriefingFrame)
    {
        if (TitleText) TitleText->SetText(NSLOCTEXT("MissionBriefing", "HeaderTitle", "MISSION BRIEFING"));
        RefreshBriefingTabStyles();
        return;
    }

    UCanvasPanel* Root = Cast<UCanvasPanel>(WidgetTree->RootWidget);
    if (!Root && RootSizeBox)
    {
        // Preserve the existing outer sizing when the Blueprint root is not a canvas.
        Root = WidgetTree->ConstructWidget<UCanvasPanel>();
        RootSizeBox->SetContent(Root);
    }
    if (!Root)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MissionBriefingDlg] No canvas/size-box host for briefing header"));
        return;
    }

    BriefingFrame = WidgetTree->ConstructWidget<USizeBox>();
    BriefingFrame->SetWidthOverride(1920.f);
    BriefingFrame->SetHeightOverride(1080.f);
    BriefingFrame->SetClipping(EWidgetClipping::ClipToBounds);
    UBorder* FrameBorder = WidgetTree->ConstructWidget<UBorder>();
    FrameBorder->SetPadding(FMargin(0.f));
    FrameBorder->SetBrush(*FCoreStyle::Get().GetBrush("WhiteBrush"));
    FrameBorder->SetBrushColor(FLinearColor(0.10f, 0.11f, 0.12f, 1.f));
    BriefingFrame->SetContent(FrameBorder);
    UScaleBox* Fit = WidgetTree->ConstructWidget<UScaleBox>();
    Fit->SetStretch(EStretch::ScaleToFit);
    Fit->SetStretchDirection(EStretchDirection::Both);
    Fit->SetContent(BriefingFrame);

    UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
    FrameBorder->SetContent(Column);
    auto AddBar = [this, Column](const FLinearColor& Color, const FMargin& BarPadding)->UBorder*
    {
        UBorder* Bar = WidgetTree->ConstructWidget<UBorder>();
        Bar->SetBrushColor(Color);
        Bar->SetPadding(BarPadding);
        Column->AddChildToVerticalBox(Bar);
        return Bar;
    };
    UBorder* Header = AddBar(FLinearColor(0.070f, 0.112f, 0.155f, 1.f), FMargin(18.f, 12.f));
    UHorizontalBox* Titles = WidgetTree->ConstructWidget<UHorizontalBox>();
    auto AddTitle = [this, Titles](UTextBlock*& Text, bool Right)
    {
        if (!Text) Text = WidgetTree->ConstructWidget<UTextBlock>();
        Text->RemoveFromParent();
        Text->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        Text->SetFont(MissionUIStyle::GetHeaderFont(38));
        Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.97f, 1.f)));
        Text->SetJustification(Right ? ETextJustify::Right : ETextJustify::Left);
        if (UHorizontalBoxSlot* Cell = Titles->AddChildToHorizontalBox(Text))
        {
            Cell->SetSize(FSlateChildSize(Right ? ESlateSizeRule::Fill : ESlateSizeRule::Automatic));
            Cell->SetVerticalAlignment(VAlign_Center);
            if (Right) Cell->SetPadding(FMargin(24.f, 0.f, 0.f, 0.f));
        }
    };
    AddTitle(TitleText, false);
    TitleText->SetText(NSLOCTEXT("MissionBriefing", "HeaderTitle", "MISSION BRIEFING"));
    AddTitle(MissionNameText, true);
    Header->SetContent(Titles);

    auto AddStatusRow = [this, AddBar](const TArray<TPair<FString, UTextBlock**>>& Entries)
    {
        UBorder* Bar = AddBar(FLinearColor(0.045f, 0.073f, 0.100f, 1.f), FMargin(18.f, 8.f));
        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
        for (const auto& Entry : Entries)
        {
            UVerticalBox* Cell = WidgetTree->ConstructWidget<UVerticalBox>();
            UTextBlock* Caption = WidgetTree->ConstructWidget<UTextBlock>();
            Caption->SetText(FText::FromString(Entry.Key));
            Caption->SetFont(MissionUIStyle::GetInfoLabelFont(12));
            Caption->SetColorAndOpacity(FSlateColor(FLinearColor(0.48f, 0.79f, 0.88f)));
            Cell->AddChildToVerticalBox(Caption);
            UHorizontalBoxSlot* CellSlot = Row->AddChildToHorizontalBox(Cell);
            CellSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            CellSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));

            if (!Entry.Value)
                continue;

            UTextBlock*& Value = *Entry.Value;
            if (!Value) Value = WidgetTree->ConstructWidget<UTextBlock>();
            Value->RemoveFromParent();
            Value->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
            Value->SetFont(MissionUIStyle::GetInfoValueFont(16));
            Value->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.97f, 1.f)));
            Value->SetJustification(ETextJustify::Left);
            Value->SetAutoWrapText(false);
            Cell->AddChildToVerticalBox(Value)->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
        }
        Bar->SetContent(Row);
    };
    AddStatusRow({{TEXT("TIME"), &GameTimeText}, {TEXT("CAMPAIGN"), &MissionTPlusText},
        {TEXT("PILOT"), &PlayerNameText}, {TEXT("UNIT"), &CurrentUnitText}, {TEXT("SCORE"), &PlayerScoreText}});
    AddStatusRow({{TEXT("SYSTEM"), &MissionSystemText}, {TEXT("SECTOR"), &MissionSectorText},
        {TEXT("START"), &MissionTimeStartText}, {TEXT("TRT TIME"), &MissionTimeTargetText}, {TEXT(""), nullptr}});
    UBorder* Rule = AddBar(FLinearColor(0.20f, 0.21f, 0.23f, 1.f), FMargin(0.f));
    USizeBox* RuleSize = WidgetTree->ConstructWidget<USizeBox>();
    RuleSize->SetHeightOverride(5.f);
    Rule->SetContent(RuleSize);

    UHorizontalBox* Tabs = WidgetTree->ConstructWidget<UHorizontalBox>();
    Column->AddChildToVerticalBox(Tabs)->SetPadding(FMargin(0.f, 12.f));
    auto AddTab = [this, Tabs](UButton*& Button, UTextBlock*& Caption, const TCHAR* Label)
    {
        if (!Button) Button = WidgetTree->ConstructWidget<UButton>();
        Button->RemoveFromParent();
        Button->SetVisibility(ESlateVisibility::Visible);
        if (!Caption) Caption = WidgetTree->ConstructWidget<UTextBlock>();
        Caption->RemoveFromParent();
        Caption->SetText(FText::FromString(Label));
        Caption->SetFont(MissionUIStyle::GetHeaderFont(22));
        Caption->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        Caption->SetJustification(ETextJustify::Center);
        Button->SetContent(Caption);
        if (UButtonSlot* Cell = Cast<UButtonSlot>(Caption->Slot))
        {
            Cell->SetPadding(FMargin(10.f, 4.f));
            Cell->SetHorizontalAlignment(HAlign_Center);
            Cell->SetVerticalAlignment(VAlign_Center);
        }
        UHorizontalBoxSlot* Cell = Tabs->AddChildToHorizontalBox(Button);
        Cell->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        Cell->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
    };
    BriefingTabs.Empty();
    for (const TCHAR* Label : { TEXT("SITREP"), TEXT("PACKAGE"), TEXT("NAVIGATION") })
    {
        UButton* Button = nullptr;
        UTextBlock* Caption = nullptr;
        AddTab(Button, Caption, Label);
        Button->OnHovered.AddUniqueDynamic(this, &UMissionBriefingDlg::HandleBriefingTabHovered);
        BriefingTabs.Add(Button);
    }
    BriefingTabs[0]->OnClicked.AddUniqueDynamic(this, &UMissionBriefingDlg::OpenBriefingSituation);
    BriefingTabs[1]->OnClicked.AddUniqueDynamic(this, &UMissionBriefingDlg::OpenBriefingPackage);
    BriefingTabs[2]->OnClicked.AddUniqueDynamic(this, &UMissionBriefingDlg::OpenBriefingNavigation);
    auto AddArrow = [this, Tabs](UButton*& Button, const TCHAR* IconPath, const FText& Tip, bool AtStart)
    {
        if (!Button) Button = WidgetTree->ConstructWidget<UButton>();
        Button->RemoveFromParent();
        Button->SetVisibility(ESlateVisibility::Visible);
        Button->SetToolTipText(Tip);
        USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>();
        IconBox->SetWidthOverride(40.f);
        IconBox->SetHeightOverride(40.f);
        UImage* Icon = WidgetTree->ConstructWidget<UImage>();
        if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, IconPath))
            Icon->SetBrushFromTexture(Texture, true);
        if (AtStart) MissionUIStyle::SetActionIcon(Icon, IconPath, FLinearColor::White);
        Button->SetColorAndOpacity(FLinearColor::White);
        IconBox->SetContent(Icon);
        Button->SetContent(IconBox);
        if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(IconBox->Slot))
        {
            ContentSlot->SetHorizontalAlignment(HAlign_Center);
            ContentSlot->SetVerticalAlignment(VAlign_Center);
            ContentSlot->SetPadding(FMargin(4.f));
        }
        UHorizontalBoxSlot* TabSlot = Tabs->AddChildToHorizontalBox(Button);
        TabSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        TabSlot->SetPadding(AtStart ? FMargin(0.f, 0.f, 10.f, 0.f) : FMargin(0.f));
        TabSlot->SetVerticalAlignment(VAlign_Center);
        // Keep the navigation order identical to CmdDlg: back, tabs, forward.
        if (AtStart)
        {
            TArray<UWidget*> Children = Tabs->GetAllChildren();
            Tabs->ClearChildren();
            UHorizontalBoxSlot* BackSlot = Tabs->AddChildToHorizontalBox(Button);
            BackSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
            BackSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            BackSlot->SetVerticalAlignment(VAlign_Center);
            for (UWidget* Child : Children)
            {
                if (Child == Button) continue;
                UHorizontalBoxSlot* Cell = Tabs->AddChildToHorizontalBox(Child);
                Cell->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
                Cell->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
            }
        }
    };
    AddArrow(ReturnButton,
        TEXT("/Game/ProIconPack/Textures/Generic_Icons/256x256/T_Arrow_L_256x256.T_Arrow_L_256x256"),
        NSLOCTEXT("MissionBriefing", "BackToCaller", "Cancel and return to the previous screen"), true);
    AddArrow(MissionButton,
        TEXT("/Game/ProIconPack/Textures/Generic_Icons/256x256/T_Arrow_R_256x256.T_Arrow_R_256x256"),
        NSLOCTEXT("MissionBriefing", "AcceptArrow", "Accept mission"), false);
    MissionButton->OnClicked.AddUniqueDynamic(this, &UMissionBriefingDlg::HandleAcceptClicked);
    ReturnButton->OnClicked.AddUniqueDynamic(this, &UMissionBriefingDlg::HandleCancelClicked);

    MissionSwitcher->RemoveFromParent();
    UVerticalBoxSlot* Content = Column->AddChildToVerticalBox(MissionSwitcher);
    Content->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Content->SetHorizontalAlignment(HAlign_Fill);
    Content->SetVerticalAlignment(VAlign_Fill);
    // All live bindings were moved first; hide only the now obsolete Blueprint frame.
    for (UWidget* Child : Root->GetAllChildren())
        if (Child) Child->SetVisibility(ESlateVisibility::Collapsed);
    if (MenuButtonContainer) MenuButtonContainer->SetVisibility(ESlateVisibility::Collapsed);
    if (UCanvasPanelSlot* Main = Root->AddChildToCanvas(Fit))
    {
        Main->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
        Main->SetOffsets(FMargin(0.f));
        Main->SetZOrder(20);
    }
    RefreshBriefingTabStyles();
}

void UMissionBriefingDlg::RefreshBriefingTabStyles()
{
    // Same texture states as CmdDlg::RefreshOperationsTabStyles.
    auto Style = [](UButton* Button, const TCHAR* Color)
    {
        if (!Button) return;
        FButtonStyle Style = Button->GetStyle();
        auto Brush = [Color](const TCHAR* State)->FSlateBrush
        {
            const FString Path = FString::Printf(TEXT("/Game/SciFiUIComponents/Textures/Button/T_Btn01%s_%s.T_Btn01%s_%s"), Color, State, Color, State);
            FSlateBrush Result;
            if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *Path))
            {
                Result.SetResourceObject(Texture);
                Result.DrawAs = ESlateBrushDrawType::Image;
                Result.ImageSize = FVector2D(64.f, 64.f);
            }
            else
            {
                Result.DrawAs = ESlateBrushDrawType::Box;
                Result.TintColor = FSlateColor(FLinearColor(0.1f, 0.2f, 0.3f, 1.f));
            }
            return Result;
        };
        Style.Normal = Brush(TEXT("n"));
        Style.Hovered = Brush(TEXT("p"));
        Style.Pressed = Style.Hovered;
        Style.Disabled = Style.Normal;
        Button->SetStyle(Style);
    };
    for (int32 Index = 0; Index < BriefingTabs.Num(); ++Index)
        Style(BriefingTabs[Index], Index == static_cast<int32>(CurrentMode) ? TEXT("Blue") : TEXT("Cyan"));
    if (BriefingFrame)
    {
        Style(MissionButton, TEXT("Green"));
        Style(ReturnButton, TEXT("Red"));
    }
}

void UMissionBriefingDlg::HandleBriefingTabHovered()
{
    if (UGameInstance* GI = GetGameInstance())
        if (UStarshatterAudioSubsystem* Audio = GI->GetSubsystem<UStarshatterAudioSubsystem>())
            Audio->PlayHoverSound(this);
}

void UMissionBriefingDlg::ActivateBriefingTab(EMissionBriefingMode Mode)
{
    if (UGameInstance* GI = GetGameInstance())
        if (UStarshatterAudioSubsystem* Audio = GI->GetSubsystem<UStarshatterAudioSubsystem>())
            Audio->PlayAcceptSound(this);
    SetMode(Mode);
}

void UMissionBriefingDlg::OpenBriefingSituation() { ActivateBriefingTab(EMissionBriefingMode::SIT); }
void UMissionBriefingDlg::OpenBriefingPackage() { ActivateBriefingTab(EMissionBriefingMode::PKG); }
void UMissionBriefingDlg::OpenBriefingNavigation() { ActivateBriefingTab(EMissionBriefingMode::NAV); }
void UMissionBriefingDlg::OpenBriefingWeapons() { if (bLiveMissionStarted) ToggleHangar(); }

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
        NewButton->SetButtonSize(0.f, bModernBriefingPresentation ? FMath::Clamp(BriefingTabHeight, 32.f, 80.f) : 42.f);
        NewButton->SetLabelFontSizeValue(bModernBriefingPresentation ? FMath::Clamp(BriefingTabFontSize, 12, 24) : 16);
        NewButton->SetMenuOption(Item);
        NewButton->SetButtonText(FText::FromString(Item).ToUpper());

        if (UVerticalBoxSlot* VBoxSlot = MenuVBox->AddChildToVerticalBox(NewButton))
        {
            VBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, bModernBriefingPresentation ? 6.f : 0.f));
            VBoxSlot->SetHorizontalAlignment(HAlign_Fill);
            VBoxSlot->SetVerticalAlignment(VAlign_Center);
        }

        if (bModernBriefingPresentation)
        {
            FText Label = FText::FromString(Item);
            if (Item == TEXT("SIT")) Label = NSLOCTEXT("MissionBriefing", "SituationTab", "SITREP");
            else if (Item == TEXT("PKG")) Label = NSLOCTEXT("MissionBriefing", "PackageTab", "PACKAGE");
            else if (Item == TEXT("NAV")) Label = NSLOCTEXT("MissionBriefing", "NavigationTab", "NAVIGATION");
            else if (Item == TEXT("WEP")) Label = NSLOCTEXT("MissionBriefing", "WeaponsTab", "WEAPONS");

            // SetButtonText also changes MenuOption. Update only the visual label
            // here so the legacy SIT/PKG/NAV/WEP routing and selection still work.
            if (NewButton->Label) NewButton->Label->SetText(Label);
            NewButton->SetToolTipText(Label);
            NewButton->SelectedColor = FLinearColor(BriefingAccentColor.R * 0.25f,
                BriefingAccentColor.G * 0.25f, BriefingAccentColor.B * 0.25f, 1.f);
            NewButton->HoveredColor = FLinearColor(0.12f, 0.19f, 0.26f, 1.f);
            NewButton->SetSelected(false);
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

void UMissionBriefingDlg::SetStandaloneMission(Mission* InMission, Campaign* InRepository)
{
    bStandaloneMission = (InMission != nullptr);
    StandaloneRepository = bStandaloneMission ? InRepository : nullptr;
    MissionPtr = InMission;
    CampaignPtr = StandaloneRepository;
    InfoPtr = nullptr;
}

void UMissionBriefingDlg::ClearStandaloneMission()
{
    bStandaloneMission = false;
    StandaloneRepository = nullptr;
    MissionPtr = nullptr;
    CampaignPtr = nullptr;
    InfoPtr = nullptr;
}

void UMissionBriefingDlg::ShowMsnDlg()
{
    CloseEmptySectorPreview();
    // A briefing can open after another screen owned the previous presentation.
    // Clear its world actors even when this dialog never started that mission.
    if (UGameInstance* GI = GetGameInstance())
        if (auto* Runtime = GI->GetSubsystem<USSWRuntimeSubsystem>())
            Runtime->EndMissionPresentation();
    PackageIndex = -1;
    if (bStandaloneMission)
    {
        // The caller owns the legacy Mission; no story campaign selection or
        // story campaign mission state is changed here.
        CampaignPtr = StandaloneRepository;
        UE_LOG(LogTemp, Log, TEXT("[MissionBriefingDlg] Standalone Mission=%p Repository=%p"),
            MissionPtr, CampaignPtr);
    }
    else
    {
        // Preserve the original campaign-driven briefing behavior.
        CampaignPtr = Campaign::GetCampaign();
        MissionPtr = nullptr;
        const int32 MissionId = CampaignPtr ? CampaignPtr->GetMissionId() : -1;
        if (CampaignPtr && MissionId > 0)
            MissionPtr = CampaignPtr->GetMission(MissionId);
        UE_LOG(LogTemp, Warning,
            TEXT("[MissionBriefingDlg] Campaign=%p MissionId=%d"), CampaignPtr, MissionId);
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

        if (!bIsScripted && !bStandaloneMission)
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
    for (UButton* Tab : BriefingTabs)
        if (Tab) Tab->SetIsEnabled(bHasMission);

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

    if (MissionSituationPanel)
    {
        MissionSituationPanel->SetParentDlg(this);
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[MissionBriefingDlg] MissionSituationPanel widget is null"));
    }

    SetMode(EMissionBriefingMode::SIT);
}

void UMissionBriefingDlg::RefreshHeader()
{
    const FText Unavailable = FText::FromString(TEXT("â€”"));
    const bool bHasCampaign = CampaignPtr && (!bStandaloneMission || CampaignPtr->IsTraining());

    if (MissionTPlusText)
        MissionTPlusText->SetText(bHasCampaign ? ToTextFromUtf8(CampaignPtr->GetName()) : Unavailable);
    if (PlayerScoreText)
        PlayerScoreText->SetText(bHasCampaign ? FText::AsNumber(CampaignPtr->GetPlayerTeamScore()) : Unavailable);
    if (CurrentUnitText)
    {
        CombatGroup* Group = bHasCampaign ? CampaignPtr->GetPlayerGroup() : nullptr;
        CurrentUnitText->SetText(Group ? ToTextFromUtf8(Group->GetDescription())
            : NSLOCTEXT("MissionBriefing", "UnassignedUnit", "Unassigned"));
    }
    if (PlayerNameText)
        PlayerNameText->SetText(Unavailable);
    if (GameTimeText)
    {
        GameTimeText->SetText(Unavailable);
        HandleUniverseSecondTick(0);
    }
    if (PlayerNameText)
        if (auto* GI = GetGameInstance())
            if (const auto* Players = GI->GetSubsystem<UStarshatterPlayerSubsystem>())
                PlayerNameText->SetText(FText::FromString(Players->GetPlayerInfo().GetRankedDisplayName()));
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

    if (MissionTimeTargetLabelText)
        MissionTimeTargetLabelText->SetText(bShowTimeOnTarget
            ? NSLOCTEXT("MissionBriefing", "TRTTimeLabel", "TRT TIME") : FText::GetEmpty());

    if (MissionTimeTargetText)
    {
        if (bShowTimeOnTarget)
        {
            const int32 TimeOnTarget = CalcTimeOnTarget();
            if (TimeOnTarget > 0)
            {
                char Buf[32] = { 0 };
                FormatDayTime(Buf, TimeOnTarget);
                MissionTimeTargetText->SetText(ToTextFromUtf8(Buf));
            }
            else
            {
                MissionTimeTargetText->SetText(NSLOCTEXT("MissionBriefing", "UnknownTRTTime", "UNKNOWN"));
            }
        }
        else
        {
            MissionTimeTargetText->SetText(FText::GetEmpty());
        }
    }
}

void UMissionBriefingDlg::SetMode(EMissionBriefingMode NewMode)
{
    CurrentMode = NewMode;
    if (MissionSituationPanel) MissionSituationPanel->SetShipPreviewActive(NewMode == EMissionBriefingMode::SIT);

    if (MissionSwitcher)
    {
        UWidget* Page = nullptr;
        switch (CurrentMode)
        {
        case EMissionBriefingMode::SIT: Page = MissionSituationPanel; break;
        case EMissionBriefingMode::PKG: Page = MissionPackagePanel; break;
        case EMissionBriefingMode::NAV: Page = MissionNavPanel; break;
        case EMissionBriefingMode::WEP: Page = MissionWepPanel; break;
        }
        if (Page && Page->GetParent() == MissionSwitcher)
            MissionSwitcher->SetActiveWidget(Page);
        else
            MissionSwitcher->SetActiveWidgetIndex(static_cast<int32>(CurrentMode));
    }

    RefreshMenuSelection();
    RefreshBriefingTabStyles();

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
    else if (Option == TEXT("WEP") && bLiveMissionStarted)
    {
        ToggleHangar();
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

void UMissionBriefingDlg::HandleSimulationUnloading()
{
    // This path is also used by debrief, replacement missions and shutdown.
    CloseEmptySectorPreview(true);
}

void UMissionBriefingDlg::CloseEmptySectorPreview(bool bPreserveSimulation)
{
    if (bClosingMissionPreview) return;
    TGuardValue<bool> ClosingGuard(bClosingMissionPreview, true);
    // Close callbacks/input before their Ship* and MissionEvent* sources die.
    ClearTravelPresentation();
    DisableMissionMenuInput();
    if (MissionCameraRig) MissionCameraRig->Reset(GetOwningPlayer());
    MissionPlayerCameraActor.Reset();
    MissionPlayerCameraDistance = 0;
    bMissionTargetInspection = false;
    if (bLiveMissionStarted && GetGameInstance())
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
    bLiveMissionStarted = false;
    MissionTargetOverlay.Reset();
    SelectedMissionTarget.Reset();
    SelectedMissionTargetName = FText::GetEmpty();
    SelectedMissionTargetLocalBounds = FBox(ForceInit);
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

    // Some authored system maps contain an unconfigured builder. Resolve the
    // selected mission explicitly before looking for its star or Sim regions.
    if (MissionPtr)
    {
        const FString SystemName=ToTextFromUtf8(MissionPtr->GetSystem()).ToString().TrimStartAndEnd();
        for (TActorIterator<ASystemSceneBuilder> It(World); It; ++It)
        {
            if (It->GetLevel()!=MissionSystemLevel->GetLoadedLevel())continue;
            if (!It->GetLastResolvedSystemName().Equals(SystemName,ESearchCase::IgnoreCase) ||
                It->GetSpawnedBodies().IsEmpty())
            {
                It->bBuildOnBeginPlay=false;
                It->DataSource=ESystemSceneSource::StarSystem;
                It->TargetSystemName=SystemName;
                It->TargetGalaxyName.Empty();
                It->BuildSystemScene();
            }
            break;
        }
    }

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
        UE_LOG(LogTemp, Error, TEXT("[MissionBriefing] Live mission startup failed; returning to main menu"));
        if (auto* Runtime = GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>())
        {
            Runtime->SetPaused(true);
            Runtime->EndMissionPresentation();
        }
        CloseEmptySectorPreview();
        if (manager) manager->ShowMenuDlg();
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 12.f, FColor::Red,
            TEXT("Unable to start the live scene. Your pilot save has been kept. See the startup log."));
        return;
    }
    const double Radius = Bounds.BoxExtent.GetMax();
    int32 Width = 0, Height = 0;
    PC->GetViewportSize(Width, Height);
    if (Radius <= KINDA_SMALL_NUMBER || Width <= 0 || Height <= 0) return;

    if (!IsValid(MissionPreviewCamera))
    {
        MissionPreviewCamera = World->SpawnActor<AMissionViewCamera>();
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
    // Position the flight camera before the cover's existing warmup/reveal checks.
    bMissionTargetInspection = false;
    MissionPlayerCameraActor.Reset();
    MissionPlayerCameraSearchTime = 0;
    MissionPlayerCameraDistance = 0;
    MissionTargetZoomDistance = 1.0;
    MissionTargetLastUpdate = World->GetTimeSeconds();
    UpdateMissionTargetCamera();
    FTimerManagerTimerParameters FlightCameraTimerParameters;
    FlightCameraTimerParameters.bLoop = true;
    FlightCameraTimerParameters.bMaxOncePerFrame = true;
    World->GetTimerManager().SetTimer(MissionTargetCameraTimer, this,
        &UMissionBriefingDlg::UpdateMissionTargetCamera, 0.000001f, FlightCameraTimerParameters);
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
    if (bLiveMissionStarted && !MissionPlayerCameraActor.IsValid())
        UpdateMissionTargetCamera();

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
    if (bLiveMissionStarted && !MissionPlayerCameraActor.IsValid())
    {
        // A missing presentation actor must not leave the loading cover up forever.
        if (Now - MissionCameraStableSince < 5.0) return;
        UE_LOG(LogTemp, Warning, TEXT("[MissionCamera] Revealing mission before player visual is ready"));
    }
    if (bLiveMissionStarted)
    {
        if (auto* Timer = GetGameInstance()->GetSubsystem<UTimerSubsystem>()) Timer->StartMissionRun(true);
        if (auto* Runtime = GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>()) Runtime->SetPaused(false);
    }
    for (const TWeakObjectPtr<AActor>& Actor : MissionSunHiddenActors)
        if (Actor.IsValid()) Actor->SetActorHiddenInGame(false);
    MissionSunHiddenActors.Empty();
    if (MissionSceneCover.IsValid())
    {
        if (UGameViewportClient* Viewport = World->GetGameViewport())
            Viewport->RemoveViewportWidgetContent(MissionSceneCover.ToSharedRef());
        MissionSceneCover.Reset();
    }
    World->GetTimerManager().ClearTimer(MissionSunRevealTimer);

}

namespace
{
class SFlightPanelWindow : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SFlightPanelWindow) {}
        SLATE_ARGUMENT(FText, Title)
        SLATE_EVENT(FSimpleDelegate, OnClose)
        SLATE_DEFAULT_SLOT(FArguments, Content)
    SLATE_END_ARGS()

    void Construct(const FArguments& Args)
    {
        ChildSlot
        [
            SNew(SBorder).BorderImage(&FlightPanelStyle::Window()).Padding(6)
            [SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()
            [SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1)
            [
                SAssignNew(TitleBar, SBorder).BorderImage(&FlightPanelStyle::TitleBar())
                .Padding(FMargin(12, 6)).Cursor(EMouseCursor::CardinalCross)
                .OnMouseButtonDown_Lambda([this](const FGeometry&, const FPointerEvent& Event)
                {
                    return Event.GetEffectingButton() == EKeys::LeftMouseButton
                        ? FReply::Handled().CaptureMouse(TitleBar.ToSharedRef()) : FReply::Unhandled();
                })
                .OnMouseMove_Lambda([this](const FGeometry&, const FPointerEvent& Event)
                {
                    if (!TitleBar->HasMouseCapture() || !Move) return FReply::Unhandled();
                    Move(Event);
                    return FReply::Handled();
                })
                .OnMouseButtonUp_Lambda([this](const FGeometry&, const FPointerEvent& Event)
                {
                    return Event.GetEffectingButton() == EKeys::LeftMouseButton && TitleBar->HasMouseCapture()
                        ? FReply::Handled().ReleaseMouseCapture() : FReply::Unhandled();
                })
                [SNew(STextBlock).Text(Args._Title).Font(MissionUIStyle::GetInfoLabelFont(15))]
            ]
            + SHorizontalBox::Slot().AutoWidth().Padding(4, 0, 0, 0)
            [
                SNew(SBox).WidthOverride(36).HeightOverride(36)
                [SNew(SButton).ButtonStyle(&Buttons.Red).ContentPadding(0)
                    .HAlign(HAlign_Center).VAlign(VAlign_Center)
                    .IsFocusable(false).ToolTipText(INVTEXT("Close panel"))
                    .OnClicked_Lambda([Close = Args._OnClose]() { Close.ExecuteIfBound(); return FReply::Handled(); })
                    [SNew(SPanelCloseIcon)]]
            ]]
            + SVerticalBox::Slot().FillHeight(1).Padding(0, 6, 0, 0)
            [SNew(SBox).Clipping(EWidgetClipping::ClipToBounds)[Args._Content.Widget]]]
        ];
    }

    FReply OnPreviewMouseButtonDown(const FGeometry&, const FPointerEvent&) override
    {
        if (Raise) Raise();
        return FReply::Unhandled();
    }

    FReply OnMouseWheel(const FGeometry&, const FPointerEvent&) override
    {
        // Children get first chance to scroll or zoom. Keep any unused wheel input off the flight camera.
        return FReply::Handled();
    }

    TFunction<void()> Raise;
    TFunction<void(const FPointerEvent&)> Move;

private:
    TSharedPtr<SBorder> TitleBar;
    FInMissionButtonStyle Buttons;
};
}

bool UMissionBriefingDlg::CanOpenFlightPanel() const
{
    return bLiveMissionStarted && !bMissionCompleteHandedOff && GetWorld() && !GetWorld()->IsPaused()
        && ((bOpeningStationService && FStationServices::IsDocked()) || (MissionPtr && MissionPtr->IsFreeFlight()) || !(Sim::GetSim() && Sim::GetSim()->HasPlayerDocked()))
        && !TravelTransition && !MissionSceneCover
        && !(MissionQuitMenu && MissionQuitMenu->IsMenuShown());
}

bool UMissionBriefingDlg::CanOpenCameraAndRadio() const
{
    if (!CanOpenFlightPanel()) return false;

    Sim* Simulation = Sim::GetSim();
    const Ship* Pilot = Simulation ? Simulation->GetPlayerShip() : nullptr;
    return Pilot && !(Pilot->GetDock() && Pilot->GetDock()->IsLaunchDeck()
        && Pilot->GetFlightPhase() < EOPSMode::ACTIVE);
}

void UMissionBriefingDlg::InstallFlightPanelToolbar()
{
    if (FlightPanelToolbar || !GetWorld() || !GetWorld()->GetGameViewport()) return;
    FlightPanelCanvas = SNew(SConstraintCanvas).Visibility(EVisibility::SelfHitTestInvisible)
        .ForceVolatile(true);
    FlightPanelZ = 0;
    auto Toolbar = SNew(SVerticalBox);
    using FToggle = void (UMissionBriefingDlg::*)();
    const auto QuantumPilot = []() -> const Ship*
    {
        Sim* Simulation = Sim::GetSim();
        return Simulation ? Simulation->GetPlayerShip() : nullptr;
    };
    auto AddButton = [this, &Toolbar, QuantumPilot](const TCHAR* Label, EShipSystemIcon Icon, FToggle Toggle,
        const TSharedPtr<SWidget> UMissionBriefingDlg::* Panel, bool bRequiresLaunch = false)
    {
        const bool bQuantum = Toggle == &UMissionBriefingDlg::ToggleQuantumTravel;
        Toolbar->AddSlot().AutoHeight().Padding(0, 0, 0, 5)
        [
            SNew(SBox).WidthOverride(38).HeightOverride(38)
            .ToolTipText_Lambda([this, Label, Toggle, bRequiresLaunch, bQuantum, QuantumPilot]()
            {
                if (Toggle == &UMissionBriefingDlg::ToggleMarketplace && !CanOpenMarketplace()) return INVTEXT("Marketplace (available when docked)");
                if (Toggle == &UMissionBriefingDlg::ToggleStationServices)
                {
                    if (!CanOpenStationServices()) return INVTEXT("Station Services (select a station or carrier to inspect)");
                    return CanUseStationServices() ? INVTEXT("Station Services") : INVTEXT("Station Services (read-only until docked at the viewed facility)");
                }
                if (bQuantum)
                {
                    const Ship* Pilot = QuantumPilot();
                    if (Pilot && Pilot->GetQuantumDrive()) return INVTEXT("Quantum jump drive");
                    return Pilot && Pilot->IsDropship()
                        ? INVTEXT("Fighter not equipped") : INVTEXT("Ship not equipped");
                }
                return bRequiresLaunch && !CanOpenCameraAndRadio()
                    ? FText::Format(INVTEXT("{0} (available after launch)"), FText::FromString(Label))
                    : FText::FromString(Label);
            })
            [
                SNew(SButton).IsFocusable(false).ContentPadding(0).HAlign(HAlign_Center).VAlign(VAlign_Center)
                .IsEnabled_Lambda([this, Toggle, bRequiresLaunch, bQuantum, QuantumPilot]()
                {
                    if (bQuantum)
                    {
                        const Ship* Pilot = QuantumPilot();
                        return Pilot && Pilot->GetQuantumDrive();
                    }
                    if (Toggle == &UMissionBriefingDlg::ToggleMarketplace) return CanOpenMarketplace();
                    if (Toggle == &UMissionBriefingDlg::ToggleStationServices) return CanOpenStationServices();
                    return CanOpenFlightPanel() && Toggle && (!bRequiresLaunch || CanOpenCameraAndRadio());
                })
                .ButtonColorAndOpacity_Lambda([this, Panel, Label]()
                {
                    if(Panel && (this->*Panel).IsValid()){PanelAttention.Acknowledge(FName(Label));return FLinearColor(.25f,.75f,.95f);}
                    if(PanelAttention.IsUnread(FName(Label)))
                    {const float Glow=.75f+.15f*FMath::Sin(float(FPlatformTime::Seconds()*3));return FLinearColor(.1f,.8f*Glow,Glow);}
                    return FLinearColor(.25f,.38f,.48f);
                })
                .OnClicked_Lambda([this, Toggle, Panel, Label]()
                {
                    if (Toggle) (this->*Toggle)();
                    if(Panel && (this->*Panel).IsValid())PanelAttention.Acknowledge(FName(Label));
                    return FReply::Handled();
                })
                [SNew(SOverlay)
                    +SOverlay::Slot()[SNew(SShipSystemIcon).Icon(Icon)]
                    +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(2)
                    [SNew(SBox).WidthOverride(5).HeightOverride(5)
                        .Visibility_Lambda([this,Label,Panel](){return PanelAttention.IsUnread(FName(Label)) && !(Panel && (this->*Panel).IsValid())?EVisibility::HitTestInvisible:EVisibility::Collapsed;})
                        [SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush")).ColorAndOpacity(FLinearColor(.2f,.9f,1))]]
                ]
            ]
        ];
    };
    AddButton(TEXT("Agents"), EShipSystemIcon::Agent, &UMissionBriefingDlg::ToggleAgents, &UMissionBriefingDlg::AgentsPopup);
    AddButton(TEXT("Camera"), EShipSystemIcon::Camera, &UMissionBriefingDlg::ToggleFlightCameraPanel, nullptr, true);
    AddButton(TEXT("Cargo Manifest"), EShipSystemIcon::Cargo, &UMissionBriefingDlg::ToggleCargoManifest, &UMissionBriefingDlg::CargoManifestPopup);
    AddButton(TEXT("Contacts"), EShipSystemIcon::Sensor, &UMissionBriefingDlg::ToggleContactsPanel, &UMissionBriefingDlg::ContactsPopup);
    AddButton(TEXT("Contracts"), EShipSystemIcon::Contracts, &UMissionBriefingDlg::ToggleContracts, &UMissionBriefingDlg::ContractsPopup);
    AddButton(TEXT("Engineering"), EShipSystemIcon::Reactor, &UMissionBriefingDlg::ToggleEngineering, &UMissionBriefingDlg::EngineeringPopup);
    AddButton(TEXT("Flight Ops"), EShipSystemIcon::Deck, &UMissionBriefingDlg::ToggleFlightOpsPopup, &UMissionBriefingDlg::FlightOpsPopup);
    AddButton(TEXT("Intel"), EShipSystemIcon::Intel, &UMissionBriefingDlg::ToggleIntelPopup, &UMissionBriefingDlg::IntelPopup);
    AddButton(TEXT("Marketplace"), EShipSystemIcon::Marketplace, &UMissionBriefingDlg::ToggleMarketplace, &UMissionBriefingDlg::MarketplacePopup);
    AddButton(TEXT("Mining Drones"), EShipSystemIcon::MiningDrone, &UMissionBriefingDlg::ToggleMiningDrones, &UMissionBriefingDlg::MiningDronesPopup);
    AddButton(TEXT("Mission"), EShipSystemIcon::Reference, &UMissionBriefingDlg::ToggleMissionPopup, &UMissionBriefingDlg::MissionPopup);
    AddButton(TEXT("Navigation"), EShipSystemIcon::Navigation, &UMissionBriefingDlg::ToggleNavigationPopup, &UMissionBriefingDlg::NavigationPopup);
    AddButton(TEXT("Objectives"), EShipSystemIcon::Computer, &UMissionBriefingDlg::ToggleObjectivesPopup, &UMissionBriefingDlg::ObjectivesPopup);
    AddButton(TEXT("Options"), EShipSystemIcon::Options, &UMissionBriefingDlg::ToggleLiveOptions, &UMissionBriefingDlg::LiveOptionsPopup);
    AddButton(TEXT("Player Logbook"), EShipSystemIcon::Gazetteer, &UMissionBriefingDlg::TogglePlayers, &UMissionBriefingDlg::PlayersPopup);
    AddButton(TEXT("Production Facilities"), EShipSystemIcon::Production, &UMissionBriefingDlg::ToggleProduction, &UMissionBriefingDlg::ProductionPopup);
    AddButton(TEXT("Quantum"), EShipSystemIcon::Jump, &UMissionBriefingDlg::ToggleQuantumTravel, &UMissionBriefingDlg::QuantumPopup);
    AddButton(TEXT("Radio"), EShipSystemIcon::Radio, &UMissionBriefingDlg::ToggleFighterRadio, nullptr, true);
    AddButton(TEXT("Ship Store"), EShipSystemIcon::ShipStore, &UMissionBriefingDlg::ToggleShipStore, &UMissionBriefingDlg::ShipStorePopup);
    AddButton(TEXT("Station Services"), EShipSystemIcon::Station, &UMissionBriefingDlg::ToggleStationServices, &UMissionBriefingDlg::StationServicesPopup);
    AddButton(TEXT("Tactical Reference"), EShipSystemIcon::Reference, &UMissionBriefingDlg::ToggleFlightReference, &UMissionBriefingDlg::ReferencePopup);
    AddButton(TEXT("Weapons"), EShipSystemIcon::Weapon, &UMissionBriefingDlg::ToggleFighterWeaponsPanel, &UMissionBriefingDlg::WeaponsPopup);
    AddButton(TEXT("Mission exit menu"), EShipSystemIcon::Exit, &UMissionBriefingDlg::ToggleMissionMenu, nullptr);
    const auto AddRadioWindow = [this](bool bCamera, const FText& Caption)
    {
        const auto IsOpen = [this, bCamera]()
        {
            return FighterHUDDetails && CanShowFighterHUD() && bFighterHUDVisible &&
                (bCamera ? FighterHUDDetails->IsCameraPanelOpen()
                         : FighterHUDDetails->IsRadioOpen() && !FighterHUDDetails->IsCameraPanelOpen()
                             && !FighterHUDDetails->IsTravelSpeedOpen());
        };
        auto Window = SNew(SFlightPanelWindow)
            .Title(Caption)
            .Visibility_Lambda([IsOpen]() { return IsOpen() ? EVisibility::Visible : EVisibility::Collapsed; })
            .OnClose(FSimpleDelegate::CreateWeakLambda(this, [this]()
            {
                FighterHUDDetails->CloseRadio();
                UpdateFighterRadioInput();
            }))
            [
                SNew(SBox).WidthOverride(260)
                [
                    SNew(STextBlock).Font(MissionUIStyle::GetInfoLabelFont(14))
                    .ColorAndOpacity(FLinearColor(.78f, .89f, .96f)).AutoWrapText(true)
                    .Margin(FMargin(8, 4, 8, 8))
                    .Text_Lambda([this, IsOpen]()
                    {
                        if (!IsOpen()) return FText::GetEmpty();
                        TArray<FString> Labels;
                        TArray<int32> Commands;
                        TArray<bool> Enabled;
                        FString Title;
                        FighterHUDDetails->BuildRadioRows(Labels, Commands, Enabled, Title);
                        FString Body;
                        for (int32 Index = 0; Index < Labels.Num(); ++Index)
                            Body += FString::Printf(TEXT("%d. %s%s\n"), Index + 1, *Labels[Index],
                                Enabled[Index] ? TEXT("") : TEXT(" (unavailable)"));
                        Body += TEXT("\n0  Back / Close");
                        return FText::FromString(Body);
                    })
                ]
            ];
        auto Position = MakeShared<TOptional<FVector2D>>();
        SConstraintCanvas::FSlot* Slot = nullptr;
        {
            auto NewSlot = FlightPanelCanvas->AddSlot();
            NewSlot.Expose(Slot).AutoSize(true).Alignment(FVector2D::ZeroVector).ZOrder(++FlightPanelZ)
                .Offset(TAttribute<FMargin>::CreateLambda([this, Position, WeakWindow = TWeakPtr<SFlightPanelWindow>(Window)]()
                {
                    const FVector2D View = FlightPanelCanvas->GetCachedGeometry().GetLocalSize();
                    const auto Panel = WeakWindow.Pin();
                    const FVector2D Extent = Panel ? Panel->GetDesiredSize() : FVector2D::ZeroVector;
                    const FVector2D Room = View - Extent;
                    const FVector2D Desired = Position->Get(FVector2D(
                        Room.X - FlightPanelStyle::RightPanelInset, FlightPanelStyle::RightPanelTop));
                    return FMargin(FMath::Clamp(Desired.X, 0.0, FMath::Max(0.0, Room.X)),
                        FMath::Clamp(Desired.Y, 0.0, FMath::Max(0.0, Room.Y)), 0, 0);
                }))[Window];
        }
        Window->Raise = [this, Slot]() { Slot->SetZOrder(++FlightPanelZ); };
        Window->Move = [this, Slot, Position](const FPointerEvent& Event)
        {
            const FGeometry& Geometry = FlightPanelCanvas->GetCachedGeometry();
            const FMargin Offset = Slot->GetOffset();
            *Position = FVector2D(Offset.Left, Offset.Top) + Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition())
                - Geometry.AbsoluteToLocal(Event.GetLastScreenSpacePosition());
        };
    };
    AddRadioWindow(true, INVTEXT("Camera"));
    AddRadioWindow(false, INVTEXT("Radio"));
    FlightPanelToolbar = SNew(SOverlay).Visibility(EVisibility::SelfHitTestInvisible)
        + SOverlay::Slot()[FlightPanelCanvas.ToSharedRef()]
        + SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(10, 0)
        [
            SNew(SBox).Visibility_Lambda([this]() { return (CanOpenFlightPanel() || CanOpenMarketplace()) ? EVisibility::Visible : EVisibility::Collapsed; })
            [Toolbar]
        ];
    GetWorld()->GetGameViewport()->AddViewportWidgetContent(FlightPanelToolbar.ToSharedRef(), 1100);
}

void UMissionBriefingDlg::AddFloatingPanel(TSharedPtr<SWidget>& Panel, TSharedRef<SWidget> Content,
    const TCHAR* Caption, int32 Index, FSimpleDelegate Close, bool bWide)
{
    constexpr double Width = 1140;
    constexpr double Height = 850;
    auto Window = SNew(SFlightPanelWindow).Title(FText::FromString(Caption)).OnClose(Close)[Content];
    if (bWide)
        Panel = SNew(SBox).Clipping(EWidgetClipping::ClipToBounds)[Window];
    else
        Panel = SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
            [SNew(SBox).WidthOverride(Width).HeightOverride(Height).Clipping(EWidgetClipping::ClipToBounds)[Window]];
    auto Position = MakeShared<FVector2D>(65 + 28 * Index, 90 + 24 * Index);
    SConstraintCanvas::FSlot* WindowSlot = nullptr;
    {
        auto NewSlot = FlightPanelCanvas->AddSlot();
        NewSlot.Expose(WindowSlot).Alignment(FVector2D::ZeroVector).ZOrder(++FlightPanelZ).Offset(
            TAttribute<FMargin>::CreateLambda([this, Position, bWide]()
            {
                const FVector2D View = FlightPanelCanvas->GetCachedGeometry().GetLocalSize();
                const double Scale = FMath::Clamp(FMath::Min((View.X - 70) / Width, View.Y * .70 / Height), 0.0, 1.0);
                const FVector2D Size = bWide
                    ? FVector2D(FMath::Clamp(View.X - 90.0, 0.0, 1760.0), FMath::Min(Height, View.Y * .70))
                    : FVector2D(Width, Height) * Scale;
                return FMargin(FMath::Clamp(Position->X, 0.0, View.X - Size.X),
                    FMath::Clamp(Position->Y, 0.0, View.Y - Size.Y), Size.X, Size.Y);
            }))[Panel.ToSharedRef()];
    }
    Window->Raise = [this, WindowSlot]() { WindowSlot->SetZOrder(++FlightPanelZ); };
    Window->Move = [this, WindowSlot, Position](const FPointerEvent& Event)
    {
        const FGeometry& Geometry = FlightPanelCanvas->GetCachedGeometry();
        const FVector2D Delta = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition())
            - Geometry.AbsoluteToLocal(Event.GetLastScreenSpacePosition());
        const FMargin Bounds = WindowSlot->GetOffset();
        *Position = FVector2D(Bounds.Left, Bounds.Top) + Delta;
    };
    UpdateFlightPanelInput(Content);
}

void UMissionBriefingDlg::RemoveFloatingPanel(TSharedPtr<SWidget>& Panel)
{
    if (!Panel) return;
    if (FlightPanelCanvas) FlightPanelCanvas->RemoveSlot(Panel.ToSharedRef());
    Panel.Reset();
    UpdateFlightPanelInput();
}

void UMissionBriefingDlg::UpdateFlightPanelInput(TSharedPtr<SWidget> Focus)
{
    APlayerController* PC = GetOwningPlayer();
    if (!PC) return;
    const bool bHasPanels = EngineeringPopup || WeaponsPopup || NavigationPopup || QuantumPopup ||
        IntelPopup || MissionPopup || ObjectivesPopup || FlightOpsPopup || ContactsPopup || ReferencePopup || LiveOptionsPopup || CargoManifestPopup || MarketplacePopup || ProductionPopup || StationServicesPopup || ShipStorePopup || AgentsPopup || PlayersPopup || AwardsPopup || WalletPopup || GuildsPopup || MiningDronesPopup || SkillsPopup || HangarPopup || InsurancePopup || LoadoutsPopup || FittingPopup || ContractsPopup;
    if (!bHasPanels && !bFlightPanelInput) return;

    if (!bFlightPanelInput) bFlightPanelPreviousCursor = PC->bShowMouseCursor;
    bFlightPanelInput = bHasPanels;
    if (!Focus && GetWorld() && GetWorld()->GetGameViewport())
        Focus = GetWorld()->GetGameViewport()->GetGameViewportWidget();

    FInputModeGameAndUI Mode;
    if (Focus) Mode.SetWidgetToFocus(Focus.ToSharedRef());
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    Mode.SetHideCursorDuringCapture(false);
    PC->SetInputMode(Mode);
    PC->bShowMouseCursor = bHasPanels || bFlightPanelPreviousCursor;
}

bool UMissionBriefingDlg::EnableMissionMenuInput()
{
    if (Sim* Simulation = Sim::GetSim()) {
        Simulation->OnMissionUnloading.RemoveAll(this);
        Simulation->OnMissionUnloading.AddUObject(this, &UMissionBriefingDlg::HandleSimulationUnloading);
    }
    APlayerController* PC = GetOwningPlayer();
    ULocalPlayer* LP = PC ? PC->GetLocalPlayer() : nullptr;
    auto* Input = LP ? LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
    if (!Input || !manager) return false;
    MissionMenuContext = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Input/IMC_Game.IMC_Game"));
    MissionMenuContext=StarshatterInputBindings::RuntimeContext(MissionMenuContext.Get(),this);
    if (!MissionMenuContext) return false;

    // BaseInput.ini binds F1-F5 to viewmode commands outside Enhanced Input.
    // Remove only those display-mode commands from this player's live bindings.
    // This is idempotent and also covers shifted variants of the same keys.
    if (UPlayerInput* PlayerInput = PC->PlayerInput)
    {
        const int32 Removed = PlayerInput->DebugExecBindings.RemoveAll([](const FKeyBind& Binding)
        {
            const bool bCameraKey = Binding.Key == EKeys::F1 || Binding.Key == EKeys::F2 ||
                Binding.Key == EKeys::F3 || Binding.Key == EKeys::F4 || Binding.Key == EKeys::F5;
            return bCameraKey && Binding.Command.TrimStart().StartsWith(TEXT("viewmode "), ESearchCase::IgnoreCase);
        });
        UE_LOG(LogTemp, Log, TEXT("[MissionCamera] Removed %d UE F1-F5 viewmode debug bindings."), Removed);
    }

    MissionMenuInput = NewObject<UEnhancedInputComponent>(PC);
    MissionMenuInput->RegisterComponent();
    MissionMenuInput->Priority = 1000;

    MissionStrafeAction=StarshatterInputBindings::FindAction(MissionMenuContext.Get(), TEXT("SSW_Strafe"));
    if (MissionStrafeAction && MissionStrafeAction->ValueType==EInputActionValueType::Axis1D)
    {
        MissionMenuInput->BindAction(MissionStrafeAction.Get(),ETriggerEvent::Triggered,this,&UMissionBriefingDlg::OnMissionStrafe);
        MissionMenuInput->BindAction(MissionStrafeAction.Get(),ETriggerEvent::Completed,this,&UMissionBriefingDlg::ReleaseMissionStrafe);
        MissionMenuInput->BindAction(MissionStrafeAction.Get(),ETriggerEvent::Canceled,this,&UMissionBriefingDlg::ReleaseMissionStrafe);
    }

    MissionForwardThrustAction=StarshatterInputBindings::FindAction(MissionMenuContext.Get(), TEXT("SSW_ForwardThrust"));
    if (MissionForwardThrustAction && MissionForwardThrustAction->ValueType==EInputActionValueType::Axis1D)
    {
        MissionMenuInput->BindAction(MissionForwardThrustAction.Get(),ETriggerEvent::Triggered,this,&UMissionBriefingDlg::OnMissionForwardThrust);
        MissionMenuInput->BindAction(MissionForwardThrustAction.Get(),ETriggerEvent::Completed,this,&UMissionBriefingDlg::ReleaseMissionForwardThrust);
        MissionMenuInput->BindAction(MissionForwardThrustAction.Get(),ETriggerEvent::Canceled,this,&UMissionBriefingDlg::ReleaseMissionForwardThrust);
    }

    MissionVerticalThrustAction=StarshatterInputBindings::FindAction(MissionMenuContext.Get(), TEXT("SSW_VerticalThrust"));
    if (MissionVerticalThrustAction && MissionVerticalThrustAction->ValueType==EInputActionValueType::Axis1D)
    {
        MissionMenuInput->BindAction(MissionVerticalThrustAction.Get(),ETriggerEvent::Triggered,this,&UMissionBriefingDlg::OnMissionVerticalThrust);
        MissionMenuInput->BindAction(MissionVerticalThrustAction.Get(),ETriggerEvent::Completed,this,&UMissionBriefingDlg::ReleaseMissionVerticalThrust);
        MissionMenuInput->BindAction(MissionVerticalThrustAction.Get(),ETriggerEvent::Canceled,this,&UMissionBriefingDlg::ReleaseMissionVerticalThrust);
    }

    MissionAugmenterAction=LoadObject<UInputAction>(nullptr,TEXT("/Game/Input/IA_Augmenter.IA_Augmenter"));
    if (MissionAugmenterAction && MissionAugmenterAction->ValueType==EInputActionValueType::Boolean)
    {
        MissionMenuInput->BindAction(MissionAugmenterAction.Get(),ETriggerEvent::Triggered,this,&UMissionBriefingDlg::OnMissionAugmenter);
        MissionMenuInput->BindAction(MissionAugmenterAction.Get(),ETriggerEvent::Completed,this,&UMissionBriefingDlg::ReleaseMissionAugmenter);
        MissionMenuInput->BindAction(MissionAugmenterAction.Get(),ETriggerEvent::Canceled,this,&UMissionBriefingDlg::ReleaseMissionAugmenter);
    }

    MissionShieldsUpAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_ShieldsUp.IA_ShieldsUp"));
    if (MissionShieldsUpAction && MissionShieldsUpAction->ValueType == EInputActionValueType::Boolean)
        MissionMenuInput->BindAction(MissionShieldsUpAction.Get(), ETriggerEvent::Started, this, &UMissionBriefingDlg::OnMissionShieldsUp);
    else UE_LOG(LogTemp, Warning, TEXT("[ShieldInput] IA_ShieldsUp must be Boolean."));

    MissionShieldsDownAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_ShieldsDown.IA_ShieldsDown"));
    if (MissionShieldsDownAction && MissionShieldsDownAction->ValueType == EInputActionValueType::Boolean)
        MissionMenuInput->BindAction(MissionShieldsDownAction.Get(), ETriggerEvent::Started, this, &UMissionBriefingDlg::OnMissionShieldsDown);
    else UE_LOG(LogTemp, Warning, TEXT("[ShieldInput] IA_ShieldsDown must be Boolean."));

    MissionShieldsFullAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_ShieldsFull.IA_ShieldsFull"));
    if (MissionShieldsFullAction && MissionShieldsFullAction->ValueType == EInputActionValueType::Boolean)
        MissionMenuInput->BindAction(MissionShieldsFullAction.Get(), ETriggerEvent::Started, this, &UMissionBriefingDlg::OnMissionShieldsFull);
    else UE_LOG(LogTemp, Warning, TEXT("[ShieldInput] IA_ShieldsFull must be Boolean."));

    MissionShieldsZeroAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_ShieldsZero.IA_ShieldsZero"));
    if (MissionShieldsZeroAction && MissionShieldsZeroAction->ValueType == EInputActionValueType::Boolean)
        MissionMenuInput->BindAction(MissionShieldsZeroAction.Get(), ETriggerEvent::Started, this, &UMissionBriefingDlg::OnMissionShieldsZero);
    else UE_LOG(LogTemp, Warning, TEXT("[ShieldInput] IA_ShieldsZero must be Boolean."));

    MissionPitchAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_Pitch.IA_Pitch"));
    if (MissionPitchAction && MissionPitchAction->ValueType == EInputActionValueType::Axis1D)
    {
        MissionMenuInput->BindAction(MissionPitchAction.Get(), ETriggerEvent::Triggered, this, &UMissionBriefingDlg::OnMissionPitch);
        MissionMenuInput->BindAction(MissionPitchAction.Get(), ETriggerEvent::Completed, this, &UMissionBriefingDlg::ReleaseMissionPitch);
        MissionMenuInput->BindAction(MissionPitchAction.Get(), ETriggerEvent::Canceled, this, &UMissionBriefingDlg::ReleaseMissionPitch);
    }
    else UE_LOG(LogTemp, Warning, TEXT("[FlightInput] IA_Pitch must be Axis1D in /Game/Input."));

    MissionYawAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_Yaw.IA_Yaw"));
    if (MissionYawAction && MissionYawAction->ValueType == EInputActionValueType::Axis1D)
    {
        MissionMenuInput->BindAction(MissionYawAction.Get(), ETriggerEvent::Triggered, this, &UMissionBriefingDlg::OnMissionYaw);
        MissionMenuInput->BindAction(MissionYawAction.Get(), ETriggerEvent::Completed, this, &UMissionBriefingDlg::ReleaseMissionYaw);
        MissionMenuInput->BindAction(MissionYawAction.Get(), ETriggerEvent::Canceled, this, &UMissionBriefingDlg::ReleaseMissionYaw);
    }
    else UE_LOG(LogTemp, Warning, TEXT("[FlightInput] IA_Yaw must be Axis1D in /Game/Input."));

    MissionRollAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_Roll.IA_Roll"));
    if (MissionRollAction && MissionRollAction->ValueType == EInputActionValueType::Axis1D)
    {
        MissionMenuInput->BindAction(MissionRollAction.Get(), ETriggerEvent::Triggered, this, &UMissionBriefingDlg::OnMissionRoll);
        MissionMenuInput->BindAction(MissionRollAction.Get(), ETriggerEvent::Completed, this, &UMissionBriefingDlg::ReleaseMissionRoll);
        MissionMenuInput->BindAction(MissionRollAction.Get(), ETriggerEvent::Canceled, this, &UMissionBriefingDlg::ReleaseMissionRoll);
    }
    else UE_LOG(LogTemp, Warning, TEXT("[FlightInput] IA_Roll must be Axis1D in /Game/Input."));

    MissionThrottleAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_Throttle.IA_Throttle"));
    if (MissionThrottleAction && MissionThrottleAction->ValueType == EInputActionValueType::Axis1D)
        MissionMenuInput->BindAction(MissionThrottleAction.Get(), ETriggerEvent::Started, this, &UMissionBriefingDlg::OnMissionThrottleStep);
    else UE_LOG(LogTemp, Warning, TEXT("[ThrottleInput] Missing or wrong value type: IA_Throttle (Axis1D)."));

    MissionThrottleZeroAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_ThrottleZero.IA_ThrottleZero"));
    if (MissionThrottleZeroAction && MissionThrottleZeroAction->ValueType == EInputActionValueType::Boolean)
        MissionMenuInput->BindAction(MissionThrottleZeroAction.Get(), ETriggerEvent::Started, this, &UMissionBriefingDlg::OnMissionThrottleZero);
    else UE_LOG(LogTemp, Warning, TEXT("[ThrottleInput] Missing or wrong value type: IA_ThrottleZero (Boolean)."));

    MissionThrottleFullAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_ThrottleFull.IA_ThrottleFull"));
    if (MissionThrottleFullAction && MissionThrottleFullAction->ValueType == EInputActionValueType::Boolean)
        MissionMenuInput->BindAction(MissionThrottleFullAction.Get(), ETriggerEvent::Started, this, &UMissionBriefingDlg::OnMissionThrottleFull);
    else UE_LOG(LogTemp, Warning, TEXT("[ThrottleInput] Missing or wrong value type: IA_ThrottleFull (Boolean)."));

    GearToggleAction = LoadObject<UInputAction>(nullptr,TEXT("/Game/Input/IA_GearToggle.IA_GearToggle"));
    if (GearToggleAction && GearToggleAction->ValueType == EInputActionValueType::Boolean)
        MissionMenuInput->BindAction(GearToggleAction.Get(),ETriggerEvent::Started,this,&UMissionBriefingDlg::ToggleMissionGear);
    else UE_LOG(LogTemp,Warning,TEXT("[GearInput] Missing Boolean IA_GearToggle."));

    EngineeringAction = LoadObject<UInputAction>(nullptr,TEXT("/Game/Input/IA_EngineeringPanel.IA_EngineeringPanel"));
    if (EngineeringAction && EngineeringAction->ValueType == EInputActionValueType::Boolean)
        MissionMenuInput->BindAction(EngineeringAction.Get(),ETriggerEvent::Started,this,&UMissionBriefingDlg::ToggleEngineering);
    else UE_LOG(LogTemp,Warning,TEXT("[Engineering] IA_EngineeringPanel must be Boolean and mapped in IMC_FighterHUD."));

    RadioChoiceContext = NewObject<UInputMappingContext>(this,NAME_None,RF_Transient);
    RadioChoiceActions.Reset();
    {
        UInputAction* Choice=NewObject<UInputAction>(this,NAME_None,RF_Transient);
        Choice->ValueType=EInputActionValueType::Boolean;
        RadioChoiceActions.Add(Choice);
        Choice->Rename(TEXT("SSW_RadioChoice0"),RadioChoiceContext.Get());
        RadioChoiceContext->MapKey(Choice,EKeys::Zero);
        MissionMenuInput->BindAction(Choice,ETriggerEvent::Started,this,&UMissionBriefingDlg::SelectFighterRadio0);
    }
    {
        UInputAction* Choice=NewObject<UInputAction>(this,NAME_None,RF_Transient);
        Choice->ValueType=EInputActionValueType::Boolean;
        RadioChoiceActions.Add(Choice);
        Choice->Rename(TEXT("SSW_RadioChoice1"),RadioChoiceContext.Get());
        RadioChoiceContext->MapKey(Choice,EKeys::One);
        MissionMenuInput->BindAction(Choice,ETriggerEvent::Started,this,&UMissionBriefingDlg::SelectFighterRadio1);
    }
    {
        UInputAction* Choice=NewObject<UInputAction>(this,NAME_None,RF_Transient);
        Choice->ValueType=EInputActionValueType::Boolean;
        RadioChoiceActions.Add(Choice);
        Choice->Rename(TEXT("SSW_RadioChoice2"),RadioChoiceContext.Get());
        RadioChoiceContext->MapKey(Choice,EKeys::Two);
        MissionMenuInput->BindAction(Choice,ETriggerEvent::Started,this,&UMissionBriefingDlg::SelectFighterRadio2);
    }
    {
        UInputAction* Choice=NewObject<UInputAction>(this,NAME_None,RF_Transient);
        Choice->ValueType=EInputActionValueType::Boolean;
        RadioChoiceActions.Add(Choice);
        Choice->Rename(TEXT("SSW_RadioChoice3"),RadioChoiceContext.Get());
        RadioChoiceContext->MapKey(Choice,EKeys::Three);
        MissionMenuInput->BindAction(Choice,ETriggerEvent::Started,this,&UMissionBriefingDlg::SelectFighterRadio3);
    }
    {
        UInputAction* Choice=NewObject<UInputAction>(this,NAME_None,RF_Transient);
        Choice->ValueType=EInputActionValueType::Boolean;
        RadioChoiceActions.Add(Choice);
        Choice->Rename(TEXT("SSW_RadioChoice4"),RadioChoiceContext.Get());
        RadioChoiceContext->MapKey(Choice,EKeys::Four);
        MissionMenuInput->BindAction(Choice,ETriggerEvent::Started,this,&UMissionBriefingDlg::SelectFighterRadio4);
    }
    {
        UInputAction* Choice=NewObject<UInputAction>(this,NAME_None,RF_Transient);
        Choice->ValueType=EInputActionValueType::Boolean;
        RadioChoiceActions.Add(Choice);
        Choice->Rename(TEXT("SSW_RadioChoice5"),RadioChoiceContext.Get());
        RadioChoiceContext->MapKey(Choice,EKeys::Five);
        MissionMenuInput->BindAction(Choice,ETriggerEvent::Started,this,&UMissionBriefingDlg::SelectFighterRadio5);
    }
    {
        UInputAction* Choice=NewObject<UInputAction>(this,NAME_None,RF_Transient);
        Choice->ValueType=EInputActionValueType::Boolean;
        Choice->bConsumeInput=true;
        RadioChoiceActions.Add(Choice);
        Choice->Rename(TEXT("SSW_RadioChoice6"),RadioChoiceContext.Get());
        RadioChoiceContext->MapKey(Choice,EKeys::Six);
        MissionMenuInput->BindAction(Choice,ETriggerEvent::Started,this,&UMissionBriefingDlg::SelectFighterRadio6);
    }
    {
        UInputAction* Choice=NewObject<UInputAction>(this,NAME_None,RF_Transient);
        Choice->ValueType=EInputActionValueType::Boolean;
        Choice->bConsumeInput=true;
        RadioChoiceActions.Add(Choice);
        Choice->Rename(TEXT("SSW_RadioChoice7"),RadioChoiceContext.Get());
        RadioChoiceContext->MapKey(Choice,EKeys::Seven);
        MissionMenuInput->BindAction(Choice,ETriggerEvent::Started,this,&UMissionBriefingDlg::SelectFighterRadio7);
    }
    {
        UInputAction* Choice=NewObject<UInputAction>(this,NAME_None,RF_Transient);
        Choice->ValueType=EInputActionValueType::Boolean;
        Choice->bConsumeInput=true;
        RadioChoiceActions.Add(Choice);
        Choice->Rename(TEXT("SSW_RadioChoice8"),RadioChoiceContext.Get());
        RadioChoiceContext->MapKey(Choice,EKeys::Eight);
        MissionMenuInput->BindAction(Choice,ETriggerEvent::Started,this,&UMissionBriefingDlg::SelectFighterRadio8);
    }
    {
        UInputAction* Choice=NewObject<UInputAction>(this,NAME_None,RF_Transient);
        Choice->ValueType=EInputActionValueType::Boolean;
        Choice->bConsumeInput=true;
        RadioChoiceActions.Add(Choice);
        Choice->Rename(TEXT("SSW_RadioChoice9"),RadioChoiceContext.Get());
        RadioChoiceContext->MapKey(Choice,EKeys::Nine);
        MissionMenuInput->BindAction(Choice,ETriggerEvent::Started,this,&UMissionBriefingDlg::SelectFighterRadio9);
    }


    // Transient context remains for the wheel and existing HUD shortcuts only.
    MissionTargetContext = NewObject<UInputMappingContext>(this, NAME_None, RF_Transient);
    // No player pause or travel-acceleration bindings in the persistent universe.
    // Use the editor-authored actions and IMC mappings, including chord triggers.
    PreviousMissionTargetAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_PreviousTarget.IA_PreviousTarget"));
    NextMissionTargetAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_NextTarget.IA_NextTarget"));
    bPreviousTargetHeld = false;
    if (PreviousMissionTargetAction && PreviousMissionTargetAction->ValueType == EInputActionValueType::Boolean)
    {
        MissionMenuInput->BindAction(PreviousMissionTargetAction.Get(), ETriggerEvent::Triggered, this, &UMissionBriefingDlg::PreviousMissionTarget);
        MissionMenuInput->BindAction(PreviousMissionTargetAction.Get(), ETriggerEvent::Completed, this, &UMissionBriefingDlg::ReleasePreviousMissionTarget);
        MissionMenuInput->BindAction(PreviousMissionTargetAction.Get(), ETriggerEvent::Canceled, this, &UMissionBriefingDlg::ReleasePreviousMissionTarget);
    }
    else UE_LOG(LogTemp, Warning, TEXT("[MissionInput] Missing Boolean IA_PreviousTarget."));
    if (NextMissionTargetAction && NextMissionTargetAction->ValueType == EInputActionValueType::Boolean)
    {
        const auto HasNextTargetMapping = [this](const UInputMappingContext* Context)
        {
            return Context && Context->GetMappings().ContainsByPredicate([this](const FEnhancedActionKeyMapping& Mapping)
            {
                return Mapping.Action.Get() == NextMissionTargetAction.Get();
            });
        };
        if (!HasNextTargetMapping(FighterInputContext.Get()) && !HasNextTargetMapping(MissionMenuContext.Get()))
            MissionTargetContext->MapKey(NextMissionTargetAction.Get(), EKeys::T);

        MissionMenuInput->BindAction(NextMissionTargetAction.Get(), ETriggerEvent::Started, this, &UMissionBriefingDlg::NextMissionTarget);
    }
    else UE_LOG(LogTemp, Warning, TEXT("[MissionInput] Missing Boolean IA_NextTarget."));
    // Use the transient wheel context so asset paths, Hold triggers,
    // or boolean action settings cannot suppress the one-frame wheel pulse.
    MissionTargetZoomAction = NewObject<UInputAction>(this, TEXT("SSW_TargetZoom"), RF_Transient);
    MissionTargetZoomAction->ValueType = EInputActionValueType::Axis1D;
    MissionTargetZoomAction->bConsumeInput = true;
    MissionTargetContext->MapKey(MissionTargetZoomAction.Get(), EKeys::MouseWheelAxis);
    MissionMenuInput->BindAction(MissionTargetZoomAction.Get(), ETriggerEvent::Triggered,
        this, &UMissionBriefingDlg::OnTargetZoom);
    FighterHUDToggleAction = NewObject<UInputAction>(this, TEXT("SSW_HUDVisibility"), RF_Transient);
    FighterHUDToggleAction->ValueType = EInputActionValueType::Boolean;
    MissionTargetContext->MapKey(FighterHUDToggleAction.Get(), EKeys::H);
    MissionMenuInput->BindAction(FighterHUDToggleAction.Get(), ETriggerEvent::Started,
        this, &UMissionBriefingDlg::ToggleFighterHUD);
    auto* MouseSteeringToggle = NewObject<UInputAction>(MissionTargetContext.Get(), TEXT("SSW_MouseSteering"), RF_Transient);
    MouseSteeringToggle->ActionDescription = NSLOCTEXT("SSW.InputActions", "MouseSteering", "Toggle Mouse Steering");
    MissionTargetContext->MapKey(MouseSteeringToggle, FKey(TEXT("#")));
    MissionMenuInput->BindAction(MouseSteeringToggle, ETriggerEvent::Started,
        this, &UMissionBriefingDlg::ToggleMouseSteering);
    if (MissionPitchAction && MissionPitchAction->ValueType == EInputActionValueType::Axis1D)
        MissionTargetContext->MapKey(MissionPitchAction.Get(), EKeys::MouseY);
    if (MissionYawAction && MissionYawAction->ValueType == EInputActionValueType::Axis1D)
        MissionTargetContext->MapKey(MissionYawAction.Get(), EKeys::MouseX);
    bFighterHUDVisible = true;
    if (!FighterHUDPanels.IsValid() && GetWorld() && GetWorld()->GetGameViewport())
    {
        TWeakObjectPtr<UMissionBriefingDlg> HUDOwner(this);
        FighterHUDPanels = SNew(SFighterHUDPanels).ShowPanels_Lambda([HUDOwner]()
        {
            Ship* Pilot=Sim::GetSim()?Sim::GetSim()->GetPlayerShip():nullptr;
            const bool Staged=Pilot && Pilot->GetDock() && Pilot->GetDock()->IsLaunchDeck() && Pilot->GetFlightPhase()<EOPSMode::ACTIVE;
            return HUDOwner.IsValid() && HUDOwner->CanShowFighterHUD() && !Staged &&
                HUDOwner->bFighterHUDVisible && !(HUDOwner->MissionCameraRig.IsValid() && HUDOwner->MissionCameraRig->bThirdPersonView) && !(HUDOwner->FighterHUDDetails.IsValid() && HUDOwner->FighterHUDDetails->IsCockpitDisplayActive()) && !HUDOwner->MissionSceneCover.IsValid() &&
                !HUDOwner->LiveOptionsPopup && !HUDOwner->CargoManifestPopup && !HUDOwner->MarketplacePopup && !HUDOwner->ProductionPopup && !HUDOwner->StationServicesPopup && !HUDOwner->ShipStorePopup && !HUDOwner->AgentsPopup && !HUDOwner->PlayersPopup && !HUDOwner->AwardsPopup && !HUDOwner->WalletPopup && !HUDOwner->GuildsPopup && !HUDOwner->MiningDronesPopup && !HUDOwner->SkillsPopup && !HUDOwner->HangarPopup && !HUDOwner->InsurancePopup && !HUDOwner->LoadoutsPopup && !HUDOwner->FittingPopup && !HUDOwner->ContractsPopup && !HUDOwner->ReferencePopup &&
                !(HUDOwner->MissionQuitMenu && HUDOwner->MissionQuitMenu->IsMenuShown());
        });
        GetWorld()->GetGameViewport()->AddViewportWidgetContent(FighterHUDPanels.ToSharedRef(), 800);
    }
    FighterInputContext = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Input/IMC_FighterHUD.IMC_FighterHUD"));
    // Compatibility with the existing asset's current name.
    if (!FighterInputContext)
        FighterInputContext = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Input/IMC_Fighter.IMC_Fighter"));
    FighterInputContext=StarshatterInputBindings::RuntimeContext(FighterInputContext.Get(),this);
    if (GearToggleAction && GearToggleAction->ValueType == EInputActionValueType::Boolean)
    {
        const auto HasGearMapping = [this](const UInputMappingContext* Context)
        {
            return Context && Context->GetMappings().ContainsByPredicate([this](const FEnhancedActionKeyMapping& Mapping)
            {
                return Mapping.Action.Get() == GearToggleAction.Get();
            });
        };
        if (!HasGearMapping(FighterInputContext.Get()) && !HasGearMapping(MissionMenuContext.Get()))
            MissionTargetContext->MapKey(GearToggleAction.Get(), EKeys::G);
    }
    bAddedFighterInputContext = FighterInputContext && !Input->HasMappingContext(FighterInputContext.Get());
    if (bAddedFighterInputContext) Input->AddMappingContext(FighterInputContext.Get(), 1000);
    if (!FighterInputContext)
        UE_LOG(LogTemp, Warning, TEXT("[FighterHUD] Missing IMC_Fighter (or IMC_FighterHUD) in /Game/Input."));
    FlightOpsPanelAction=LoadObject<UInputAction>(nullptr,TEXT("/Game/Input/IA_FlightOpsPanel.IA_FlightOpsPanel"));
    if(FlightOpsPanelAction && FlightOpsPanelAction->ValueType==EInputActionValueType::Boolean)
        MissionMenuInput->BindAction(FlightOpsPanelAction.Get(),ETriggerEvent::Started,this,&UMissionBriefingDlg::ToggleFlightOpsPopup);
    ObjectivesPanelAction=LoadObject<UInputAction>(nullptr,TEXT("/Game/Input/IA_ObjectivesPanel.IA_ObjectivesPanel"));
    if(ObjectivesPanelAction && ObjectivesPanelAction->ValueType==EInputActionValueType::Boolean)
        MissionMenuInput->BindAction(ObjectivesPanelAction.Get(),ETriggerEvent::Started,this,&UMissionBriefingDlg::ToggleObjectivesPopup);
    else UE_LOG(LogTemp,Warning,TEXT("[MissionHUD] Create Boolean IA_ObjectivesPanel and map it in IMC_FighterHUD."));
    NavMapAction = nullptr;
    // Search both authored mission contexts before installing a fallback.
    const UInputMappingContext* NavigationContexts[] = {FighterInputContext.Get(), MissionMenuContext.Get()};
    for (const UInputMappingContext* NavContext : NavigationContexts)
    {
        if (!NavContext) continue;
        for (const auto& NavMapping : NavContext->GetMappings())
        {
            const UInputAction* MappedNavigationAction = NavMapping.Action.Get();
            if (!MappedNavigationAction) continue;
            const FString NavigationActionName = MappedNavigationAction->GetName();
            if (NavigationActionName == TEXT("IA_NavMap") || NavigationActionName == TEXT("IA_NavigationPanel"))
            {
                NavMapAction = const_cast<UInputAction*>(MappedNavigationAction);
                break;
            }
        }
        if (NavMapAction) break;
    }
    if (!NavMapAction)
    {
        NavMapAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_NavMap.IA_NavMap"));
        if (!NavMapAction)
            NavMapAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_NavigationPanel.IA_NavigationPanel"));
        if (!NavMapAction)
        {
            NavMapAction = NewObject<UInputAction>(this, NAME_None, RF_Transient);
            NavMapAction->ValueType = EInputActionValueType::Boolean;
        }
        MissionTargetContext->MapKey(NavMapAction.Get(), EKeys::N);
        UE_LOG(LogTemp, Display, TEXT("[NavigationInput] No authored navigation mapping; using N for %s"), *GetNameSafe(NavMapAction.Get()));
    }
    MissionMenuInput->BindAction(NavMapAction.Get(), ETriggerEvent::Started, this, &UMissionBriefingDlg::ToggleNavigationPopup);
    UE_LOG(LogTemp, Display, TEXT("[NavigationInput] Bound %s"), *GetNameSafe(NavMapAction.Get()));
    WeaponsPanelAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_WeaponsPanel.IA_WeaponsPanel"));
    if (WeaponsPanelAction && WeaponsPanelAction->ValueType == EInputActionValueType::Boolean)
        MissionMenuInput->BindAction(WeaponsPanelAction.Get(), ETriggerEvent::Started, this, &UMissionBriefingDlg::ToggleFighterWeaponsPanel);
    else UE_LOG(LogTemp, Warning, TEXT("[FighterHUD] Missing Boolean IA_WeaponsPanel in /Game/Input."));
    HUDWarningsAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_HUDWarnings.IA_HUDWarnings"));
    if (HUDWarningsAction && HUDWarningsAction->ValueType == EInputActionValueType::Boolean)
        MissionMenuInput->BindAction(HUDWarningsAction.Get(), ETriggerEvent::Started, this, &UMissionBriefingDlg::ToggleFighterCautionPanel);
    else UE_LOG(LogTemp, Warning, TEXT("[FighterHUD] Missing Boolean IA_HUDWarnings in /Game/Input."));
    HUDInformationAction=nullptr; // Mission/radio messages control their own visibility.
    FighterMFDLeftAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_MFDLeftCycle.IA_MFDLeftCycle"));
    FighterMFDRightAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_MFDRightCycle.IA_MFDRightCycle"));
    if (FighterMFDLeftAction && FighterMFDLeftAction->ValueType == EInputActionValueType::Boolean)
        MissionMenuInput->BindAction(FighterMFDLeftAction.Get(), ETriggerEvent::Started, this, &UMissionBriefingDlg::CycleFighterMFDLeft);
    else UE_LOG(LogTemp, Warning, TEXT("[FighterHUD] IA_MFDLeftCycle is missing or is not Boolean."));
    if (FighterMFDRightAction && FighterMFDRightAction->ValueType == EInputActionValueType::Boolean)
        MissionMenuInput->BindAction(FighterMFDRightAction.Get(), ETriggerEvent::Started, this, &UMissionBriefingDlg::CycleFighterMFDRight);
    else UE_LOG(LogTemp, Warning, TEXT("[FighterHUD] IA_MFDRightCycle is missing or is not Boolean."));
    MissionWeaponActions.Reset();
    auto BindWeaponAction = [this](const TCHAR* ActionName, ETriggerEvent Event, void (UMissionBriefingDlg::*Handler)()) {
        // Bind the asset used by the mapping context, including actions in subfolders.
        UInputAction* Action = nullptr;
        const UInputMappingContext* Contexts[] = { FighterInputContext.Get(), MissionMenuContext.Get() };
        for (const UInputMappingContext* Context : Contexts) {
            if (!Context) continue;
            for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings()) {
                if (Mapping.Action && Mapping.Action->GetName().Equals(ActionName, ESearchCase::IgnoreCase)) {
                    Action = const_cast<UInputAction*>(Mapping.Action.Get());
                    break;
                }
            }
            if (Action) break;
        }
        if (!Action) {
            const FString Path = FString::Printf(TEXT("/Game/Input/%s.%s"), ActionName, ActionName);
            Action = LoadObject<UInputAction>(nullptr, *Path);
        }
        if (Action && Action->ValueType == EInputActionValueType::Boolean) {
            MissionWeaponActions.Add(Action);
            MissionMenuInput->BindAction(Action, Event, this, Handler);
        } else UE_LOG(LogTemp, Warning, TEXT("[WeaponsInput] Missing Boolean action %s"), ActionName);
    };
    // Resolve the actual mapped action, including assets moved to subfolders.
    // Give its existing mappings priority over overlapping flight/FLCS actions.
    // No physical key is hard-coded: changing M to B in the IMC is respected.
    if (AutoNavInputContext) Input->RemoveMappingContext(AutoNavInputContext.Get());
    AutoNavInputContext = NewObject<UInputMappingContext>(this);
    TSet<const UInputAction*> BoundAutoActions;
    const UInputMappingContext* AutoContexts[] = { FighterInputContext.Get(), MissionMenuContext.Get() };
    for (const UInputMappingContext* Context : AutoContexts) {
        if (!Context) continue;
        for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings()) {
            if (!Mapping.Action || !Mapping.Action->GetName().Equals(TEXT("IA_AutoNav"), ESearchCase::IgnoreCase)) continue;
            UInputAction* AutoAction = const_cast<UInputAction*>(Mapping.Action.Get());
            if (AutoAction->ValueType != EInputActionValueType::Boolean) {
                UE_LOG(LogTemp, Error, TEXT("[AutoNavInput] %s must be Boolean"), *AutoAction->GetPathName());
                continue;
            }
            AutoNavInputContext->MapKey(AutoAction, Mapping.Key) = Mapping;
            if (!BoundAutoActions.Contains(AutoAction)) {
                BoundAutoActions.Add(AutoAction);
                MissionWeaponActions.Add(AutoAction);
                MissionMenuInput->BindAction(AutoAction, ETriggerEvent::Started, this, &UMissionBriefingDlg::ToggleMissionAutoNav);
            }
            UE_LOG(LogTemp, Display, TEXT("[AutoNavInput] %s -> %s"), *Mapping.Key.ToString(), *AutoAction->GetPathName());
        }
    }
    // Use the mapped radio action and keep the player's resolved key and triggers.
    for (const UInputMappingContext* Context : AutoContexts) {
        if (!Context) continue;
        for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings()) {
            if (!Mapping.Action || !Mapping.Action->GetName().Equals(TEXT("IA_RadioMenu"), ESearchCase::IgnoreCase) ||
                Mapping.Action->ValueType != EInputActionValueType::Boolean) continue;
            UInputAction* MappedRadioAction = const_cast<UInputAction*>(Mapping.Action.Get());
            if (RadioMenuAction && RadioMenuAction.Get() != MappedRadioAction) continue;
            if (!RadioMenuAction) {
                RadioMenuAction = MappedRadioAction;
                MissionMenuInput->BindAction(MappedRadioAction, ETriggerEvent::Started, this, &UMissionBriefingDlg::ToggleFighterRadio);
            }
            AutoNavInputContext->MapKey(MappedRadioAction, Mapping.Key) = Mapping;
        }
    }
    // Raise the configured boost mapping above camera defaults.
    for (const UInputMappingContext* Context : AutoContexts) {
        if (!Context) continue;
        for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings()) {
            if (!Mapping.Action || !Mapping.Action->GetName().Equals(TEXT("IA_Augmenter"), ESearchCase::IgnoreCase)) continue;
            UInputAction* BoostAction = const_cast<UInputAction*>(Mapping.Action.Get());
            if (BoostAction->ValueType != EInputActionValueType::Boolean) continue;
            AutoNavInputContext->MapKey(BoostAction, Mapping.Key) = Mapping;
            if (BoostAction != MissionAugmenterAction.Get() && !MissionWeaponActions.Contains(BoostAction)) {
                MissionWeaponActions.Add(BoostAction);
                MissionMenuInput->BindAction(BoostAction, ETriggerEvent::Triggered, this, &UMissionBriefingDlg::OnMissionAugmenter);
                MissionMenuInput->BindAction(BoostAction, ETriggerEvent::Completed, this, &UMissionBriefingDlg::ReleaseMissionAugmenter);
                MissionMenuInput->BindAction(BoostAction, ETriggerEvent::Canceled, this, &UMissionBriefingDlg::ReleaseMissionAugmenter);
            }
            UE_LOG(LogTemp, Display, TEXT("[AugmenterInput] %s -> %s"), *Mapping.Key.ToString(), *BoostAction->GetPathName());
        }
    }
    if (AutoNavInputContext->GetMappings().Num())
    {
        // Register the authored signature first to retain existing saved rebinds.
        StarshatterInputBindings::RegisterContext(AutoNavInputContext.Get());
        for(int32 Index=0;Index<AutoNavInputContext->GetMappings().Num();++Index)
        {
            StarshatterInputBindings::ConfigureAugmenterHold(AutoNavInputContext->GetMapping(Index),AutoNavInputContext.Get());
        }
        Input->AddMappingContext(AutoNavInputContext.Get(),1105);
    }
    else UE_LOG(LogTemp, Error, TEXT("[AutoNavInput] No IA_AutoNav mapping found in the active fighter/menu contexts."));
    BindWeaponAction(TEXT("IA_FirePrimary"), ETriggerEvent::Triggered, &UMissionBriefingDlg::FireMissionPrimary);
    BindWeaponAction(TEXT("IA_FireSecondary"), ETriggerEvent::Started, &UMissionBriefingDlg::FireMissionSecondary);
    BindWeaponAction(TEXT("IA_Decoy"), ETriggerEvent::Started, &UMissionBriefingDlg::FireMissionDecoy);
    BindWeaponAction(TEXT("IA_LaunchProbe"), ETriggerEvent::Started, &UMissionBriefingDlg::LaunchMissionProbe);
    BindWeaponAction(TEXT("IA_PrimaryWeaponCycle"), ETriggerEvent::Started, &UMissionBriefingDlg::CycleMissionPrimary);
    BindWeaponAction(TEXT("IA_SecondaryWeaponCycle"), ETriggerEvent::Started, &UMissionBriefingDlg::CycleMissionSecondary);
    BindWeaponAction(TEXT("IA_WeaponPattern"), ETriggerEvent::Started, &UMissionBriefingDlg::CycleMissionPattern);
    BindWeaponAction(TEXT("IA_WeaponSweep"), ETriggerEvent::Started, &UMissionBriefingDlg::CycleMissionSweep);
    BindWeaponAction(TEXT("IA_DropWeaponTarget"), ETriggerEvent::Started, &UMissionBriefingDlg::DropMissionWeaponTarget);
    // Bind the authored asset from the active HUD context, including subfolders.
    UInputAction* ContactsAction = nullptr;
    if (FighterInputContext) for (const auto& Mapping : FighterInputContext->GetMappings())
        if (Mapping.Action && Mapping.Action->GetName() == TEXT("IA_ContactsPanel"))
            { ContactsAction = const_cast<UInputAction*>(Mapping.Action.Get()); break; }
    if (!ContactsAction) ContactsAction = LoadObject<UInputAction>(nullptr,
        TEXT("/Game/Input/Targeting/IA_ContactsPanel.IA_ContactsPanel"));
    if (!ContactsAction) ContactsAction = LoadObject<UInputAction>(nullptr,
        TEXT("/Game/Input/IA_ContactsPanel.IA_ContactsPanel"));
    if (ContactsAction) {
        MissionWeaponActions.Add(ContactsAction);
        MissionMenuInput->BindAction(ContactsAction, ETriggerEvent::Started, this, &UMissionBriefingDlg::ToggleContactsPanel);
    } else UE_LOG(LogTemp, Error, TEXT("[Contacts] IA_ContactsPanel not found"));
    BindWeaponAction(TEXT("IA_SensorMode"), ETriggerEvent::Started, &UMissionBriefingDlg::CycleMissionSensor);
    BindWeaponAction(TEXT("IA_SensorGroundMode"), ETriggerEvent::Started, &UMissionBriefingDlg::MissionSensorGround);
    bSensorRangeUpHeld = bSensorRangeDownHeld = false;
    // Started can precede chord satisfaction. Act only once the full chord triggers.
    BindWeaponAction(TEXT("IA_SensorRangeUp"), ETriggerEvent::Triggered, &UMissionBriefingDlg::MissionSensorRangeUp);
    BindWeaponAction(TEXT("IA_SensorRangeUp"), ETriggerEvent::Completed, &UMissionBriefingDlg::ReleaseSensorRangeUp);
    BindWeaponAction(TEXT("IA_SensorRangeUp"), ETriggerEvent::Canceled, &UMissionBriefingDlg::ReleaseSensorRangeUp);
    BindWeaponAction(TEXT("IA_SensorRangeDown"), ETriggerEvent::Triggered, &UMissionBriefingDlg::MissionSensorRangeDown);
    BindWeaponAction(TEXT("IA_SensorRangeDown"), ETriggerEvent::Completed, &UMissionBriefingDlg::ReleaseSensorRangeDown);
    BindWeaponAction(TEXT("IA_SensorRangeDown"), ETriggerEvent::Canceled, &UMissionBriefingDlg::ReleaseSensorRangeDown);
    BindWeaponAction(TEXT("IA_EMCONUp"), ETriggerEvent::Started, &UMissionBriefingDlg::MissionEMCONUp);
    BindWeaponAction(TEXT("IA_EMCONDown"), ETriggerEvent::Started, &UMissionBriefingDlg::MissionEMCONDown);
    BindWeaponAction(TEXT("IA_TargetNearest"), ETriggerEvent::Started, &UMissionBriefingDlg::LockMissionClosest);
    BindWeaponAction(TEXT("IA_TargetHostile"), ETriggerEvent::Started, &UMissionBriefingDlg::LockMissionHostile);
    BindWeaponAction(TEXT("IA_TargetNearestThreat"), ETriggerEvent::Started, &UMissionBriefingDlg::LockMissionThreat);
    BindWeaponAction(TEXT("IA_TargetSubsystemNext"), ETriggerEvent::Started, &UMissionBriefingDlg::CycleMissionSubsystem);
    BindWeaponAction(TEXT("IA_TargetSubsystemPrevious"), ETriggerEvent::Started, &UMissionBriefingDlg::CycleMissionSubsystemPrevious);
    BindWeaponAction(TEXT("IA_TargetHostileThreat"), ETriggerEvent::Started, &UMissionBriefingDlg::LockMissionHostileThreat);
    UInputAction* AutoTrackAction = StarshatterInputBindings::MapTargetTracking(MissionTargetContext.Get());
    MissionMenuInput->BindAction(AutoTrackAction, ETriggerEvent::Started, this, &UMissionBriefingDlg::ToggleMissionAutoTrack);
    FighterWeaponCycleAction = NewObject<UInputAction>(this, TEXT("SSW_WeaponCycle"), RF_Transient);
    FighterWeaponCycleAction->ValueType = EInputActionValueType::Boolean;
    MissionTargetContext->MapKey(FighterWeaponCycleAction.Get(), EKeys::BackSpace);
    MissionMenuInput->BindAction(FighterWeaponCycleAction.Get(), ETriggerEvent::Started, this, &UMissionBriefingDlg::CycleFighterWeapon);
    if (!FighterHUDDetails.IsValid() && GetWorld() && GetWorld()->GetGameViewport())
    {
        TWeakObjectPtr<UMissionBriefingDlg> Owner(this);
        FighterHUDDetails = SNew(SFighterHUDDetails)
            .ThirdPerson_Lambda([Owner]() { return Owner.IsValid() && Owner->MissionCameraRig.IsValid() && Owner->MissionCameraRig->bThirdPersonView; })
            .CameraMode_Lambda([Owner]() -> FString {
                return Owner.IsValid() && Owner->MissionCameraRig ? Owner->MissionCameraRig->ModeLabel() : TEXT("");
            })
            .ShowHUD_Lambda([Owner]() { return Owner.IsValid() && Owner->bFighterHUDVisible && Owner->CanShowFighterHUD(); })
            .SelectedTarget_Lambda([Owner]() { return Owner.IsValid() ? Owner->SelectedMissionTarget : TWeakObjectPtr<AActor>(); })
            .SelectedName_Lambda([Owner]() { return Owner.IsValid() ? Owner->SelectedMissionTargetName : FText::GetEmpty(); })
            .OnCockpitShortcut_Lambda([Owner](FName Action)
            {
                if (!Owner.IsValid()) return;
                if (Action==TEXT("Contacts")) Owner->ToggleContactsPanel();
                else if (Action==TEXT("Engineering")) Owner->ToggleEngineering();
                else if (Action==TEXT("Weapons")) Owner->ToggleFighterWeaponsPanel();
                else if (Action==TEXT("Navigation")) Owner->ToggleNavigationPopup();
                else if (Action==TEXT("Radio")) Owner->ToggleFighterRadio();
                else if (Action==TEXT("Camera")) Owner->ToggleFlightCameraPanel();
                else if (Action==TEXT("Cargo")) Owner->ToggleCargoManifest();
                else if (Action==TEXT("Player")) Owner->TogglePlayers();
                else if (Action==TEXT("Options")) Owner->ToggleLiveOptions();
                else if (Action==TEXT("Exit")) Owner->ToggleMissionMenu();
            })
            .OnOverviewWheel_Lambda([Owner](float Wheel)
            {
                if (!Owner.IsValid() || !Owner->MissionCameraRig ||
                    Owner->MissionCameraRig->Mode != FMissionCameraRig::Overview || !Owner->CanUseFighterHUD()) return false;
                Owner->OnTargetZoom(FInputActionValue(Wheel));
                return true;
            });
        GetWorld()->GetGameViewport()->AddViewportWidgetContent(FighterHUDDetails.ToSharedRef(), 810);
    }
    InstallFlightPanelToolbar();
    QuantumMenuAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_QuantumMenu.IA_QuantumMenu"));
    bool bQuantumMapped = false;
    for (const UInputMappingContext* Context : {FighterInputContext.Get(), MissionMenuContext.Get()})
    {
        if (!Context) continue;
        for (const auto& Mapping : Context->GetMappings())
            if (Mapping.Action && Mapping.Action->GetName() == TEXT("IA_QuantumMenu"))
            {
                QuantumMenuAction = const_cast<UInputAction*>(Mapping.Action.Get());
                bQuantumMapped = true;
            }
    }
    if (!QuantumMenuAction) { QuantumMenuAction = NewObject<UInputAction>(this); QuantumMenuAction->ValueType = EInputActionValueType::Boolean; }
    if (QuantumMenuAction->ValueType == EInputActionValueType::Boolean)
    {
        if (!bQuantumMapped) MissionTargetContext->MapKey(QuantumMenuAction.Get(), EKeys::Q);
        MissionMenuInput->BindAction(QuantumMenuAction.Get(), ETriggerEvent::Started, this, &UMissionBriefingDlg::ToggleQuantumTravel);
    }
    StarshatterInputBindings::RegisterContext(MissionTargetContext.Get());
    // Mission exit is a fixed Escape shortcut, outside the remappable bindings.
    auto* MissionExit = NewObject<UInputAction>(MissionTargetContext.Get(), TEXT("MissionExit"), RF_Transient);
    MissionExit->ValueType = EInputActionValueType::Boolean;
    MissionTargetContext->MapKey(MissionExit, EKeys::Escape);
    MissionMenuInput->BindAction(MissionExit, ETriggerEvent::Started, this, &UMissionBriefingDlg::ToggleMissionMenu);
    Input->AddMappingContext(MissionTargetContext.Get(), 1001);
    UE_LOG(LogTemp, Log, TEXT("[MissionZoom] MouseWheelAxis bound in mission context (priority 1001)."));

    bAddedMissionMenuContext = !Input->HasMappingContext(MissionMenuContext.Get());
    if (bAddedMissionMenuContext) Input->AddMappingContext(MissionMenuContext.Get(), 1000);
    BindMissionCameraInput();
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
    StartLiveSessionUpdates();
    return true;
}
void UMissionBriefingDlg::DisableMissionMenuInput()
{
    if (FlightPanelToolbar && GetWorld() && GetWorld()->GetGameViewport())
        GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(FlightPanelToolbar.ToSharedRef());
    FlightPanelToolbar.Reset();
    if (IsValid(MissionPreviewCamera))
        MissionPreviewCamera->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    // Stop the timer before resetting the rig; otherwise it recreates a chase
    // camera over the parked ship while the debrief is being presented.
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(MissionTargetCameraTimer);
    if (MissionTargetOverlay.IsValid() && GetWorld() && GetWorld()->GetGameViewport())
        GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(MissionTargetOverlay.ToSharedRef());
    MissionTargetOverlay.Reset();
    CloseQuantumTravel();
    QuantumMenuAction = nullptr;
    CloseNavigationPopup();
    NavMapAction=nullptr;
    CloseFlightOpsPopup();
    CloseIntelPopup();
    CloseMissionPopup();
    CloseObjectivesPopup();
    ObjectivesPanelAction=nullptr;
    CloseFlightReference();
    CloseCargoManifest();
    CloseMarketplace();
    CloseProduction();
    CloseStationServices();
    CloseMiningDrones();
    CloseShipStore();
    CloseAgents();
    ClosePlayers();
    CloseInsurance();
    CloseHangar();
    CloseSkills();
    CloseContracts();
    CloseLiveOptions();
    CloseContactsPopup();
    CloseWeaponsPopup();
    CloseEngineering();
    FlightPanelCanvas.Reset();
    EngineeringAction = nullptr;
    if (MissionCameraRig) MissionCameraRig->Reset(GetOwningPlayer());
    ClearMissionRotationInput();
    if (FighterHUDDetails.IsValid() && GetWorld() && GetWorld()->GetGameViewport())
        GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(FighterHUDDetails.ToSharedRef());
    FighterHUDDetails.Reset();
    WeaponsPanelAction = nullptr;
    HUDWarningsAction = nullptr;
    HUDInformationAction = nullptr;
    FighterMFDLeftAction = nullptr;
    FighterMFDRightAction = nullptr;
    FighterWeaponCycleAction = nullptr;
    if (FighterHUDPanels.IsValid() && GetWorld() && GetWorld()->GetGameViewport())
        GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(FighterHUDPanels.ToSharedRef());
    FighterHUDPanels.Reset();
    FighterHUDToggleAction = nullptr;
    bFighterHUDVisible = true;
    if (MissionQuitMenu) { delete MissionQuitMenu; MissionQuitMenu = nullptr; }
    if (APlayerController* PC = GetOwningPlayer())
    {
        if (MissionMenuInput) PC->PopInputComponent(MissionMenuInput.Get());
        if (ULocalPlayer* LP = PC->GetLocalPlayer())
            if (auto* Input = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
            {
                if (AutoNavInputContext) Input->RemoveMappingContext(AutoNavInputContext.Get());
                if (MissionCameraContext) Input->RemoveMappingContext(MissionCameraContext.Get());
                if (RadioChoiceContext) Input->RemoveMappingContext(RadioChoiceContext.Get());
                if (MissionTargetContext) Input->RemoveMappingContext(MissionTargetContext.Get());
                if (bAddedFighterInputContext && FighterInputContext)
                    Input->RemoveMappingContext(FighterInputContext.Get());
                if (bAddedMissionMenuContext && MissionMenuContext)
                    Input->RemoveMappingContext(MissionMenuContext.Get());
            }
    }
    if (MissionMenuInput) MissionMenuInput->DestroyComponent();
    MissionWeaponActions.Reset();
    AutoNavInputContext = nullptr;
    bPreviousTargetHeld = false;
    RadioChoiceActions.Reset(); RadioChoiceContext = nullptr; RadioMenuAction = nullptr;
    FlightOpsPanelAction = nullptr;
    GearToggleAction = nullptr;
    MissionThrottleAction = nullptr;
    MissionCameraViewAction = nullptr;
    MissionCameraContext = nullptr;
    MissionCameraActions.Reset();
    MissionCameraRig.Reset();
    MissionStrafeAction = nullptr;
    MissionForwardThrustAction = nullptr;
    MissionVerticalThrustAction = nullptr;
    MissionAugmenterAction = nullptr;
    MissionShieldsUpAction = nullptr;
    MissionShieldsDownAction = nullptr;
    MissionShieldsFullAction = nullptr;
    MissionShieldsZeroAction = nullptr;
    USSWMouseFlightModifier::bMouseSteeringEnabled = false;
    MissionPitchAction = nullptr;
    MissionYawAction = nullptr;
    MissionRollAction = nullptr;
    MissionThrottleZeroAction = nullptr;
    MissionThrottleFullAction = nullptr;
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
    // Do not also advance when the user invokes Shift+T for reverse targeting.
    APlayerController* PC = GetOwningPlayer();
    if (PC && StarshatterInputBindings::IsModifierDown(PC, TEXT("Shift")))
    {
        HUDView::SetDefaultColorSet((FMath::Clamp(HUDView::DefaultColorSet(), 0, 3) + 1) % 4);
        if (HUDView* Hud = HUDView::GetInstance()) Hud->SetHUDColorSet(HUDView::DefaultColorSet());
        return;
    }
    if (!bLiveMissionStarted || MissionSceneCover.IsValid() || ReferencePopup || LiveOptionsPopup || CargoManifestPopup || MarketplacePopup || ProductionPopup || StationServicesPopup || ShipStorePopup || AgentsPopup || PlayersPopup || AwardsPopup || WalletPopup || GuildsPopup || MiningDronesPopup || SkillsPopup || HangarPopup || InsurancePopup || LoadoutsPopup || FittingPopup || ContractsPopup ||
        EngineeringPopup.IsValid() || WeaponsPopup.IsValid() || IntelPopup.IsValid() || MissionPopup.IsValid() || ObjectivesPopup.IsValid() ||
        FlightOpsPopup.IsValid() || NavigationPopup.IsValid() ||
        (MissionQuitMenu && MissionQuitMenu->IsMenuShown()) || !GetWorld() || GetWorld()->IsPaused())
        return;

    Sim* Simulation = Sim::GetSim();
    Ship* Player = Simulation ? Simulation->GetPlayerShip() : nullptr;
    if (!Player || Player->IsDead() || Player->IsDying() || !Player->GetSensor()) return;

    SimElement* AssignedTargetGroup = FindTargetPracticeWave(Simulation);
    const Mission* CurrentMission = Simulation->GetMission();
    if (CurrentMission && CurrentMission->GetIdentity() == 102 &&
        CurrentMission->GetMissionType() == int(EMissionType::TRAINING))
    {
        // Between waves, select the route marker, which is not a sensor contact.
        if (!AssignedTargetGroup)
        {
            for (AActor* Candidate : GetSelectableMissionContacts())
            {
                const AWaypointActor* Waypoint = Cast<AWaypointActor>(Candidate);
                if (Waypoint && Waypoint->WaypointIndex > 0)
                {
                    SelectMissionContact(Candidate);
                    return;
                }
            }
        }
    }

    const bool bGroundStrikeTraining = CurrentMission && CurrentMission->GetIdentity() == 104 &&
        CurrentMission->GetMissionType() == int(EMissionType::TRAINING);
    if (bGroundStrikeTraining)
        AssignedTargetGroup = Simulation->FindElement("Cooling Tower");

    SimObject* Target = Player->GetSensor()->LockTarget(ESimObject::SHIP, AssignedTargetGroup != nullptr, true, AssignedTargetGroup);
    Player->SetTarget(Target);
    SelectedMissionTarget.Reset();
    SelectedMissionTargetName = FText::GetEmpty();
    if (AssignedTargetGroup && !bGroundStrikeTraining)
    {
        UE_LOG(LogTemp, Display, TEXT("[T2TargetCycle] Group='%hs' Lock='%hs'"),
            AssignedTargetGroup->GetName().data(), Target ? Target->GetName() : "none");
        if (!Target)
            RadioView::Message("No assigned drone available in this sector.");
    }
    if (!Target) return;

    for (TActorIterator<AShipActor> It(GetWorld()); It; ++It)
    {
        if (It->GetRuntimeShip() == Target)
        {
            SelectMissionContact(*It);
            break;
        }
    }
}
void UMissionBriefingDlg::CycleMissionTarget(int32 Direction)
{
    if (!bLiveMissionStarted || MissionSceneCover.IsValid() || StationServicesPopup.IsValid() || EngineeringPopup.IsValid() || WeaponsPopup.IsValid() || IntelPopup.IsValid() || MissionPopup.IsValid() || ObjectivesPopup.IsValid() || FlightOpsPopup.IsValid() || NavigationPopup.IsValid() ||
        (MissionQuitMenu && MissionQuitMenu->IsMenuShown()) || !IsValid(MissionPreviewCamera)) return;
    Sim* Simulation = Sim::GetSim();
    Ship* Player = Simulation ? Simulation->GetPlayerShip() : nullptr;
    if (!Player) return;
    TArray<AActor*> Candidates = GetSelectableMissionContacts();
    if (Candidates.IsEmpty())
    {
        SelectedMissionTarget.Reset();
        Player->DropTarget();
        return;
    }
    const int32 Current = Candidates.IndexOfByPredicate([this](AActor* Actor) { return Actor == SelectedMissionTarget.Get(); });
    const int32 Index = Current == INDEX_NONE ? (Direction > 0 ? 0 : Candidates.Num()-1)
        : (Current + (Direction > 0 ? 1 : -1) + Candidates.Num()) % Candidates.Num();
    AActor* Selected = Candidates[Index];
    SelectMissionContact(Selected);
}

TArray<AActor*> UMissionBriefingDlg::GetSelectableMissionContacts() const
{
    Sim* Simulation = Sim::GetSim();
    SimRegion* Region = Simulation ? Simulation->GetActiveRegion() : nullptr;
    if (!Region) return {};
    Ship* Player = Simulation->GetPlayerShip();
    if (!Player || !Region->GetShips().contains(Player)) return {};
    TArray<AActor*> Candidates;
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
        const bool bEligible = bInRegion && ShipData != Player && !bDead && !bDestroying && !bHidden;
        if (bEligible) Candidates.Add(Actor);
    }
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        AActor* Object = *It;
        if (!IsValid(Object) || !Object->ActorHasTag(TEXT("MissionNavigationTarget"))) continue;
        FMissionObjectData Data;
        if (ReadMissionObject(Object, Player, Region, Data)) Candidates.AddUnique(Object);
    }
    Candidates.Sort([](const AActor& A, const AActor& B) { return A.GetUniqueID() < B.GetUniqueID(); });
    return Candidates;
}

void UMissionBriefingDlg::SelectMissionContact(AActor* Selected)
{
    if (!IsValid(Selected) || !GetWorld() || !GetWorld()->GetGameViewport() ||
        !GetSelectableMissionContacts().Contains(Selected)) return;
    Sim* Simulation = Sim::GetSim();
    Ship* Player = Simulation ? Simulation->GetPlayerShip() : nullptr;
    SimRegion* Region = Simulation ? Simulation->GetActiveRegion() : nullptr;
    if (!Player || !Region) return;
    SelectedMissionTarget = Selected;
    if (bMissionTargetInspection) MissionTargetZoomDistance = 1.0;
    SelectedMissionTargetLocalBounds = MissionTargetLocalBounds(Selected);
    AShipActor* ShipVisual = Cast<AShipActor>(Selected);
    Ship* SelectedShip = ShipVisual ? ShipVisual->GetRuntimeShip() : nullptr;
    // Use SSW sensor/EMCON gating and propagate the lock to weapons and mission events.
    if (SelectedShip)
    {
        if (Player->GetTarget() != SelectedShip) Player->LockTarget(SelectedShip);
    }
    else Player->DropTarget(); // Navigation selection is not a weapons lock.
    FMissionObjectData ObjectData;
    ReadMissionObject(Selected, Player, Region, ObjectData);
    const FString FullName = SelectedShip ? SelectedShip->GetContactLabel() : ObjectData.Name;
    SelectedMissionTargetName = FText::FromString(FullName);
    if (!MissionTargetOverlay.IsValid())
    {
        TWeakObjectPtr<UMissionBriefingDlg> WeakThis(this);
        MissionTargetOverlay = SNew(SMissionTargetOverlay).Controller(GetOwningPlayer())
            .RangeKm_Lambda([WeakThis]() {
                if (!WeakThis.IsValid()) return -1.0;
                Sim* S = Sim::GetSim();
                Ship* P = S ? S->GetPlayerShip() : nullptr;
                SimRegion* R = S ? S->GetActiveRegion() : nullptr;
                AActor* A = WeakThis->SelectedMissionTarget.Get();
                if (!P || !R || !R->GetShips().contains(P) || !IsValid(A)) return -1.0;
                if (auto* Visual = Cast<AShipActor>(A)) {
                    Ship* T = Visual->GetRuntimeShip();
                    if (T && R->GetShips().contains(T)) return FVector::Distance(P->GetLocation(),T->GetLocation())/1000.0;
                }
                FMissionObjectData Data;
                return ReadMissionObject(A,P,R,Data) ? Data.RangeKm : -1.0;
            })
            .Target_Lambda([WeakThis]() { return WeakThis.IsValid() ? WeakThis->SelectedMissionTarget : TWeakObjectPtr<AActor>(); })
            .LocalBounds_Lambda([WeakThis]() { return WeakThis.IsValid() ? WeakThis->SelectedMissionTargetLocalBounds : FBox(ForceInit); })
            .TargetColor_Lambda([WeakThis]()
            {
                const FLinearColor Neutral = FLinearColor::Yellow;
                if (!WeakThis.IsValid()) return Neutral;
                if (const auto* Waypoint = Cast<AWaypointActor>(WeakThis->SelectedMissionTarget.Get()))
                    return Waypoint->bAsteroidFieldContact ? FLinearColor::Yellow : FLinearColor(.25f,1.f,.5f);
                AShipActor* Actor = Cast<AShipActor>(WeakThis->SelectedMissionTarget.Get());
                Sim* CurrentSim = Sim::GetSim();
                SimRegion* Region = CurrentSim ? CurrentSim->GetActiveRegion() : nullptr;
                Ship* Data = IsValid(Actor) ? Actor->GetRuntimeShip() : nullptr;
                if (!Data || !Region || !Region->GetShips().contains(Data)) return Neutral;
                Ship* Observer = CurrentSim->GetPlayerShip();
                if (!Observer || !Region->GetShips().contains(Observer)) return Neutral;
                const SimContact* Contact = Observer->FindContact(Data);
                const int32 IFF = Contact ? Contact->GetIFF(Observer) : Data->GetIFF();
                return SFighterHUDDetails::IdentificationColor(IFF,Observer->GetIFF());
            })
            .TargetName_Lambda([WeakThis]()
            {
                if (!WeakThis.IsValid()) return FText::GetEmpty();
                Sim* Current=Sim::GetSim();
                SimRegion* Active=Current?Current->GetActiveRegion():nullptr;
                Ship* Pilot=Current?Current->GetPlayerShip():nullptr;
                AShipActor* SelectedActor=Cast<AShipActor>(WeakThis->SelectedMissionTarget.Get());
                if (!SelectedActor) return WeakThis->SelectedMissionTargetName;
                Ship* TargetData=IsValid(SelectedActor)?SelectedActor->GetRuntimeShip():nullptr;
                const bool Locked=Pilot && Active && Active->GetShips().contains(Pilot) && TargetData &&
                    Active->GetShips().contains(TargetData) && Pilot->GetTarget()==TargetData;
                return FText::FromString(WeakThis->SelectedMissionTargetName.ToString() + (Locked?TEXT(""):TEXT(" [NO LOCK]")));
            });
        GetWorld()->GetGameViewport()->AddViewportWidgetContent(MissionTargetOverlay.ToSharedRef(), 900);
    }
    MissionTargetLastUpdate = GetWorld()->GetTimeSeconds();
    FTimerManagerTimerParameters TimerParameters;
    TimerParameters.bLoop = true;
    TimerParameters.bMaxOncePerFrame = true;
    GetWorld()->GetTimerManager().SetTimer(MissionTargetCameraTimer, this,
        &UMissionBriefingDlg::UpdateMissionTargetCamera, 0.000001f, TimerParameters);
}
bool UMissionBriefingDlg::CanUseFighterHUD() const
{
    return bLiveMissionStarted && !bMissionCompleteHandedOff
        && !(Sim::GetSim() && Sim::GetSim()->HasPlayerDocked()) && !QuantumPopup.IsValid() && !TravelTransition.IsValid() && !MissionSceneCover.IsValid() && !EngineeringPopup.IsValid() && !WeaponsPopup.IsValid() && !IntelPopup.IsValid() && !MissionPopup.IsValid() && !ObjectivesPopup.IsValid() && !FlightOpsPopup.IsValid() && !NavigationPopup.IsValid() && !ReferencePopup && !LiveOptionsPopup && !CargoManifestPopup && !MarketplacePopup && !ProductionPopup && !StationServicesPopup && !ShipStorePopup && !AgentsPopup && !PlayersPopup && !AwardsPopup && !WalletPopup && !GuildsPopup && !MiningDronesPopup && !HangarPopup && !InsurancePopup && !LoadoutsPopup && !FittingPopup && !ContractsPopup &&
        !(MissionQuitMenu && MissionQuitMenu->IsMenuShown());
}

bool UMissionBriefingDlg::CanShowFighterHUD() const
{
    return CanOpenFlightPanel();
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
    if (Game::IsTimeCompressed()) return;
    if (!CanUseFighterHUD() || !FighterHUDDetails.IsValid()) return;
    APlayerController* PC = GetOwningPlayer();
    const bool Primary = PC && (StarshatterInputBindings::IsModifierDown(PC,TEXT("Shift")));
    FighterHUDDetails->CycleWeapon(Primary);
}

void UMissionBriefingDlg::ToggleFighterWeaponsPanel()
{
    if (WeaponsPopup) { CloseWeaponsPopup(); return; }
    if (!CanOpenFlightPanel() || !FlightPanelCanvas) return;
    if (Game::IsTimeCompressed()) return;
    APlayerController* PC=GetOwningPlayer();
    if (!PC) return;
    ClearMissionRotationInput();
    if (MissionCameraRig) MissionCameraRig->LookX=MissionCameraRig->LookY=MissionCameraRig->RangeInput=0;
    UClass* PanelClass=LoadClass<UWeaponsDlg>(nullptr,TEXT("/Game/Screens/inGame/WBP_Weapons.WBP_Weapons_C"));
    WeaponsPanelWidget=CreateWidget<UWeaponsDlg>(PC,PanelClass?PanelClass:UWeaponsDlg::StaticClass());
    if(!WeaponsPanelWidget)return;
    WeaponsPanelWidget->OnPanelClosed=FSimpleDelegate::CreateWeakLambda(this,[this](){CloseWeaponsPopup();});
    WeaponsPanelWidget->OnExitMenuRequested = FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::ToggleMissionMenu);
    WeaponsPanelWidget->SetVisibility(ESlateVisibility::Visible);
    AddFloatingPanel(WeaponsPopup, WeaponsPanelWidget->TakeWidget(), TEXT("Weapons"), 1, FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::CloseWeaponsPopup));
}

void UMissionBriefingDlg::CloseWeaponsPopup()
{
    if (!WeaponsPopup) return;
    RemoveFloatingPanel(WeaponsPopup);
    if (WeaponsPanelWidget) WeaponsPanelWidget->RemoveFromParent();
    WeaponsPanelWidget = nullptr;
}


void UMissionBriefingDlg::ToggleFighterCautionPanel()
{
    if (CanUseFighterHUD() && FighterHUDDetails.IsValid()) FighterHUDDetails->ToggleCautionPanel();
}

void UMissionBriefingDlg::ToggleFighterHUD()
{
    if (!bLiveMissionStarted || MissionSceneCover.IsValid() || StationServicesPopup.IsValid() || EngineeringPopup.IsValid() || WeaponsPopup.IsValid() || IntelPopup.IsValid() || MissionPopup.IsValid() || ObjectivesPopup.IsValid() || FlightOpsPopup.IsValid() || NavigationPopup.IsValid() ||
        (MissionQuitMenu && MissionQuitMenu->IsMenuShown())) return;
    APlayerController* PC = GetOwningPlayer();
    if (PC && StarshatterInputBindings::IsModifierDown(PC, TEXT("Shift")))
    {
        HUDView::SetDefaultColorSet((FMath::Clamp(HUDView::DefaultColorSet(), 0, 3) + 1) % 4);
        if (HUDView* Hud = HUDView::GetInstance()) Hud->SetHUDColorSet(HUDView::DefaultColorSet());
        return;
    }
    bFighterHUDVisible = FighterHUDDetails.IsValid() && FighterHUDDetails->CycleFlightHUDMode(bFighterHUDVisible);
    if (!bFighterHUDVisible && FighterHUDDetails.IsValid())
    {
        FighterHUDDetails->CloseRadio();
        UpdateFighterRadioInput();
    }
}

void UMissionBriefingDlg::OnTargetZoom(const FInputActionValue& Value)
{
    if (!bLiveMissionStarted ||
        !IsValid(MissionPreviewCamera) || MissionSceneCover.IsValid() ||
        (MissionQuitMenu && MissionQuitMenu->IsMenuShown())) return;
    const float Wheel = Value.Get<float>();
    if (!FMath::IsFinite(Wheel) || FMath::IsNearlyZero(Wheel)) return;
    // Positive wheel input moves closer; the existing camera timer smooths motion.
    MissionTargetZoomDistance = FMath::Clamp(
        MissionTargetZoomDistance * FMath::Pow(0.85, double(FMath::Clamp(Wheel, -10.0f, 10.0f))),
        0.5, 3.0);
}

void UMissionBriefingDlg::ToggleMissionMenu()
{
    CloseIntelPopup();
    CloseMissionPopup();
    CloseFlightReference();
    CloseCargoManifest();
    CloseMarketplace();
    CloseProduction();
    CloseStationServices();
    CloseMiningDrones();
    CloseShipStore();
    CloseAgents();
    ClosePlayers();
    CloseInsurance();
    CloseHangar();
    CloseSkills();
    CloseContracts();
    CloseLiveOptions();
    CloseContactsPopup();
    if (QuantumPopup.IsValid()) { CloseQuantumTravel(); }
    if(NavigationPopup.IsValid()){CloseNavigationPopup();}
    if(FlightOpsPopup.IsValid()){CloseFlightOpsPopup();}
    if(ObjectivesPopup.IsValid()){CloseObjectivesPopup();}
    if (WeaponsPopup.IsValid()) { CloseWeaponsPopup(); }
    if (EngineeringPopup.IsValid()) { CloseEngineering(); }
    ClearMissionRotationInput();
    if (MissionCameraRig) MissionCameraRig->LookX = MissionCameraRig->LookY = MissionCameraRig->RangeInput = 0.0;
    if (FighterHUDDetails.IsValid()) FighterHUDDetails->CloseRadio();
    UpdateFighterRadioInput();
    if (!MissionQuitMenu || !MissionSystemLevel || MissionSceneCover.IsValid()) return;
    if (MissionQuitMenu->IsMenuShown())
    {
        MissionQuitMenu->CloseMenu();
        HandleMissionMenuAction(QuitView::Resume);
    }
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
    if (Action == QuitView::Resume)
    {
        if (APlayerController* PC = GetOwningPlayer())
        {
            FInputModeGameAndUI Mode;
            Mode.SetHideCursorDuringCapture(false);
            PC->SetInputMode(Mode);
        }
        return;
    }
    if (Action == QuitView::Controls)
    {
        ToggleLiveOptions();
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
        const bool bFreeFlight = Self->MissionPtr && Self->MissionPtr->IsFreeFlight();
        if (Action == QuitView::Accept && !bFreeFlight && Self->Manager && Self->Manager->GetDebriefDlg())
        {
            Self->CloseEmptySectorPreview(true);
            if (Runtime) Runtime->SetGameMode(EGameMode::PLAN);
            Self->Manager->ShowDebriefDlg(Self->CampaignPtr && Self->CampaignPtr->IsTraining() ? Self->CampaignPtr : nullptr);
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
        if (Action == QuitView::Accept && !bFreeFlight)
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
        // A standalone mission leaves the active story campaign untouched.
        if ((Action == QuitView::Abort || Action == QuitView::MainMenu) && !Self->bStandaloneMission)
            if (Campaign* Camp = Campaign::GetCampaign())
                if (Camp->GetCampaignId() < Campaign::SINGLE_MISSIONS) Camp->RollbackMission();
        if (Runtime) Runtime->SetGameMode(EGameMode::CMPN);
        if (Action == QuitView::MainMenu)
        {
            Self->ClearStandaloneMission();
            if (Self->manager) Self->manager->ShowMenuDlg();
        }
        else if (Self->bStandaloneMission)
        {
            Self->ClearStandaloneMission();
            if (Self->manager) Self->manager->ReturnFromMissionBriefing();
        }
        else if (Self->manager)
        {
            Self->manager->ShowOperationsMissionsDlg();
        }
    });
}

bool UMissionBriefingDlg::StartLiveMission()
{
    if (bLiveMissionStarted) return true;
    if (!MissionPtr || !MissionSystemLevel || !GetGameInstance())
    { UE_LOG(LogTemp, Error, TEXT("[LiveStart] Missing mission, streamed level or game instance")); return false; }
    auto* Runtime = GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>();
    auto* Environment = GetGameInstance()->GetSubsystem<UStarshatterEnvironmentSubsystem>();
    if (!Runtime || !Environment)
    { UE_LOG(LogTemp, Error, TEXT("[LiveStart] Missing runtime or environment subsystem")); return false; }
    ASystemSceneBuilder* Builder = nullptr;
    for (TActorIterator<ASystemSceneBuilder> It(GetWorld()); It; ++It)
        if (It->GetLevel() == MissionSystemLevel->GetLoadedLevel()) { Builder = *It; break; }
    if (!Builder || Builder->GetSpawnedRegions().IsEmpty())
    { UE_LOG(LogTemp, Error, TEXT("[LiveStart] Streamed system has no scene builder or regions: %hs"), MissionPtr->GetSystem()); return false; }
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
        Simulation->OnMissionUnloading.RemoveAll(this);
        Simulation->UnloadMission();
    }
    Environment->BuildSimRegionsForSim(Simulation);
    Runtime->BeginMissionPresentation(Builder);
    // Use the selected/generated campaign Mission with its current loadout and orders.
    MissionPtr->SetComplete(false);
    Simulation->OnMissionUnloading.RemoveAll(this);
    Simulation->OnMissionUnloading.AddUObject(this, &UMissionBriefingDlg::HandleSimulationUnloading);
    Simulation->LoadMission(MissionPtr, false);
    bTestAgentIntroduced = false;
    bPrimaryObjectivesAnnounced = false;
    bMissionCompleteHandedOff = false;
    Simulation->ExecMission();
    if (!Simulation->GetActiveRegion() || !Simulation->GetPlayerShip())
    {
        UE_LOG(LogTemp, Error, TEXT("[LiveStart] Mission %hs failed provisioning: region=%p player=%p"),
            MissionPtr->GetName(), Simulation->GetActiveRegion(), Simulation->GetPlayerShip());
        return false;
    }
    bLiveMissionStarted = true;
    if (!MissionPtr->IsFreeFlight() && CampaignPtr && (!bStandaloneMission || MissionPtr->IsStoryMission() || CampaignPtr->IsTraining()))
    {
        auto* Instance = Cast<USSWGameInstance>(GetGameInstance());
        const auto* Timer = GetGameInstance()->GetSubsystem<UTimerSubsystem>();
        if (Instance && !Instance->IsTrainingTestSession() && !TrainingReplay::IsActive(Instance,MissionPtr->GetIdentity()) && Timer && Instance->CampaignSave &&
            Instance->CampaignSave->CampaignIndex == CampaignPtr->GetCampaignId())
        {
            const uint64 AuthoredStart = static_cast<uint64>(FMath::Max(0, MissionPtr->GetStart()));
            Instance->CampaignSave->AdvanceCampaignClock(Timer->GetUniverseTimeSeconds(), AuthoredStart);
            Instance->SaveCampaign();
        }
    }
    Runtime->SetGameMode(EGameMode::PLAY);
    if (auto* Insurance = GetGameInstance()->GetSubsystem<UShipInsuranceSubsystem>()) Insurance->BeginMission();
    Runtime->SetPaused(true); // Remain behind the scene cover until camera warmup completes.
    UE_LOG(LogTemp, Log, TEXT("[MissionBriefing] Live mission started: %hs"), MissionPtr->GetName());
    return true;
}

void UMissionBriefingDlg::OnCommit()
{
    if (CampaignPtr && (!bStandaloneMission || CampaignPtr->IsTraining()) && MissionPtr &&
        !MissionPtr->IsFreeFlight() &&
        !CampaignPtr->IsMissionRankUnlocked(MissionPtr->GetIdentity()))
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 6.f, FColor::Yellow, CampaignPtr->MissionRankRequirement(MissionPtr->GetIdentity()));
        return;
    }
    // Ignore a repeated launch while this preview is loading or visible.
    if (MissionSystemLevel) return;
    UWorld* World = GetWorld();
    UGameViewportClient* Viewport = World ? World->GetGameViewport() : nullptr;
    if (!MissionPtr || !Viewport)
    {
        UE_LOG(LogTemp, Error, TEXT("[MissionBriefing] PROCEED blocked: no mission or game viewport. Mission=%p Viewport=%p"), MissionPtr, Viewport);
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Red,
            TEXT("MISSION PROCEED: Mission data or viewport unavailable. See Output Log."));
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
        UE_LOG(LogTemp, Error, TEXT("[MissionBriefing] PROCEED blocked: system map not found: %s (mission=%hs)"), *PackagePath, MissionPtr->GetName());
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 9.f, FColor::Red,
            FString::Printf(TEXT("MISSION PROCEED: Missing map %s"), *PackagePath));
        return; // Do not enter an empty mission when the authored map is absent.
    }


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
        UE_LOG(LogTemp, Error, TEXT("[MissionBriefing] Could not initialize mission input"));
        CloseEmptySectorPreview();
        return;
    }
    MissionSystemPackage = PackagePath;
    MissionSystemLevel->OnLevelShown.AddDynamic(
        this, &UMissionBriefingDlg::HandleMissionSystemLevelShown);
    MissionSystemLevel->SetShouldBeLoaded(true);
    MissionSystemLevel->SetShouldBeVisible(true);

    // Release the isolated ship preview before entering the live mission.
    if (MissionSituationPanel) MissionSituationPanel->SetShipPreviewActive(false);
    // Hide all menu-owned dialogs, including the briefing's outer host.
    if (manager) manager->Hide();
    if (Manager) Manager->Hide();
    Hide();

    // Stream the authored system map; do not explicitly start the mission simulation.
    // Retain starfield actors and restore visible scene objects on return.
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (MissionSystemLevel && MissionSceneIsolation::BelongsToLevel(Actor,MissionSystemLevel->GetLoadedLevel())) continue;
        if (MissionSceneIsolation::IsWorldSurface(Actor) ||
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

    if (APlayerController* PC = GetOwningPlayer())
    {
        FInputModeGameAndUI Input;
        Input.SetHideCursorDuringCapture(false);
        PC->SetInputMode(Input);
        PC->bShowMouseCursor = true;
    }
}
void UMissionBriefingDlg::OnCancel()
{
    if (MissionSituationPanel) MissionSituationPanel->SetShipPreviewActive(false);
    if (bStandaloneMission) ClearStandaloneMission();
    if (manager) manager->ReturnFromMissionBriefing();
    else Hide();
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
        Timer->OnUniverseSecond.RemoveAll(this);
        Timer->OnUniverseSecond.AddUObject(this, &UMissionBriefingDlg::HandleUniverseSecondTick);
        Timer->OnUniverseMinute.RemoveAll(this);
        Timer->OnUniverseMinute.AddUObject(this, &UMissionBriefingDlg::HandleUniverseMinuteTick);
        Timer->OnCampaignTPlusChanged.RemoveAll(this);
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
        if (!Input->HasMappingContext(RadioChoiceContext.Get())) { StarshatterInputBindings::RegisterContext(RadioChoiceContext.Get()); Input->AddMappingContext(RadioChoiceContext.Get(),1100); }
    }
    else Input->RemoveMappingContext(RadioChoiceContext.Get());
}
void UMissionBriefingDlg::ToggleFlightCameraPanel()
{
    if (!CanOpenCameraAndRadio() || !FighterHUDDetails || !MissionCameraRig) return;
    bFighterHUDVisible = true;
    FighterHUDDetails->ToggleCameraPanel(MissionCameraRig);
    if (FighterHUDDetails->IsCameraPanelOpen()) CloseContactsPopup();
    UpdateFighterRadioInput();
}

void UMissionBriefingDlg::ToggleFighterRadio()
{
    if (!CanOpenCameraAndRadio() || !FighterHUDDetails) return;
    if (LastRadioInputFrame == GFrameCounter) return;
    LastRadioInputFrame = GFrameCounter;
    bFighterHUDVisible=true;
    FighterHUDDetails->ToggleRadio();
    if (FighterHUDDetails->IsRadioOpen()) CloseContactsPopup();
    UpdateFighterRadioInput();
}

bool UMissionBriefingDlg::HandleContactsRadioKey(const FKey& Key)
{
    if (!RadioMenuAction || !AutoNavInputContext || !RadioMenuAction->Triggers.IsEmpty() ||
        !RadioMenuAction->Modifiers.IsEmpty()) return false;

    for (const FEnhancedActionKeyMapping& Mapping : AutoNavInputContext->GetMappings())
    {
        if (Mapping.Action.Get() == RadioMenuAction.Get() && Mapping.Key == Key &&
            Mapping.Triggers.IsEmpty() && Mapping.Modifiers.IsEmpty())
        {
            ToggleFighterRadio();
            return FighterHUDDetails.IsValid() && FighterHUDDetails->IsRadioOpen();
        }
    }
    return false;
}
void UMissionBriefingDlg::ToggleTravelSpeedMenu()
{
    RadioView::Message("Travel acceleration is disabled. Universe time is server controlled.");
}

void UMissionBriefingDlg::ResetTravelSpeed()
{
    if (auto* GI = GetGameInstance())
        if (auto* Runtime = GI->GetSubsystem<USSWRuntimeSubsystem>()) Runtime->SetTimeCompression(1);
    if (FighterHUDDetails.IsValid() && FighterHUDDetails->IsTravelSpeedOpen()) FighterHUDDetails->CloseRadio();
    UpdateFighterRadioInput();
}

void UMissionBriefingDlg::SelectFighterRadio(int32 Number)
{
    if (FighterHUDDetails.IsValid() && FighterHUDDetails->IsCameraPanelOpen())
    {
        if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
        if (const TOptional<int32> Command = FighterHUDDetails->SelectCameraPanel(Number))
            ExecuteMissionCameraCommand(*Command);
        UpdateFighterRadioInput();
        return;
    }
    if (FighterHUDDetails.IsValid() && FighterHUDDetails->IsTravelSpeedOpen())
    {
        constexpr uint32 Rates[] = {1, 2, 5, 10, 20};
        if (Number < 0 || Number > UE_ARRAY_COUNT(Rates)) return;
        if (Number > 0)
        {
            auto* GI = GetGameInstance();
            auto* Runtime = GI ? GI->GetSubsystem<USSWRuntimeSubsystem>() : nullptr;
            if (!Runtime) return;
            Sim* Simulation = Sim::GetSim();
            if (Number > 1 && USSWRuntimeSubsystem::HasTravelThreats(Simulation ? Simulation->GetPlayerShip() : nullptr))
            {
                Runtime->SetTimeCompression(1);
                return;
            }
            Runtime->SetTimeCompression(Rates[Number - 1]);
            if (Runtime->GetTimeCompression() != Rates[Number - 1])
            {
                RadioView::Message("Travel speed unavailable. Engage Auto Nav or Auto Track in clear cruise.");
                return;
            }
            ClearMissionRotationInput();
        }
        FighterHUDDetails->CloseRadio();
        UpdateFighterRadioInput();
        return;
    }
    if (CanOpenFlightPanel() && bFighterHUDVisible && FighterHUDDetails.IsValid()) FighterHUDDetails->RadioSelect(Number);
    UpdateFighterRadioInput();
}
void UMissionBriefingDlg::SelectFighterRadio0() { SelectFighterRadio(0); }
void UMissionBriefingDlg::SelectFighterRadio1() { SelectFighterRadio(1); }
void UMissionBriefingDlg::SelectFighterRadio2() { SelectFighterRadio(2); }
void UMissionBriefingDlg::SelectFighterRadio3() { SelectFighterRadio(3); }
void UMissionBriefingDlg::SelectFighterRadio4() { SelectFighterRadio(4); }
void UMissionBriefingDlg::SelectFighterRadio5() { SelectFighterRadio(5); }
void UMissionBriefingDlg::SelectFighterRadio6() { SelectFighterRadio(6); }
void UMissionBriefingDlg::SelectFighterRadio7() { SelectFighterRadio(7); }
void UMissionBriefingDlg::SelectFighterRadio8() { SelectFighterRadio(8); }
void UMissionBriefingDlg::SelectFighterRadio9() { SelectFighterRadio(9); }

void UMissionBriefingDlg::ToggleMissionGear()
{
    if (Game::IsTimeCompressed()) return;
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


void UMissionBriefingDlg::OnMissionThrottleStep(const FInputActionValue& Value)
{
    const float Axis = Value.Get<float>();
    if (!FMath::IsFinite(Axis) || FMath::Abs(Axis) < 0.1f) return;
    ApplyMissionThrottle(Axis > 0.0f ? 5.0 : -5.0, true);
}

void UMissionBriefingDlg::OnMissionThrottleZero()
{
    ApplyMissionThrottle(0.0, false);
}

void UMissionBriefingDlg::OnMissionThrottleFull()
{
    ApplyMissionThrottle(100.0, false);
}

void UMissionBriefingDlg::ApplyMissionThrottle(double Amount, bool bRelative)
{
    if (Game::IsTimeCompressed()) return;
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation = Sim::GetSim();
    Ship* PlayerShip = Simulation ? Simulation->GetPlayerShip() : nullptr;
    SimRegion* Region = Simulation ? Simulation->GetActiveRegion() : nullptr;
    if (!PlayerShip || !Region || !Region->GetShips().contains(PlayerShip) ||
        PlayerShip->IsDead() || PlayerShip->IsDying()) return;

    // Accumulate against the request so rapid presses survive engine spool-up.
    const double Requested = FMath::Clamp(
        bRelative ? PlayerShip->GetThrottleRequest() + Amount : Amount, 0.0, 100.0);
    PlayerShip->SetManualThrottle(Requested);
    if (!bRelative && Amount==0.0) PlayerShip->StopManualFlight();
    UE_LOG(LogTemp, Display, TEXT("[ThrottleInput] Requested=%.0f%% Actual=%.1f%%"),
        Requested, PlayerShip->GetThrottle());
}


void UMissionBriefingDlg::ToggleMouseSteering()
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    USSWMouseFlightModifier::bMouseSteeringEnabled = !USSWMouseFlightModifier::bMouseSteeringEnabled;
    if (!USSWMouseFlightModifier::bMouseSteeringEnabled) ClearMissionRotationInput();
    RadioView::Message(USSWMouseFlightModifier::bMouseSteeringEnabled ? "Mouse steering on" : "Mouse steering off");
}

void UMissionBriefingDlg::ApplyMissionRotation(int32 Axis, float Value)
{
    Sim* Simulation = Sim::GetSim();
    Ship* Player = Simulation ? Simulation->GetPlayerShip() : nullptr;
    SimRegion* Region = Simulation ? Simulation->GetActiveRegion() : nullptr;
    if (!Player || !Region || !Region->GetShips().contains(Player)) return;
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused())
    { Player->ClearManualRotationInput(); return; }
    APlayerController* PC = GetOwningPlayer();
    if (PC && (StarshatterInputBindings::IsModifierDown(PC,TEXT("Shift"))))
    {
        Player->SetManualRotationAxis(Axis, 0.0f);
        return;
    }
    Player->SetManualRotationAxis(Axis, Value);
}

void UMissionBriefingDlg::ClearMissionRotationInput()
{
    Sim* Simulation = Sim::GetSim();
    Ship* Player = Simulation ? Simulation->GetPlayerShip() : nullptr;
    SimRegion* Region = Simulation ? Simulation->GetActiveRegion() : nullptr;
    if (Player && Region && Region->GetShips().contains(Player)) Player->ClearManualRotationInput();
}

void UMissionBriefingDlg::OnMissionPitch(const FInputActionValue& Value) { ApplyMissionRotation(0, Value.Get<float>()); }
void UMissionBriefingDlg::ReleaseMissionPitch() { ApplyMissionRotation(0, 0.0f); }

void UMissionBriefingDlg::OnMissionYaw(const FInputActionValue& Value) { ApplyMissionRotation(1, Value.Get<float>()); }
void UMissionBriefingDlg::ReleaseMissionYaw() { ApplyMissionRotation(1, 0.0f); }

void UMissionBriefingDlg::OnMissionRoll(const FInputActionValue& Value) { ApplyMissionRotation(2, Value.Get<float>()); }
void UMissionBriefingDlg::ReleaseMissionRoll() { ApplyMissionRotation(2, 0.0f); }


void UMissionBriefingDlg::OnMissionShieldsUp() { ApplyMissionShields(1); }
void UMissionBriefingDlg::OnMissionShieldsDown() { ApplyMissionShields(-1); }
void UMissionBriefingDlg::OnMissionShieldsFull() { ApplyMissionShields(2); }
void UMissionBriefingDlg::OnMissionShieldsZero() { ApplyMissionShields(-2); }

void UMissionBriefingDlg::ApplyMissionShields(int32 Command)
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    // Plain S/X can also fire while the Shift chord is active; give full/zero priority.
    if (APlayerController* PC = GetOwningPlayer())
    {
        if (StarshatterInputBindings::IsModifierDown(PC,TEXT("Shift")))
        {
            if (Command == 1) Command = 2;
            if (Command == -1) Command = -2;
        }
    }
    Sim* Simulation = Sim::GetSim();
    Ship* Player = Simulation ? Simulation->GetPlayerShip() : nullptr;
    SimRegion* Region = Simulation ? Simulation->GetActiveRegion() : nullptr;
    if (!Player || !Region || !Region->GetShips().contains(Player) || Player->IsDead() || Player->IsDying()) return;
    Shield* Shields = Player->GetShield();
    if (!Shields) return;
    // Match legacy ShipCtrl: step from the current power level, not shield health.
    const double Level = Shields->GetPowerLevel();
    double Requested = 0.0;
    if (Command == 2) Requested = 100.0;
    else if (Command == -2) Requested = 0.0;
    else if (Command > 0)
        Requested = Level < 25.0 ? 25.0 : Level < 50.0 ? 50.0 : Level < 75.0 ? 75.0 : 100.0;
    else
        Requested = Level > 75.0 ? 75.0 : Level > 50.0 ? 50.0 : Level > 25.0 ? 25.0 : 0.0;
    Shields->SetPowerLevel(Requested);
    HUDSounds::PlaySound(HUDSounds::SND_SHIELD_LEVEL);
    UE_LOG(LogTemp, Display, TEXT("[ShieldInput] Requested=%.0f%% Current=%.1f%%"), Requested, Level);
}

void UMissionBriefingDlg::ApplyMissionTranslation(int32 Axis, float Value)
{
    Sim* Simulation=Sim::GetSim();
    Ship* Player=Simulation?Simulation->GetPlayerShip():nullptr;
    SimRegion* Region=Simulation?Simulation->GetActiveRegion():nullptr;
    if (!Player || !Region || !Region->GetShips().contains(Player)) return;
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused())
    { Player->ClearManualRotationInput(); return; }
    Player->SetManualTranslationAxis(Axis, Value);
}

void UMissionBriefingDlg::ApplyMissionAugmenter(bool Enabled)
{
    Sim* Simulation=Sim::GetSim();
    Ship* Player=Simulation?Simulation->GetPlayerShip():nullptr;
    SimRegion* Region=Simulation?Simulation->GetActiveRegion():nullptr;
    if (!Player || !Region || !Region->GetShips().contains(Player)) return;
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused())
    { Player->ClearManualRotationInput(); return; }
    const bool WasEnabled=Player->GetAugmenter();
    Player->SetManualAugmenter(Enabled);
    if(WasEnabled!=Player->GetAugmenter())
        UE_LOG(LogTemp,Display,TEXT("[AugmenterState] Ship='%hs' Held=%d Enabled=%d Throttle=%.1f Request=%.1f"),
            Player->GetName(),Enabled,Player->GetAugmenter(),Player->GetThrottle(),Player->GetThrottleRequest());
}

void UMissionBriefingDlg::OnMissionStrafe(const FInputActionValue& Value) { ApplyMissionTranslation(0,Value.Get<float>()); }
void UMissionBriefingDlg::ReleaseMissionStrafe() { ApplyMissionTranslation(0,0); }

void UMissionBriefingDlg::OnMissionForwardThrust(const FInputActionValue& Value) { ApplyMissionTranslation(1,Value.Get<float>()); }
void UMissionBriefingDlg::ReleaseMissionForwardThrust() { ApplyMissionTranslation(1,0); }

void UMissionBriefingDlg::OnMissionVerticalThrust(const FInputActionValue& Value) { ApplyMissionTranslation(2,Value.Get<float>()); }
void UMissionBriefingDlg::ReleaseMissionVerticalThrust() { ApplyMissionTranslation(2,0); }

void UMissionBriefingDlg::OnMissionAugmenter()
{
    APlayerController* PC = GetOwningPlayer();
    const bool Shift = PC && (StarshatterInputBindings::IsModifierDown(PC,TEXT("Shift")));
    ApplyMissionAugmenter(!Shift);
}
void UMissionBriefingDlg::ReleaseMissionAugmenter() { ApplyMissionAugmenter(false); }


void UMissionBriefingDlg::ToggleMissionCameraView()
{
    OnMissionCameraCommand(FInputActionValue(true), 7);
}

void UMissionBriefingDlg::UpdateMissionTargetCamera()
{
    if (bLiveMissionStarted && MissionPtr && MissionPtr->IsTestFlight() && !bTestAgentIntroduced &&
        !MissionSceneCover.IsValid() && CanOpenFlightPanel() && FlightPanelCanvas)
    {
        if (!AgentsPopup) ToggleAgents();
        bTestAgentIntroduced = AgentsPopup != nullptr;
        if (bTestAgentIntroduced) UE_LOG(LogTemp, Display, TEXT("[Agents] Janek instructor introduction displayed"));
    }

    UWorld* World = GetWorld();
    APlayerController* PC = GetOwningPlayer();
    if (!World || !PC || !bLiveMissionStarted || bMissionCompleteHandedOff
        || !IsValid(MissionPreviewCamera)) return;
    // Docking and floating panels must not freeze the camera at its first,
    // pre-synchronization pose. Keep following the rendered parked hull too.
    if (World->IsPaused()) return;
    Sim* Simulation = Sim::GetSim();
    SimRegion* Region = Simulation ? Simulation->GetActiveRegion() : nullptr;
    Ship* Player = Simulation ? Simulation->GetPlayerShip() : nullptr;
    if (!Player || !Region || !Region->GetShips().contains(Player) || Player->IsDead())
    {
        if (MissionCameraRig) MissionCameraRig->RestoreVisibility(PC);
        return;
    }
    SyncMissionNavigationTarget();
    AShipActor* Visual = Cast<AShipActor>(MissionPlayerCameraActor.Get());
    const double Now = World->GetTimeSeconds();
    if (!IsValid(Visual) || Visual->IsActorBeingDestroyed() || Visual->GetRuntimeShip()!=Player)
    {
        MissionPlayerCameraActor.Reset();
        if (Now < MissionPlayerCameraSearchTime) return;
        MissionPlayerCameraSearchTime = Now + 0.5;
        Visual = nullptr;
        for (TActorIterator<AShipActor> It(World); It; ++It)
        {
            if (!It->IsActorBeingDestroyed() && It->GetRuntimeShip()==Player)
            {
                Visual=*It;
                MissionPlayerCameraActor=Visual;
                // Hull/visual bounds helper excludes exhaust particle bounds.
                MissionPlayerLocalBounds=MissionTargetLocalBounds(Visual);
                MissionPlayerCameraDistance=0;
                break;
            }
        }
        if (!Visual) return;
    }
    const double Delta=FMath::Clamp(Now-MissionTargetLastUpdate,0.0,0.1);
    MissionTargetLastUpdate=Now;
    if (!MissionCameraRig) MissionCameraRig = MakeShared<FMissionCameraRig>();
    MissionCameraRig->Tick(PC, MissionPreviewCamera.Get(), Visual, Player, Region,
        Cast<AShipActor>(SelectedMissionTarget.Get()), Delta, MissionTargetZoomDistance);
}

void UMissionBriefingDlg::BindMissionCameraInput()
{
    APlayerController* PC = GetOwningPlayer();
    ULocalPlayer* LP = PC ? PC->GetLocalPlayer() : nullptr;
    auto* Input = LP ? LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
    if (!Input || !MissionMenuInput) return;
    MissionCameraRig = MakeShared<FMissionCameraRig>();
    MissionCameraContext = NewObject<UInputMappingContext>(this, NAME_None, RF_Transient);
    MissionCameraActions.Reset();
    UInputAction* Shift = NewObject<UInputAction>(this, NAME_None, RF_Transient);
    Shift->ValueType = EInputActionValueType::Boolean;
    Shift->bConsumeInput = false;
    MissionCameraActions.Add(Shift);
    MissionCameraContext->MapKey(Shift, EKeys::LeftShift);
    MissionCameraContext->MapKey(Shift, EKeys::RightShift);
    const TCHAR* Names[] = {
        TEXT("IA_CameraCockpit"), TEXT("IA_CameraVirtual"), TEXT("IA_CameraChase"),
        TEXT("IA_CameraDrop"), TEXT("IA_CameraOrbit"), TEXT("IA_CameraTarget"),
        TEXT("IA_CameraThreat"), TEXT("IA_ViewTarget"), TEXT("IA_CameraNextObject"),
        TEXT("IA_CameraWide"), TEXT("IA_CameraLookX"), TEXT("IA_CameraLookY"), TEXT("IA_CameraRange"),
        TEXT("SSW_CameraPanel")
    };
    const FKey Keys[] = {EKeys::F1, EKeys::F1, EKeys::F2, EKeys::F2, EKeys::F3,
        EKeys::F4, EKeys::Invalid, EKeys::V, EKeys::Invalid, EKeys::K,
        EKeys::Right, EKeys::Up, EKeys::Add, EKeys::Apostrophe};
    auto IsMapped = [this](UInputAction* Action)
    {
        const UInputMappingContext* Contexts[] = {MissionMenuContext.Get(), FighterInputContext.Get()};
        for (const UInputMappingContext* Context : Contexts)
            if (Context)
                for (const auto& Mapping : Context->GetMappings())
                    if (Mapping.Action == Action) return true;
        return false;
    };
    for (int32 Command = 0; Command < UE_ARRAY_COUNT(Names); ++Command)
    {
        const FString Path = FString::Printf(TEXT("/Game/Input/%s.%s"), Names[Command], Names[Command]);
        const bool bPanel = Command == FMissionCameraRig::PanelCommand;
        const bool bAxis = Command >= 10 && !bPanel;
        UInputAction* Action = bPanel ? nullptr : LoadObject<UInputAction>(nullptr, *Path);
        const EInputActionValueType Type = bAxis ? EInputActionValueType::Axis1D : EInputActionValueType::Boolean;
        if (Action && Action->ValueType != Type)
        {
            UE_LOG(LogTemp, Warning, TEXT("[MissionCamera] %s has the wrong value type; using the default binding."), Names[Command]);
            Action = nullptr;
        }
        if (!Action)
        {
            Action = NewObject<UInputAction>(MissionCameraContext.Get(), bPanel ? FName(Names[Command]) : NAME_None, RF_Transient);
            Action->ValueType = Type;
        }
        MissionCameraActions.Add(Action);
        if (!IsMapped(Action) && Keys[Command].IsValid())
        {
            auto Map = [this, Action, Shift, Command](FKey Key, bool bNegative)
            {
                auto& Mapping = MissionCameraContext->MapKey(Action, Key);
                if (Command == 1 || Command == 3 || Command == 8 || Command == 10 || Command == 11)
                {
                    auto* Chord = NewObject<UInputTriggerChordAction>(MissionCameraContext.Get());
                    Chord->ChordAction = Shift;
                    Mapping.Triggers.Add(Chord);
                }
                if (bNegative) Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(MissionCameraContext.Get()));
            };
            Map(Keys[Command], false);
            if (Command == 10) Map(EKeys::Left, true);
            if (Command == 11) Map(EKeys::Down, true);
            if (Command == 12) Map(EKeys::Subtract, true);
        }
        MissionMenuInput->BindAction(Action, bAxis ? ETriggerEvent::Triggered : ETriggerEvent::Started,
            this, &UMissionBriefingDlg::OnMissionCameraCommand, Command);
        if (bAxis)
        {
            MissionMenuInput->BindAction(Action, ETriggerEvent::Completed, this, &UMissionBriefingDlg::ReleaseMissionCameraCommand, Command);
            MissionMenuInput->BindAction(Action, ETriggerEvent::Canceled, this, &UMissionBriefingDlg::ReleaseMissionCameraCommand, Command);
        }
    }
    StarshatterInputBindings::RegisterContext(MissionCameraContext.Get()); Input->AddMappingContext(MissionCameraContext.Get(), 1002);
}

void UMissionBriefingDlg::ReleaseMissionCameraCommand(const FInputActionValue&, int32 Command)
{
    if (!MissionCameraRig) return;
    if (Command == 10) MissionCameraRig->LookX = 0.0;
    if (Command == 11) MissionCameraRig->LookY = 0.0;
    if (Command == 12) MissionCameraRig->RangeInput = 0.0;
}

void UMissionBriefingDlg::OnMissionCameraCommand(const FInputActionValue& Value, int32 Command)
{
    if (!CanUseFighterHUD() || !MissionCameraRig || !GetWorld() || GetWorld()->IsPaused()) return;
    APlayerController* PC = GetOwningPlayer();
    const bool Shift = PC && (StarshatterInputBindings::IsModifierDown(PC,TEXT("Shift")));
    // Guard the unmodified F1/F2 actions when a chord fires in another IMC.
    if ((Command == 0 || Command == 2) && Shift) return;
    if ((Command == 1 || Command == 3) && !Shift) return;
    if (Command == FMissionCameraRig::PanelCommand)
    {
        ToggleFlightCameraPanel();
        return;
    }
    if (Command >= 10)
    {
        const float Axis = Value.Get<float>();
        if (!FMath::IsFinite(Axis)) return;
        const double Amount = FMath::Clamp(double(Axis), -1.0, 1.0);
        if (Command == 10) MissionCameraRig->LookX = Amount;
        if (Command == 11) MissionCameraRig->LookY = Amount;
        if (Command == 12) MissionCameraRig->RangeInput = Amount;
        return;
    }
    ExecuteMissionCameraCommand(Command);
}

void UMissionBriefingDlg::ExecuteMissionCameraCommand(int32 Command)
{
    if (!MissionCameraRig) return;
    using Rig = FMissionCameraRig;
    const int32 LookIndex = Command - Rig::FirstLookCommand;
    if (LookIndex >= 0 && LookIndex < UE_ARRAY_COUNT(Rig::LookYaw))
    {
        MissionCameraRig->SetMode(Rig::Virtual);
        MissionCameraRig->HeadYaw = Rig::LookYaw[LookIndex];
        MissionCameraRig->HeadPitch = Rig::LookPitch[LookIndex];
        MissionCameraRig->LookX = MissionCameraRig->LookY = 0.0;
    }
    else
    switch (Command)
    {
    case 0: MissionCameraRig->SetMode(FMissionCameraRig::Cockpit); break;
    case 1: MissionCameraRig->SetMode(FMissionCameraRig::Virtual); break;
    case 2: MissionCameraRig->SetMode(FMissionCameraRig::Chase); break;
    case 3: MissionCameraRig->SetMode(FMissionCameraRig::Drop); break;
    case 4:
        MissionCameraRig->SetMode(FMissionCameraRig::Orbit);
        MissionCameraRig->ViewObject.Reset();
        break;
    case 5: MissionCameraRig->SetMode(FMissionCameraRig::Target); break;
    case 6: MissionCameraRig->SetMode(FMissionCameraRig::Threat); break;
    case 7:
        if (MissionCameraRig->Mode == FMissionCameraRig::Orbit &&
            MissionCameraRig->ViewObject.Get() == SelectedMissionTarget.Get())
            MissionCameraRig->SetMode(FMissionCameraRig::Chase);
        else if (AShipActor* Selected = Cast<AShipActor>(SelectedMissionTarget.Get()))
        {
            Sim* Simulation = Sim::GetSim();
            SimRegion* Region = Simulation ? Simulation->GetActiveRegion() : nullptr;
            Ship* Pilot = Simulation ? Simulation->GetPlayerShip() : nullptr;
            Ship* Data = Selected->GetRuntimeShip();
            if (!Region || !Pilot || !Region->GetShips().contains(Pilot) || !Data || !Region->GetShips().contains(Data)) return;
            SimContact* Contact = Pilot->FindContact(Data);
            if (Data->GetIFF() != Pilot->GetIFF() && (!Contact || !Contact->ActLock())) return;
            MissionCameraRig->SetMode(FMissionCameraRig::Orbit);
            MissionCameraRig->ViewObject = Selected;
        }
        break;
    case 8: CycleMissionViewObject(); break;
    case 9: MissionCameraRig->bWide = !MissionCameraRig->bWide; break;
    case Rig::OverviewCommand: MissionCameraRig->SetMode(Rig::Overview); break;
    default: return;
    }
    bMissionTargetInspection = false; // View-object and weapon-target selection are independent.
    MissionTargetZoomDistance = 1.0;
    UpdateMissionTargetCamera();
}

void UMissionBriefingDlg::CycleMissionViewObject()
{
    Sim* Simulation = Sim::GetSim();
    SimRegion* Region = Simulation ? Simulation->GetActiveRegion() : nullptr;
    Ship* Pilot = Simulation ? Simulation->GetPlayerShip() : nullptr;
    if (!MissionCameraRig || !Region || !Pilot || !Region->GetShips().contains(Pilot)) return;
    TArray<AShipActor*> Candidates;
    for (TActorIterator<AShipActor> It(GetWorld()); It; ++It)
    {
        Ship* Data = It->GetRuntimeShip();
        if (!Data || !Region->GetShips().contains(Data) || It->IsActorBeingDestroyed() || Data->IsDead() || Data->IsDying()) continue;
        SimContact* Contact = Pilot->FindContact(Data);
        if (Data == Pilot || (Contact && Contact->ActLock())) Candidates.Add(*It);
    }
    Candidates.Sort([](const AShipActor& A, const AShipActor& B) { return A.GetName() < B.GetName(); });
    if (Candidates.IsEmpty()) return;
    AShipActor* Current = MissionCameraRig->ViewObject.IsValid() ? MissionCameraRig->ViewObject.Get()
        : Cast<AShipActor>(MissionPlayerCameraActor.Get());
    const int32 Index = Candidates.IndexOfByKey(Current);
    MissionCameraRig->SetMode(FMissionCameraRig::Orbit);
    MissionCameraRig->ViewObject = Candidates[(Index + 1) % Candidates.Num()];
}

namespace
{
    template<typename WidgetType>
    UClass* ResolveFlightPopupClass(UGameInstance* Instance, FName AssetId)
    {
        if (auto* Registry = Instance ? Instance->GetSubsystem<UStarshatterAssetRegistrySubsystem>() : nullptr)
        {
            if (UClass* Registered = Registry->GetWidgetClass(AssetId).Get())
            {
                if (Registered->IsChildOf(WidgetType::StaticClass()) && !Registered->HasAnyClassFlags(CLASS_Abstract))
                    return Registered;

                UE_LOG(LogTemp, Warning, TEXT("Invalid popup class %s registered for %s"),
                    *GetNameSafe(Registered), *AssetId.ToString());
            }
        }
        return WidgetType::StaticClass();
    }
}

void UMissionBriefingDlg::ToggleFlightReference()
{
    if (ReferencePopup) { CloseFlightReference(); return; }
    if (!CanOpenFlightPanel() || !FlightPanelCanvas) return;

    ClearMissionRotationInput();
    FlightReferenceWidget = CreateWidget<UFlightTacticalReferenceDlg>(GetOwningPlayer(),
        ResolveFlightPopupClass<UFlightTacticalReferenceDlg>(GetGameInstance(), TEXT("UI.TacticalPopupClass")));
    if (!FlightReferenceWidget) return;
    AddFloatingPanel(ReferencePopup, FlightReferenceWidget->TakeWidget(), TEXT("Tactical Reference"), 1,
        FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::CloseFlightReference));
}

void UMissionBriefingDlg::CloseFlightReference()
{
    RemoveFloatingPanel(ReferencePopup);
    FlightReferenceWidget = nullptr;
}

void UMissionBriefingDlg::ToggleCargoManifest()
{
    if (CargoManifestPopup) { CloseCargoManifest(); return; }
    if (!CanOpenFlightPanel() || !FlightPanelCanvas) return;

    ClearMissionRotationInput();
    CargoManifestWidget = CreateWidget<UCargoManifestDlg>(GetOwningPlayer(),
        ResolveFlightPopupClass<UCargoManifestDlg>(GetGameInstance(), TEXT("UI.CargoManifestPopupClass")));
    if (!CargoManifestWidget) return;
    AddFloatingPanel(CargoManifestPopup, CargoManifestWidget->TakeWidget(), TEXT("Cargo Manifest"), 1,
        FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::CloseCargoManifest));
}

void UMissionBriefingDlg::CloseCargoManifest()
{
    RemoveFloatingPanel(CargoManifestPopup);
    CargoManifestWidget = nullptr;
}

bool UMissionBriefingDlg::CanOpenMarketplace() const
{
    if (!bLiveMissionStarted || bMissionCompleteHandedOff || !GetWorld() || GetWorld()->IsPaused() ||
        TravelTransition || MissionSceneCover || (MissionQuitMenu && MissionQuitMenu->IsMenuShown()))
        return false;
    Sim* Simulation = Sim::GetSim();
    const Ship* Pilot = Simulation ? Simulation->GetPlayerShip() : nullptr;
    return Pilot && !Pilot->IsDead() && Pilot->GetDock() && Pilot->GetCarrier() &&
        !Pilot->GetCarrier()->IsDead() && Pilot->GetFlightPhase() >= EOPSMode::DOCKED &&
        Pilot->GetFlightPhase() <= EOPSMode::LOCKED;
}

void UMissionBriefingDlg::ToggleMarketplace()
{
    if (MarketplacePopup) { CloseMarketplace(); return; }
    if (!CanOpenMarketplace() || !FlightPanelCanvas) return;

    ClearMissionRotationInput();
    MarketplaceWidget = CreateWidget<UMarketplaceDlg>(GetOwningPlayer(),
        ResolveFlightPopupClass<UMarketplaceDlg>(GetGameInstance(), TEXT("UI.MarketplacePopupClass")));
    if (!MarketplaceWidget) return;
    AddFloatingPanel(MarketplacePopup, MarketplaceWidget->TakeWidget(), TEXT("Marketplace"), 1,
        FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::CloseMarketplace), true);
}

void UMissionBriefingDlg::CloseMarketplace()
{
    RemoveFloatingPanel(MarketplacePopup);
    MarketplaceWidget = nullptr;
}

FName UMissionBriefingDlg::ResolveStationServicesTarget() const
{
    Sim* Simulation = Sim::GetSim();
    const Ship* Pilot = Simulation ? Simulation->GetPlayerShip() : nullptr;
    SimRegion* Region = Simulation ? Simulation->GetActiveRegion() : nullptr;
    if (!Pilot) return NAME_None;
    const auto Eligible = [](const Ship* Base)
    {
        if (!Base || Base->IsDead() || Base->IsDying()) return false;
        const auto Kind = Base->GetClassification();
        return Kind == CLASSIFICATION::STATION || Kind == CLASSIFICATION::STARBASE || Kind == CLASSIFICATION::CARRIER;
    };
    if (Region)
    {
        const auto* Visual = Cast<AShipActor>(SelectedMissionTarget.Get());
        Ship* Selected = Visual ? Visual->GetRuntimeShip() : nullptr;
        if (Selected && Region->GetShips().contains(Selected) && Eligible(Selected))
            return FName(UTF8_TO_TCHAR(Selected->GetName()));
        for (ListIter<Ship> Ships = Region->GetShips(); ++Ships;)
            if (static_cast<SimObject*>(Ships.value()) == Pilot->GetTarget() && Eligible(Ships.value()))
                return FName(UTF8_TO_TCHAR(Ships.value()->GetName()));
    }
    const Ship* Dock = FStationServices::IsDocked() ? Pilot->GetCarrier() : nullptr;
    return Eligible(Dock) ? FName(UTF8_TO_TCHAR(Dock->GetName())) : NAME_None;
}

bool UMissionBriefingDlg::CanOpenStationServices() const
{
    if (!bLiveMissionStarted || bMissionCompleteHandedOff || !GetWorld() || GetWorld()->IsPaused() ||
        TravelTransition || MissionSceneCover || (MissionQuitMenu && MissionQuitMenu->IsMenuShown())) return false;
    Sim* Simulation = Sim::GetSim();
    const Ship* Pilot = Simulation ? Simulation->GetPlayerShip() : nullptr;
    return Pilot && !Pilot->IsDead() && !Pilot->IsDying() &&
        (StationServicesPopup.IsValid() || !ResolveStationServicesTarget().IsNone());
}

bool UMissionBriefingDlg::CanUseStationServices() const
{
    if (!CanOpenStationServices() || !FStationServices::IsDocked()) return false;
    const FName Viewed = StationServicesPopup.IsValid() ? StationServicesFacility : ResolveStationServicesTarget();
    return StationServicesAI::Context(Viewed).bDocked;
}

void UMissionBriefingDlg::ToggleStationServices()
{
    if (StationServicesPopup) { CloseStationServices(); return; }
    if (!CanOpenStationServices() || !FlightPanelCanvas) return;
    StationServicesFacility = ResolveStationServicesTarget();
    if (StationServicesFacility.IsNone()) return;
    ClearMissionRotationInput();
    StationServicesWidget = CreateWidget<UStationServicesDlg>(GetOwningPlayer(),
        ResolveFlightPopupClass<UStationServicesDlg>(GetGameInstance(), TEXT("UI.StationServicesPopupClass")));
    if (!StationServicesWidget) return;
    StationServicesWidget->SetStation(StationServicesFacility);
    StationServicesWidget->OnOpenService = FOpenStationService::CreateUObject(this, &UMissionBriefingDlg::OpenStationService);
    AddFloatingPanel(StationServicesPopup, StationServicesWidget->TakeWidget(), TEXT("Station Services"), 1,
        FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::CloseStationServices));
}

void UMissionBriefingDlg::CloseStationServices()
{
    if (StationServicesWidget) StationServicesWidget->OnOpenService.Unbind();
    RemoveFloatingPanel(StationServicesPopup);
    StationServicesWidget = nullptr;
    StationServicesFacility = NAME_None;
}

void UMissionBriefingDlg::OpenStationService(ESSWStationService Service)
{
    if (!CanUseStationServices()) return;
    FText Unavailable;
    if (!StationServicesAI::CanUse(StationServicesAI::Context(StationServicesFacility), Service, Unavailable)) return;
    TGuardValue<bool> ServiceScope(bOpeningStationService, true);
    auto* Players = GetGameInstance() ? GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>() : nullptr;
    const FGuid AssignedHull = Players ? Players->GetPlayerInfo().AssignedHangarHull : FGuid();
    switch (Service)
    {
    case ESSWStationService::Cargo:
        if (!CargoManifestPopup) ToggleCargoManifest(); else UpdateFlightPanelInput(CargoManifestPopup);
        break;
    case ESSWStationService::Market:
        if (!MarketplacePopup) ToggleMarketplace(); else UpdateFlightPanelInput(MarketplacePopup);
        break;
    case ESSWStationService::Insurance:
        if (AssignedHull.IsValid()) OpenInsurance(AssignedHull);
        else if (!HangarPopup) ToggleHangar(); else UpdateFlightPanelInput(HangarPopup);
        break;
    case ESSWStationService::Loadout:
        if (AssignedHull.IsValid()) OpenHangarLoadout(AssignedHull);
        else if (!HangarPopup) ToggleHangar(); else UpdateFlightPanelInput(HangarPopup);
        break;
    case ESSWStationService::Refit:
        if (AssignedHull.IsValid()) OpenHangarFit(AssignedHull);
        else if (!HangarPopup) ToggleHangar(); else UpdateFlightPanelInput(HangarPopup);
        break;
    case ESSWStationService::Repair:
    case ESSWStationService::HullSelection:
        if (!HangarPopup) ToggleHangar(); else UpdateFlightPanelInput(HangarPopup);
        break;
    case ESSWStationService::Manufacturing:
    case ESSWStationService::Blueprints:
        if (!ProductionPopup) ToggleProduction(); else UpdateFlightPanelInput(ProductionPopup);
        if(Service==ESSWStationService::Blueprints && ProductionWidget)ProductionWidget->ShowBlueprints();
        break;
    case ESSWStationService::SkillBooks:
        if (!SkillsPopup) ToggleSkills(); else UpdateFlightPanelInput(SkillsPopup);
        break;
    }
}

void UMissionBriefingDlg::ToggleProduction()
{
    if (ProductionPopup) { CloseProduction(); return; }
    if (!CanOpenFlightPanel() || !FlightPanelCanvas) return;

    ClearMissionRotationInput();
    ProductionWidget = CreateWidget<UProductionDlg>(GetOwningPlayer(),
        ResolveFlightPopupClass<UProductionDlg>(GetGameInstance(), TEXT("UI.ProductionPopupClass")));
    if (!ProductionWidget) return;
    AddFloatingPanel(ProductionPopup, ProductionWidget->TakeWidget(), TEXT("Production Facilities"), 1,
        FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::CloseProduction), true);
}

void UMissionBriefingDlg::CloseProduction()
{
    RemoveFloatingPanel(ProductionPopup);
    ProductionWidget = nullptr;
}

void UMissionBriefingDlg::ToggleShipStore()
{
    if (ShipStorePopup) { CloseShipStore(); return; }
    if (!CanOpenFlightPanel() || !FlightPanelCanvas) return;

    ClearMissionRotationInput();
    ShipStoreWidget = CreateWidget<UShipStoreDlg>(GetOwningPlayer(),
        ResolveFlightPopupClass<UShipStoreDlg>(GetGameInstance(), TEXT("UI.ShipStorePopupClass")));
    if (!ShipStoreWidget) return;
    AddFloatingPanel(ShipStorePopup, ShipStoreWidget->TakeWidget(), TEXT("Ship Store"), 1,
        FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::CloseShipStore), true);
}

void UMissionBriefingDlg::CloseShipStore()
{
    RemoveFloatingPanel(ShipStorePopup);
    ShipStoreWidget = nullptr;
}

void UMissionBriefingDlg::TogglePlayers()
{
    if (PlayersPopup) { ClosePlayers(); return; }
    if (!CanOpenFlightPanel() || !FlightPanelCanvas) return;

    ClearMissionRotationInput();
    PlayersWidget = CreateWidget<UFlightLogbookDlg>(GetOwningPlayer(),
        ResolveFlightPopupClass<UFlightLogbookDlg>(GetGameInstance(), TEXT("UI.PlayersPopupClass")));
    if (PlayersWidget)
        {
            PlayersWidget->OnOpenShipSkins=FSimpleDelegate::CreateUObject(this,&UMissionBriefingDlg::ToggleShipSkins);
            PlayersWidget->OnOpenHangar = FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::ToggleHangar);
            PlayersWidget->OnOpenSkills = FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::ToggleSkills);
            PlayersWidget->OnOpenAwards = FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::ToggleAwards);
            PlayersWidget->OnOpenWallet = FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::ToggleWallet);
            PlayersWidget->OnOpenGuilds = FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::ToggleGuilds);
        }
    if (PlayersWidget)
        AddFloatingPanel(PlayersPopup, PlayersWidget->BuildPanelContent(GetGameInstance()), TEXT("Player Logbook"), 1,
            FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::ClosePlayers));
}

void UMissionBriefingDlg::ClosePlayers()
{
    CloseGuilds();
    CloseShipSkins();
    CloseWallet();
    CloseAwards();
    RemoveFloatingPanel(PlayersPopup);
    PlayersWidget = nullptr;
}

void UMissionBriefingDlg::ToggleAwards()
{
    if (AwardsPopup) { CloseAwards(); return; }
    if (!CanOpenFlightPanel() || !FlightPanelCanvas) return;
    ClearMissionRotationInput();
    AwardsWidget = CreateWidget<UAwardsDlg>(GetOwningPlayer(),
        ResolveFlightPopupClass<UAwardsDlg>(GetGameInstance(), TEXT("UI.AwardsPopupClass")));
    if (!AwardsWidget) return;
    AddFloatingPanel(AwardsPopup, AwardsWidget->TakeWidget(), TEXT("Awards"), 2,
        FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::CloseAwards));
}

void UMissionBriefingDlg::CloseAwards()
{
    RemoveFloatingPanel(AwardsPopup);
    AwardsWidget = nullptr;
}

void UMissionBriefingDlg::ToggleWallet()
{
    if (WalletPopup) { CloseWallet(); return; }
    if (!CanOpenFlightPanel() || !FlightPanelCanvas) return;
    ClearMissionRotationInput();
    TransactionWidget=CreateWidget<UTransactionDlg>(GetOwningPlayer(),
        ResolveFlightPopupClass<UTransactionDlg>(GetGameInstance(),TEXT("UI.TransactionPopupClass")));
    if(!TransactionWidget)return;
    TransactionWidget->OnOpenMarketplace=FSimpleDelegate::CreateWeakLambda(this,[this]()
        { if(!MarketplacePopup)ToggleMarketplace();else UpdateFlightPanelInput(MarketplacePopup); });
    TransactionWidget->OnOpenContracts=FSimpleDelegate::CreateWeakLambda(this,[this]()
        { if(!ContractsPopup)ToggleContracts();else UpdateFlightPanelInput(ContractsPopup); });
    TransactionWidget->OnOpenProduction=FSimpleDelegate::CreateWeakLambda(this,[this]()
        { if(!ProductionPopup)ToggleProduction();else UpdateFlightPanelInput(ProductionPopup); });
    TransactionWidget->OnOpenHangar=FSimpleDelegate::CreateWeakLambda(this,[this]()
        { if(!HangarPopup)ToggleHangar();else UpdateFlightPanelInput(HangarPopup); });
    AddFloatingPanel(WalletPopup,TransactionWidget->TakeWidget(),TEXT("Wallet"),2,
        FSimpleDelegate::CreateUObject(this,&UMissionBriefingDlg::CloseWallet),true);
}

void UMissionBriefingDlg::CloseWallet()
{
    RemoveFloatingPanel(WalletPopup);
    TransactionWidget=nullptr;
}

void UMissionBriefingDlg::ToggleMiningDrones()
{
    if (MiningDronesPopup) { CloseMiningDrones(); return; }
    if (!CanOpenFlightPanel() || !FlightPanelCanvas) return;
    ClearMissionRotationInput();
    MiningDronesWidget = CreateWidget<UMiningDronesDlg>(GetOwningPlayer(),
        ResolveFlightPopupClass<UMiningDronesDlg>(GetGameInstance(), TEXT("UI.MiningDronesPopupClass")));
    if (!MiningDronesWidget) return;
    AddFloatingPanel(MiningDronesPopup, MiningDronesWidget->TakeWidget(), TEXT("Mining Drones"), 2,
        FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::CloseMiningDrones), true);
}

void UMissionBriefingDlg::CloseMiningDrones()
{
    RemoveFloatingPanel(MiningDronesPopup);
    MiningDronesWidget = nullptr;
}

void UMissionBriefingDlg::ToggleGuilds()
{
    if(GuildsPopup){CloseGuilds();return;}
    if(!CanOpenFlightPanel() || !FlightPanelCanvas)return;
    ClearMissionRotationInput();
    GuildsWidget=CreateWidget<UGuildsDlg>(GetOwningPlayer(),
        ResolveFlightPopupClass<UGuildsDlg>(GetGameInstance(),TEXT("UI.GuildsPopupClass")));
    if(!GuildsWidget)return;
    AddFloatingPanel(GuildsPopup,GuildsWidget->TakeWidget(),TEXT("Guilds"),2,
        FSimpleDelegate::CreateUObject(this,&UMissionBriefingDlg::CloseGuilds),true);
}

void UMissionBriefingDlg::CloseGuilds()
{
    RemoveFloatingPanel(GuildsPopup);GuildsWidget=nullptr;
}

void UMissionBriefingDlg::ToggleHangar()
{
    if (HangarPopup) { CloseHangar(); return; }
    if (!CanOpenFlightPanel() || !FlightPanelCanvas) return;
    ClearMissionRotationInput();
    HangarWidget = CreateWidget<UHangarDlg>(GetOwningPlayer(),
        ResolveFlightPopupClass<UHangarDlg>(GetGameInstance(), TEXT("UI.HangarPopupClass")));
    if (!HangarWidget) return;
    HangarWidget->OnLoadout = FOpenShipInsurance::CreateWeakLambda(this, [this](FGuid Hull)
    {
        TGuardValue<bool> ServiceScope(bOpeningStationService, CanUseStationServices());
        OpenHangarLoadout(Hull);
    });
    HangarWidget->OnFit = FOpenShipInsurance::CreateWeakLambda(this, [this](FGuid Hull)
    {
        TGuardValue<bool> ServiceScope(bOpeningStationService, CanUseStationServices());
        OpenHangarFit(Hull);
    });
    HangarWidget->OnInsurance = FOpenShipInsurance::CreateWeakLambda(this, [this](FGuid Hull)
    {
        TGuardValue<bool> ServiceScope(bOpeningStationService, CanUseStationServices());
        OpenInsurance(Hull);
    });
    AddFloatingPanel(HangarPopup, HangarWidget->TakeWidget(), TEXT("Hangar"), 2,
        FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::CloseHangar));
}

void UMissionBriefingDlg::CloseHangar()
{
    CloseInsurance();
    CloseHangarLoadout();
    CloseFitting();
    RemoveFloatingPanel(HangarPopup);
    HangarWidget = nullptr;
}

void UMissionBriefingDlg::OpenInsurance(FGuid Hull)
{
    auto* Service = GetGameInstance()->GetSubsystem<UShipInsuranceSubsystem>();
    if (!Service || !Service->CanModifyHull(Hull) || !Service->FindShip(Hull) ||
        !CanOpenFlightPanel() || !FlightPanelCanvas) return;
    CloseInsurance();
    ClearMissionRotationInput();
    InsuranceWidget = CreateWidget<UInsuranceDlg>(GetOwningPlayer(),
        ResolveFlightPopupClass<UInsuranceDlg>(GetGameInstance(), TEXT("UI.InsurancePopupClass")));
    if (!InsuranceWidget) return;
    InsuranceWidget->SetHull(Hull);
    AddFloatingPanel(InsurancePopup, InsuranceWidget->TakeWidget(), TEXT("Insurance"), 3,
        FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::CloseInsurance));
}

void UMissionBriefingDlg::CloseInsurance()
{
    RemoveFloatingPanel(InsurancePopup);
    InsuranceWidget = nullptr;
}

void UMissionBriefingDlg::OpenHangarLoadout(FGuid Hull)
{
    auto* Service = GetGameInstance()->GetSubsystem<UShipInsuranceSubsystem>();
    auto* Players = GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    if (!Service || !Service->CanInsure() || !Players ||
        Players->GetPlayerInfo().AssignedHangarHull != Hull || !MissionWepPanel) return;
    CloseHangarLoadout();
    MissionWepPanel->SetParentDlg(this);
    MissionWepPanel->RemoveFromParent();
    LoadoutsWidget = CreateWidget<ULoadoutsDlg>(GetOwningPlayer());
    if (!LoadoutsWidget) return;
    LoadoutsWidget->SetEditor(MissionWepPanel);
    AddFloatingPanel(LoadoutsPopup, LoadoutsWidget->TakeWidget(), TEXT("Loadout"), 3,
        FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::CloseHangarLoadout), true);
    MissionWepPanel->RefreshFromMission();
}

void UMissionBriefingDlg::CloseHangarLoadout()
{
    RemoveFloatingPanel(LoadoutsPopup);
    LoadoutsWidget = nullptr;
}

void UMissionBriefingDlg::OpenHangarFit(FGuid Hull)
{
    auto* Service = GetGameInstance()->GetSubsystem<UShipInsuranceSubsystem>();
    if (!Service || !Service->CanModifyHull(Hull) || !Service->FindShip(Hull)) return;
    CloseFitting();
    FittingWidget = CreateWidget<UShipFitDlg>(GetOwningPlayer(),
        ResolveFlightPopupClass<UShipFitDlg>(GetGameInstance(), TEXT("UI.ShipFitPopupClass")));
    if (!FittingWidget) return;
    FittingWidget->SetHull(Hull);
    AddFloatingPanel(FittingPopup, FittingWidget->TakeWidget(), TEXT("Fitting"), 3,
        FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::CloseFitting));
}

void UMissionBriefingDlg::CloseFitting()
{
    RemoveFloatingPanel(FittingPopup);
    FittingWidget = nullptr;
}

void UMissionBriefingDlg::ToggleAgents()
{
    if (AgentsPopup) { CloseAgents(); return; }
    if (!CanOpenFlightPanel() || !FlightPanelCanvas) return;

    ClearMissionRotationInput();
    AgentsWidget = CreateWidget<UAgentsDlg>(GetOwningPlayer(),
        ResolveFlightPopupClass<UAgentsDlg>(GetGameInstance(), TEXT("UI.AgentsPopupClass")));
    if (!AgentsWidget) return;
    AgentsWidget->SetMenuManager(manager);
    AgentsWidget->SetCanLaunch(TDelegate<bool()>::CreateWeakLambda(this, [this]()
    {
        const Sim* Simulation = Sim::GetSim();
        return MissionPtr && MissionPtr->IsFreeFlight() &&
            (CanOpenMarketplace() || (Simulation && FStationServices::IsDocked() && CanOpenFlightPanel()));
    }));
    AddFloatingPanel(AgentsPopup, AgentsWidget->TakeWidget(), TEXT("Agents"), 1,
        FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::CloseAgents));
}

void UMissionBriefingDlg::CloseAgents()
{
    RemoveFloatingPanel(AgentsPopup);
    AgentsWidget = nullptr;
}

void UMissionBriefingDlg::ToggleContracts()
{
    if (ContractsPopup) { CloseContracts(); return; }
    if (!CanOpenFlightPanel() || !FlightPanelCanvas) return;

    ClearMissionRotationInput();
    ContractsWidget = CreateWidget<UContractsDlg>(GetOwningPlayer(),
        ResolveFlightPopupClass<UContractsDlg>(GetGameInstance(), TEXT("UI.ContractsPopupClass")));
    if (!ContractsWidget) return;
    AddFloatingPanel(ContractsPopup, ContractsWidget->TakeWidget(), TEXT("Contracts"), 1,
        FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::CloseContracts));
}

void UMissionBriefingDlg::CloseContracts()
{
    RemoveFloatingPanel(ContractsPopup);
    ContractsWidget = nullptr;
}

void UMissionBriefingDlg::ToggleLiveOptions()
{
    if (LiveOptionsPopup) { CloseLiveOptions(); return; }
    if (!CanOpenFlightPanel() || !FlightPanelCanvas) return;

    ClearMissionRotationInput();
    LiveOptionsWidget = CreateWidget<ULiveOptionsDlg>(GetOwningPlayer(),
        ResolveFlightPopupClass<ULiveOptionsDlg>(GetGameInstance(), TEXT("UI.OptionsPopupClass")));
    if (!LiveOptionsWidget) return;
    AddFloatingPanel(LiveOptionsPopup, LiveOptionsWidget->TakeWidget(), TEXT("Options"), 0,
        FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::CloseLiveOptions));
}

void UMissionBriefingDlg::CloseLiveOptions()
{
    if (!LiveOptionsPopup) return;
    if (LiveOptionsWidget) LiveOptionsWidget->SaveSettings();
    RemoveFloatingPanel(LiveOptionsPopup);
    LiveOptionsWidget = nullptr;
}

void UMissionBriefingDlg::ToggleEngineering()
{
    if (EngineeringPopup) { CloseEngineering(); return; }
    if (!CanOpenFlightPanel() || !FlightPanelCanvas) return;
    APlayerController* PC=GetOwningPlayer();
    if (!PC) return;
    ClearMissionRotationInput();
    if (MissionCameraRig) MissionCameraRig->LookX=MissionCameraRig->LookY=MissionCameraRig->RangeInput=0;
    UClass* PanelClass=LoadClass<UEngineeringDlg>(nullptr,TEXT("/Game/Screens/inGame/WBP_Engineering.WBP_Engineering_C"));
    EngineeringPanelWidget=CreateWidget<UEngineeringDlg>(PC,PanelClass?PanelClass:UEngineeringDlg::StaticClass());
    if(!EngineeringPanelWidget)return;
    EngineeringPanelWidget->OnPanelClosed=FSimpleDelegate::CreateWeakLambda(this,[this](){CloseEngineering();});
    EngineeringPanelWidget->OnExitMenuRequested = FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::ToggleMissionMenu);
    EngineeringPanelWidget->SetVisibility(ESlateVisibility::Visible);
    AddFloatingPanel(EngineeringPopup, EngineeringPanelWidget->TakeWidget(), TEXT("Engineering"), 0, FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::CloseEngineering));
}

void UMissionBriefingDlg::CloseEngineering()
{
    if (!EngineeringPopup) return;
    RemoveFloatingPanel(EngineeringPopup);
    if (EngineeringPanelWidget) EngineeringPanelWidget->RemoveFromParent();
    EngineeringPanelWidget = nullptr;
}

void UMissionBriefingDlg::ToggleIntelPopup()
{
    if (IntelPopup) { CloseIntelPopup(); return; }
    if (!CanOpenFlightPanel() || !FlightPanelCanvas) return;
    ClearMissionRotationInput();
    IntelWidget = CreateWidget<UCmdIntelDlg>(GetOwningPlayer(), UCmdIntelDlg::StaticClass());
    if (!IntelWidget) return;
    IntelWidget->UseReportSubpanels();
    AddFloatingPanel(IntelPopup, IntelWidget->TakeWidget(), TEXT("Intel & News"), 4,
        FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::CloseIntelPopup));
    IntelWidget->ShowIntelDlg();
}

void UMissionBriefingDlg::CloseIntelPopup()
{
    if (!IntelPopup) return;
    RemoveFloatingPanel(IntelPopup);
    if (IntelWidget) IntelWidget->RemoveFromParent();
    IntelWidget = nullptr;
}

void UMissionBriefingDlg::ToggleMissionPopup()
{
    if (MissionPopup) { CloseMissionPopup(); return; }
    if (!CanOpenFlightPanel() || !FlightPanelCanvas) return;
    APlayerController* PC=GetOwningPlayer();
    if (!PC) return;
    ClearMissionRotationInput();
    if (MissionCameraRig) MissionCameraRig->LookX=MissionCameraRig->LookY=MissionCameraRig->RangeInput=0;
    UClass* PanelClass = LoadClass<UObjectivesDlg>(nullptr,
        TEXT("/Game/Screens/inGame/WBP_Objectives.WBP_Objectives_C"));
    MissionPanelWidget = CreateWidget<UObjectivesDlg>(PC, PanelClass ? PanelClass : UObjectivesDlg::StaticClass());
    if (!MissionPanelWidget) return;
    MissionPanelWidget->SetMissionContext(this, true);
    MissionPanelWidget->OnPanelClosed = FSimpleDelegate::CreateWeakLambda(this, [this]() { CloseMissionPopup(); });
    MissionPanelWidget->OnExitMenuRequested = FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::ToggleMissionMenu);
    MissionPanelWidget->SetVisibility(ESlateVisibility::Visible);
    AddFloatingPanel(MissionPopup, MissionPanelWidget->TakeWidget(), TEXT("Mission"), 5, FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::CloseMissionPopup));
}

void UMissionBriefingDlg::CloseMissionPopup()
{
    if (!MissionPopup) return;
    RemoveFloatingPanel(MissionPopup);
    if (MissionPanelWidget) MissionPanelWidget->RemoveFromParent();
    MissionPanelWidget = nullptr;
}

void UMissionBriefingDlg::ToggleObjectivesPopup()
{
    if (ObjectivesPopup) { CloseObjectivesPopup(); return; }
    if (!CanOpenFlightPanel() || !FlightPanelCanvas) return;
    APlayerController* PC=GetOwningPlayer();
    if (!PC) return;
    ClearMissionRotationInput();
    if (MissionCameraRig) MissionCameraRig->LookX=MissionCameraRig->LookY=MissionCameraRig->RangeInput=0;
    UClass* PanelClass = LoadClass<UObjectivesDlg>(nullptr,
        TEXT("/Game/Screens/inGame/WBP_Objectives.WBP_Objectives_C"));
    ObjectivesPanelWidget = CreateWidget<UObjectivesDlg>(PC, PanelClass ? PanelClass : UObjectivesDlg::StaticClass());
    if (!ObjectivesPanelWidget) return;
    ObjectivesPanelWidget->SetMissionContext(this);
    ObjectivesPanelWidget->OnPanelClosed = FSimpleDelegate::CreateWeakLambda(this, [this]() { CloseObjectivesPopup(); });
    ObjectivesPanelWidget->OnExitMenuRequested = FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::ToggleMissionMenu);
    ObjectivesPanelWidget->SetVisibility(ESlateVisibility::Visible);
    AddFloatingPanel(ObjectivesPopup, ObjectivesPanelWidget->TakeWidget(), TEXT("Objectives"), 5, FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::CloseObjectivesPopup));
}

void UMissionBriefingDlg::CloseObjectivesPopup()
{
    if (!ObjectivesPopup) return;
    RemoveFloatingPanel(ObjectivesPopup);
    if (ObjectivesPanelWidget) ObjectivesPanelWidget->RemoveFromParent();
    ObjectivesPanelWidget = nullptr;
}



void UMissionBriefingDlg::ToggleFlightOpsPopup()
{
    if (FlightOpsPopup) { CloseFlightOpsPopup(); return; }
    if (!CanOpenFlightPanel() || !FlightPanelCanvas) return;
    APlayerController* PC=GetOwningPlayer();
    if (!PC) return;
    ClearMissionRotationInput();
    if (MissionCameraRig) MissionCameraRig->LookX=MissionCameraRig->LookY=MissionCameraRig->RangeInput=0;
    UClass* PanelClass = LoadClass<UFlightOpsDlg>(nullptr,
        TEXT("/Game/Screens/inGame/WBP_FlightOps.WBP_FlightOps_C"));
    FlightOpsPanelWidget = CreateWidget<UFlightOpsDlg>(PC, PanelClass ? PanelClass : UFlightOpsDlg::StaticClass());
    if (!FlightOpsPanelWidget) return;
    FlightOpsPanelWidget->OnPanelClosed = FSimpleDelegate::CreateWeakLambda(this, [this]() { CloseFlightOpsPopup(); });
    FlightOpsPanelWidget->OnExitMenuRequested = FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::ToggleMissionMenu);
    FlightOpsPanelWidget->SetVisibility(ESlateVisibility::Visible);
    AddFloatingPanel(FlightOpsPopup, FlightOpsPanelWidget->TakeWidget(), TEXT("Flight Ops"), 4, FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::CloseFlightOpsPopup));
}

void UMissionBriefingDlg::CloseFlightOpsPopup()
{
    if (!FlightOpsPopup) return;
    RemoveFloatingPanel(FlightOpsPopup);
    if (FlightOpsPanelWidget) FlightOpsPanelWidget->RemoveFromParent();
    FlightOpsPanelWidget = nullptr;
}



void UMissionBriefingDlg::ToggleNavigationPopup()
{
    if (NavigationPopup) { CloseNavigationPopup(); return; }
    if (!CanOpenFlightPanel() || !FlightPanelCanvas) return;
    APlayerController* PC=GetOwningPlayer();
    if (!PC) return;
    ClearMissionRotationInput();
    if (MissionCameraRig) MissionCameraRig->LookX=MissionCameraRig->LookY=MissionCameraRig->RangeInput=0;
    UClass* PanelClass=LoadClass<UNavigationDlg>(nullptr,
        TEXT("/Game/Screens/inGame/WBP_Navigation.WBP_Navigation_C"));
    NavigationPanelWidget=CreateWidget<UNavigationDlg>(PC,PanelClass?PanelClass:UNavigationDlg::StaticClass());
    if(!NavigationPanelWidget) { UE_LOG(LogTemp, Error, TEXT("[NavigationInput] Failed to create Navigation widget")); return; }
    UE_LOG(LogTemp, Display, TEXT("[NavigationInput] Created %s"), *NavigationPanelWidget->GetClass()->GetName());
    NavigationPanelWidget->OnPanelClosed=FSimpleDelegate::CreateWeakLambda(this,[this](){CloseNavigationPopup();});
    NavigationPanelWidget->OnExitMenuRequested = FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::ToggleMissionMenu);
    NavigationPanelWidget->SetVisibility(ESlateVisibility::Visible);
    AddFloatingPanel(NavigationPopup, NavigationPanelWidget->TakeWidget(), TEXT("Navigation"), 2, FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::CloseNavigationPopup));
}

void UMissionBriefingDlg::CloseNavigationPopup()
{
    if (!NavigationPopup) return;
    RemoveFloatingPanel(NavigationPopup);
    if (NavigationPanelWidget) NavigationPanelWidget->RemoveFromParent();
    NavigationPanelWidget = nullptr;
}



void UMissionBriefingDlg::FireMissionPrimary()
{
    if (Game::IsTimeCompressed()) return;
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim();
    Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead() || P->IsDying()) return;
    P->FirePrimary();
}

void UMissionBriefingDlg::FireMissionSecondary()
{
    if (Game::IsTimeCompressed()) return;
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim();
    Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead() || P->IsDying()) return;
    P->FireSecondary();
}

void UMissionBriefingDlg::FireMissionDecoy()
{
    if (Game::IsTimeCompressed()) return;
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim();
    Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead() || P->IsDying()) return;
    P->FireDecoy();
}

void UMissionBriefingDlg::LaunchMissionProbe()
{
    if (Game::IsTimeCompressed()) return;
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim();
    Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead() || P->IsDying()) return;
    P->LaunchProbe();
}

void UMissionBriefingDlg::CycleMissionPrimary()
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim();
    Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead() || P->IsDying()) return;
    P->CyclePrimary();
}

void UMissionBriefingDlg::CycleMissionSecondary()
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim();
    Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead() || P->IsDying()) return;
    P->CycleSecondary();
}

void UMissionBriefingDlg::CycleMissionPattern()
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim();
    Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead() || P->IsDying()) return;
    if (auto* G=P->GetSecondaryGroup()) G->SetControlMode(static_cast<WeaponsControl>((static_cast<int32>(G->GetControlMode())+1)%3));
}

void UMissionBriefingDlg::CycleMissionSweep()
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim();
    Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead() || P->IsDying()) return;
    if (auto* G=P->GetPrimaryGroup()) G->SetSweep(static_cast<WeaponsSweep>((static_cast<int32>(G->GetSweep())+1)%3));
}

void UMissionBriefingDlg::DropMissionWeaponTarget()
{
    if (Game::IsTimeCompressed()) return;
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim();
    Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead() || P->IsDying()) return;
    P->DropTarget();
}

namespace
{
    const FVector2D ContactsDesignSize(480, 560);

    FVector2D ContactsPanelSize(const FVector2D Viewport)
    {
        const double Scale = FMath::Clamp(FMath::Min(
            (Viewport.X - 2 * FlightPanelStyle::RightPanelInset) / ContactsDesignSize.X,
            (Viewport.Y * FlightPanelStyle::RightPanelBottom - FlightPanelStyle::RightPanelTop) /
                ContactsDesignSize.Y), 0.0, 1.0);
        return ContactsDesignSize * Scale;
    }

    FVector2D ContactsDefaultPosition(FVector2D Viewport, FVector2D Size)
    {
        return FVector2D(FMath::Max(0.0, Viewport.X - Size.X - FlightPanelStyle::RightPanelInset),
            FlightPanelStyle::RightPanelTop);
    }
}

void UMissionBriefingDlg::MoveContactsPanel(FVector2D PreviousCursor, FVector2D CurrentCursor)
{
    if (!ContactsPopup) return;

    const FGeometry& Geometry = ContactsPopup->GetCachedGeometry();
    const FVector2D Viewport = Geometry.GetLocalSize();
    const FVector2D Size = ContactsPanelSize(Viewport);
    const FVector2D Room = Viewport - Size;
    if (Room.X <= 0 || Room.Y <= 0) return;

    const FVector2D Origin = ContactsPanelPosition.IsSet()
        ? ContactsPanelPosition.GetValue() * Room : ContactsDefaultPosition(Viewport, Size);
    const FVector2D Moved = Origin + Geometry.AbsoluteToLocal(CurrentCursor) - Geometry.AbsoluteToLocal(PreviousCursor);
    ContactsPanelPosition = FVector2D(FMath::Clamp(Moved.X / Room.X, 0.0, 1.0),
        FMath::Clamp(Moved.Y / Room.Y, 0.0, 1.0));
}

void UMissionBriefingDlg::ToggleContactsPanel()
{
    if (ContactsPopup) { CloseContactsPopup(); return; }
    if (!CanOpenFlightPanel() || !GetWorld() || !GetWorld()->GetGameViewport()) return;
    APlayerController* PC = GetOwningPlayer();
    if (!PC) return;
    ClearMissionRotationInput();
    if (MissionCameraRig) MissionCameraRig->LookX=MissionCameraRig->LookY=MissionCameraRig->RangeInput=0;
    if (FighterHUDDetails.IsValid()) FighterHUDDetails->CloseRadio();
    UpdateFighterRadioInput();
    UClass* PanelClass = LoadClass<UContactsDlg>(nullptr,
        TEXT("/Game/Screens/inGame/BP_ContactsList.BP_ContactsList_C"));
    ContactsPanelWidget = CreateWidget<UContactsDlg>(PC, PanelClass ? PanelClass : UContactsDlg::StaticClass());
    if (!ContactsPanelWidget) return;


    ContactsPanelWidget->SetMissionHost(this);
    ContactsPanelWidget->OnPanelClosed = FSimpleDelegate::CreateWeakLambda(this,[this](){CloseContactsPopup();});
    ContactsPanelWidget->OnExitMenuRequested = FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::ToggleMissionMenu);
    ContactsPanelWidget->SetVisibility(ESlateVisibility::Visible);
    const TSharedRef<SWidget> Panel = ContactsPanelWidget->TakeWidget();
    auto Window = SNew(SFlightPanelWindow).Title(INVTEXT("Contacts"))
        .OnClose(FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::CloseContactsPopup))[Panel];
    Window->Move = [this](const FPointerEvent& Event)
    {
        MoveContactsPanel(Event.GetLastScreenSpacePosition(), Event.GetScreenSpacePosition());
    };
    ContactsPopup = SNew(SConstraintCanvas)
        .Visibility(EVisibility::SelfHitTestInvisible)
        .ForceVolatile(true)
        + SConstraintCanvas::Slot().Alignment(FVector2D::ZeroVector)
        .Offset(TAttribute<FMargin>::Create(TAttribute<FMargin>::FGetter::CreateWeakLambda(this, [this]()
        {
            const FVector2D Viewport = ContactsPopup ? ContactsPopup->GetCachedGeometry().GetLocalSize() : FVector2D::ZeroVector;
            const FVector2D Size = ContactsPanelSize(Viewport);
            const FVector2D Origin = ContactsPanelPosition.IsSet()
                ? ContactsPanelPosition.GetValue() * (Viewport - Size) : ContactsDefaultPosition(Viewport, Size);
            return FMargin(Origin.X, Origin.Y, Size.X, Size.Y);
        })))
        [
            SNew(SScaleBox)
            .Stretch(EStretch::ScaleToFit)
            .StretchDirection(EStretchDirection::DownOnly)
            [SNew(SBox).WidthOverride(ContactsDesignSize.X).HeightOverride(ContactsDesignSize.Y)
                .Clipping(EWidgetClipping::ClipToBounds)
                [Window]]
        ];
    GetWorld()->GetGameViewport()->AddViewportWidgetContent(ContactsPopup.ToSharedRef(),1100);
    UpdateFlightPanelInput(Panel);
}

void UMissionBriefingDlg::CloseContactsPopup()
{
    if (!ContactsPopup) return;
    if (GetWorld() && GetWorld()->GetGameViewport())
        GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(ContactsPopup.ToSharedRef());
    ContactsPopup.Reset();
    if (ContactsPanelWidget) ContactsPanelWidget->RemoveFromParent();
    ContactsPanelWidget = nullptr;
    UpdateFlightPanelInput();
}

void UMissionBriefingDlg::CycleMissionSensor()
{
    APlayerController* SensorPC = GetOwningPlayer();
    if (SensorPC && (StarshatterInputBindings::IsModifierDown(SensorPC,TEXT("Shift")) || StarshatterInputBindings::IsModifierDown(SensorPC,TEXT("Control")))) return;
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim(); Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead()) return;
    if (auto* S=P->GetSensor()) { if (S->GetMode()>=ESensorMode::PST) P->DropTarget(); else P->SetSensorMode(S->GetMode()==ESensorMode::GM?ESensorMode::PAS:static_cast<ESensorMode>(static_cast<int32>(S->GetMode())+1)); }
}

void UMissionBriefingDlg::MissionSensorGround()
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim(); Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead()) return;
    if(P->IsAirborne() && P->GetSensor() && P->GetSensorMode()<ESensorMode::PST) P->SetSensorMode(ESensorMode::GM);
}

void UMissionBriefingDlg::ReleaseSensorRangeUp() { bSensorRangeUpHeld = false; }
void UMissionBriefingDlg::ReleaseSensorRangeDown() { bSensorRangeDownHeld = false; }

void UMissionBriefingDlg::MissionSensorRangeUp()
{
    if (bSensorRangeUpHeld) return;
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim(); Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead()) return;
    if (auto* S=P->GetSensor()) {
        bSensorRangeUpHeld = true;
        const double Before = S->GetBeamRange();
        S->IncreaseRange();
        UE_LOG(LogTemp, Display, TEXT("[SensorRange] Up %.0f -> %.0f metres"), Before, S->GetBeamRange());
    }
}

void UMissionBriefingDlg::MissionSensorRangeDown()
{
    if (bSensorRangeDownHeld) return;
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim(); Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead()) return;
    if (auto* S=P->GetSensor()) {
        bSensorRangeDownHeld = true;
        const double Before = S->GetBeamRange();
        S->DecreaseRange();
        UE_LOG(LogTemp, Display, TEXT("[SensorRange] Down %.0f -> %.0f metres"), Before, S->GetBeamRange());
    }
}

void UMissionBriefingDlg::MissionEMCONUp()
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim(); Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead()) return;
    P->SetEMCON(P->GetEMCON()+1);
}

void UMissionBriefingDlg::MissionEMCONDown()
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim(); Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead()) return;
    P->SetEMCON(P->GetEMCON()-1);
}

void UMissionBriefingDlg::LockMissionClosest()
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim(); Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead()) return;
    P->LockTarget(ESimObject::SHIP,true,false);
}

void UMissionBriefingDlg::LockMissionHostile()
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim(); Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead()) return;
    P->LockTarget(ESimObject::SHIP,true,true);
}

void UMissionBriefingDlg::LockMissionThreat()
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim(); Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead()) return;
    P->LockTarget(ESimObject::SHOT,true,true);
}

void UMissionBriefingDlg::CycleMissionSubsystem()
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim(); Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead()) return;
    P->CycleSubTarget(1);
}

void UMissionBriefingDlg::CycleMissionSubsystemPrevious()
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* S=Sim::GetSim();Ship* P=S?S->GetPlayerShip():nullptr;
    if(P && !P->IsDead()){P->CycleSubTarget(-1);}
}

void UMissionBriefingDlg::LockMissionHostileThreat()
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* S=Sim::GetSim();Ship* P=S?S->GetPlayerShip():nullptr;
    if(P && !P->IsDead()){P->LockTarget(ESimObject::SHOT,false,true);}
}

namespace
{
bool TrackFixedMissionContact(AActor* Actor, Ship* Player, SimRegion* Region)
{
    if (!IsValid(Actor) || !Player || !Region) return false;
    if (const auto* Waypoint = Cast<AWaypointActor>(Actor))
    {
        FMissionObjectData Data;
        if ((Waypoint->bAsteroidFieldContact || !Waypoint->NamedLocationType.IsEmpty()) &&
            ReadMissionObject(Actor, Player, Region, Data))
            return Player->SetFixedAutoTrackPoint(OtherHand(Waypoint->LegacyLocation), Data.Name);
    }
    if (const auto* Visual = Cast<ANativeAsteroidActor>(Actor))
        if (const Asteroid* Rock = Visual->ResolveContact(Region))
            return Player->SetFixedAutoTrackPoint(Rock->GetLocation(),
                FString::Printf(TEXT("Asteroid %02d"), Visual->GetContactIndex() + 1));
    if (const auto* Visual = Cast<AShipActor>(Actor))
        if (Ship* Station = Visual->GetRuntimeShip(); Station && Region->GetShips().contains(Station) &&
            Station->IsStatic() && !Station->IsDead())
            return Player->SetFixedAutoTrackPoint(Station->GetLocation(), ANSI_TO_TCHAR(Station->GetName()));
    return false;
}
}

void UMissionBriefingDlg::ToggleMissionAutoTrack()
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation = Sim::GetSim();
    Ship* Player = Simulation ? Simulation->GetPlayerShip() : nullptr;
    if (!Player) return;
    if (Player->IsAutoTrackEngaged())
    {
        Player->SetAutoTrack(false);
        ResetTravelSpeed();
        return;
    }
    if (TrackFixedMissionContact(SelectedMissionTarget.Get(), Player, Simulation->GetActiveRegion()) ||
        Player->SetAutoTrack(true)) return;

    const auto* Contact = dynamic_cast<const Ship*>(Player->GetTarget());
    if (Contact && Contact->GetRegion() != Player->GetRegion())
        RadioView::Message("Auto Track unavailable. Target is in another sector.");
    else if (Player->GetSensorMode() == ESensorMode::GM)
        RadioView::Message("Auto Track unavailable in Ground Map. Switch to Standard radar and lock the drone.");
    else if (!Contact || !Player->GetSensor() || !Player->GetSensor()->IsTracking(Player->GetTarget()))
        RadioView::Message("Auto Track unavailable. Acquire a radar lock first.");
    else
        RadioView::Message("Auto Track unavailable in the current flight state.");
}

void UMissionBriefingDlg::ToggleMissionAutoNav()
{
    UE_LOG(LogTemp, Display, TEXT("[AutoNavInput] Started; controls=%d paused=%d"),
        CanUseFighterHUD() ? 1 : 0, GetWorld() && GetWorld()->IsPaused() ? 1 : 0);
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation = Sim::GetSim();
    Ship* Player = Simulation ? Simulation->GetPlayerShip() : nullptr;
    SimRegion* Region = Simulation ? Simulation->GetActiveRegion() : nullptr;
    if (!Player || !Region || !Region->GetShips().contains(Player) || Player->IsDead()) return;
    if (LastAutoNavInputFrame == GFrameCounter) return;
    LastAutoNavInputFrame = GFrameCounter;
    if (Player->GetInbound())
    {
        Player->SetAutoLand(!Player->IsAutoLandEngaged());
        SyncMissionNavigationTarget();
        return;
    }
    if (Player->IsAutoNavEngaged())
    {
        Player->SetAutoNav(false);
        return;
    }
    if (Player->IsAutoTrackEngaged())
    {
        Player->SetAutoTrack(false);
        return;
    }
    if (!TrackFixedMissionContact(SelectedMissionTarget.Get(), Player, Region)) Player->SetAutoNav(true);
}


void UMissionBriefingDlg::SyncMissionNavigationTarget()
{
    if (!bLiveMissionStarted || !GetWorld()) return;
    Sim* LiveSim = Sim::GetSim();
    Ship* Pilot = LiveSim ? LiveSim->GetPlayerShip() : nullptr;
    SimRegion* ActiveRegion = LiveSim ? LiveSim->GetActiveRegion() : nullptr;
    if (!Pilot || !ActiveRegion || !ActiveRegion->GetShips().contains(Pilot) || !Pilot->GetElement()) return;
    InboundSlot* Arrival=Pilot->GetInbound();
    FlightDeck* RecoveryDeck=Arrival?Arrival->GetDeck():nullptr;
    const Ship* RecoveryCarrier=RecoveryDeck?RecoveryDeck->GetCarrier():nullptr;
    if(Arrival && Arrival->Cleared() && RecoveryCarrier)
    {
        // Recovery owns ILS guidance, not the pilot's manually selected contact.
        if(!bRecoveryTargetSelected)
            for(TActorIterator<AShipActor> It(GetWorld());It;++It)
                if(It->GetRuntimeShip()==RecoveryCarrier)
                {
                    if(SelectedMissionTarget.Get()!=*It) SelectMissionContact(*It);
                    bRecoveryTargetSelected=SelectedMissionTarget.Get()==*It;
                    break;
                }
        return;
    }
    bRecoveryTargetSelected=false;
    if (Pilot->GetFixedAutoTrackPoint()) return;
    Instruction* RoutePoint = Pilot->GetNextNavPoint();
    const bool Engaged = Pilot->IsAutoNavEngaged();
    const bool AutoChanged = Engaged && (!bAutoNavSelectionActive || AutoNavSelectedRoutePoint != RoutePoint);
    if (!Engaged) { bAutoNavSelectionActive = false; AutoNavSelectedRoutePoint = nullptr; }
    if (!RoutePoint) return;
    const int32 RouteIndex = Pilot->GetNavIndex(RoutePoint);
    AWaypointActor* SelectedWaypoint = Cast<AWaypointActor>(SelectedMissionTarget.Get());
    const bool OwnWaypoint = SelectedWaypoint && SelectedWaypoint->ElementIdentity == Pilot->GetElement()->GetIdentity();
    bool CompletedSelection = false;
    if (OwnWaypoint && SelectedWaypoint->WaypointIndex != RouteIndex) {
        List<Instruction>& Plan = Pilot->GetFlightPlan();
        const int32 PreviousIndex = SelectedWaypoint->WaypointIndex - 1;
        if (PreviousIndex >= 0 && PreviousIndex < Plan.size()) {
            Instruction* Previous = Plan.at(PreviousIndex);
            CompletedSelection = Previous && Previous->GetStatus() > INSTRUCTION_STATUS::ACTIVE;
        }
    }
    // A completed selected waypoint follows the route even in manual flight.
    // Updating target display never engages autonav or modifies the route.
    if (!AutoChanged && !CompletedSelection) return;
    const TArray<AActor*> Candidates = GetSelectableMissionContacts();
    AActor* Destination = nullptr;
    if (!OwnWaypoint) {
        if (SimObject* Object = RoutePoint->GetTarget()) {
            for (TActorIterator<AShipActor> It(GetWorld()); It; ++It)
                if (It->GetRuntimeShip() == Object && Candidates.Contains(*It)) { Destination = *It; break; }
        }
    }
    if (!Destination) {
        for (TActorIterator<AWaypointActor> It(GetWorld()); It; ++It)
            if (It->ElementIdentity == Pilot->GetElement()->GetIdentity() &&
                It->WaypointIndex == RouteIndex && Candidates.Contains(*It)) { Destination = *It; break; }
    }
    if (!Destination) return;
    SelectMissionContact(Destination);
    if (SelectedMissionTarget.Get() == Destination) {
        AutoNavSelectedRoutePoint = RoutePoint;
        bAutoNavSelectionActive = Engaged;
        UE_LOG(LogTemp, Display, TEXT("[NavTargetSync] Waypoint=%d Actor=%s Auto=%d"), RouteIndex, *Destination->GetName(), Engaged ? 1 : 0);
    }
}


void UMissionBriefingDlg::ToggleQuantumTravel()
{
    if (QuantumPopup) { CloseQuantumTravel(); return; }
    if (!CanOpenFlightPanel() || !FlightPanelCanvas) return;
    Ship* QuantumPlayer = Sim::GetSim() ? Sim::GetSim()->GetPlayerShip() : nullptr;
    if (!QuantumPlayer || !QuantumPlayer->GetQuantumDrive()) return;
    APlayerController* PC = GetOwningPlayer();
    if (!PC) return;
    ClearMissionRotationInput();
    if (MissionCameraRig) MissionCameraRig->LookX = MissionCameraRig->LookY = MissionCameraRig->RangeInput = 0;
    TWeakObjectPtr<UMissionBriefingDlg> Owner(this);
    auto Content = SNew(SQuantumTravelPopup)
        .ResolveShip([Owner](){Sim* S=Sim::GetSim();return Owner.IsValid() && Owner->bLiveMissionStarted && S ? S->GetPlayerShip() : nullptr;})
        .CanOperate([Owner](){return Owner.IsValid() && Owner->bLiveMissionStarted && Owner->GetWorld() && !Owner->GetWorld()->IsPaused() && !Owner->PendingTravelLevel;})
        .OnClose(FSimpleDelegate::CreateWeakLambda(this,[this](){CloseQuantumTravel();}))
        .OnExitMenuRequested(FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::ToggleMissionMenu));
    AddFloatingPanel(QuantumPopup, Content, TEXT("Quantum / Farcaster Navigation"), 3, FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::CloseQuantumTravel));
}

void UMissionBriefingDlg::CloseQuantumTravel()
{
    if (!QuantumPopup) return;
    RemoveFloatingPanel(QuantumPopup);
}

void UMissionBriefingDlg::ClearTravelPresentation(bool bStopUpdates)
{
    if (bStopUpdates && GetWorld()) GetWorld()->GetTimerManager().ClearTimer(TravelPresentationTimer);
    CloseQuantumTravel();
    if (QuantumCameraPresentation) QuantumCameraPresentation->Reset();
    QuantumCameraPresentation.Reset();
    bTravelPresentationUnavailable=false; PreviousPresentedWarp=1; PreviousQuantumPhase=QuantumDrive::ACTIVE_READY; PresentedQuantumTraveler=nullptr;
    if (GetWorld() && GetWorld()->GetGameViewport() && TravelTransition.IsValid()) GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(TravelTransition.ToSharedRef());
    TravelTransition.Reset(); PresentedTravelRegion=nullptr; PendingTravelPackage.Empty(); TravelRevealAt=0;
    AtmosphereTravelFade = 0.f;
    bAtmosphereTravel = false;
    bAtmosphereDescent = false;
    if (PendingTravelLevel)
    {
        PendingTravelLevel->OnLevelShown.RemoveDynamic(this,&UMissionBriefingDlg::HandleTravelSystemShown);
        PendingTravelLevel->SetShouldBeVisible(false); PendingTravelLevel->SetShouldBeLoaded(false); PendingTravelLevel->SetIsRequestingUnloadAndRemoval(true);
        PendingTravelLevel=nullptr;
    }
}

void UMissionBriefingDlg::UpdateTravelPresentation(float DeltaSeconds)
{
    if (!bLiveMissionStarted || !GetWorld() || MissionSceneCover.IsValid())
    { if (QuantumCameraPresentation) QuantumCameraPresentation->Reset(); return; }
    Sim* Simulation=Sim::GetSim(); Ship* Pilot=Simulation?Simulation->GetPlayerShip():nullptr;
    SimRegion* Region=Pilot?Pilot->GetRegion():nullptr;
    if (!Pilot || !Region || Pilot->IsDead() || Pilot->IsDying()) { ClearTravelPresentation(false); return; }
    if (PresentedQuantumTraveler!=Pilot)
    {
        if (QuantumCameraPresentation) QuantumCameraPresentation->Reset();
        PresentedQuantumTraveler=Pilot; PreviousPresentedWarp=1; PreviousQuantumPhase=QuantumDrive::ACTIVE_READY;
    }
    auto* Runtime=GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>();
    const FString Package=Region->GetSystem()?FString(TEXT("/Game/Maps/"))+UTF8_TO_TCHAR(Region->GetSystem()->GetName()):FString();
    const int TransitionType = Pilot->GetTransitionType();
    const bool bEnteringAtmosphere = TransitionType == Ship::TRANSITION_DROP_ORBIT;
    const bool bLeavingAtmosphere = TransitionType == Ship::TRANSITION_MAKE_ORBIT;
    const bool bOrbitTransition = bEnteringAtmosphere || bLeavingAtmosphere;
    const bool bSurfaceSwap = PresentedTravelRegion && PresentedTravelRegion != Region &&
        PresentedTravelRegion->GetSystem() == Region->GetSystem() &&
        PresentedTravelRegion->IsAirSpace() != Region->IsAirSpace();

    if (bOrbitTransition)
    {
        bAtmosphereTravel = true;
        bAtmosphereDescent = bEnteringAtmosphere;
        constexpr float FadeInSeconds = 2.0f;
        AtmosphereTravelFade = FMath::SmoothStep(0.f, FadeInSeconds,
            FadeInSeconds - Pilot->GetTransitionTimeRemaining());
    }
    else if (bSurfaceSwap)
    {
        bAtmosphereTravel = true;
        bAtmosphereDescent = Region->IsAirSpace();
        AtmosphereTravelFade = 1.f;
    }
    else if (bAtmosphereTravel && !GetWorld()->IsPaused())
    {
        constexpr float FadeOutSeconds = 2.0f;
        AtmosphereTravelFade = FMath::Max(0.f, AtmosphereTravelFade - DeltaSeconds / FadeOutSeconds);
        bAtmosphereTravel = AtmosphereTravelFade > 0.f;
    }

    if (PresentedTravelRegion != Region)
    {
        if (PresentedTravelRegion)
        {
            CloseQuantumTravel(); SelectedMissionTarget.Reset(); SelectedMissionTargetName=FText::GetEmpty(); bMissionTargetInspection=false;
            if (!bSurfaceSwap)
            {
                MissionPlayerCameraActor.Reset(); MissionPlayerCameraSearchTime=0; MissionPlayerCameraDistance=0;
                if (MissionCameraRig) MissionCameraRig->Reset(GetOwningPlayer());
            }
            if (Runtime) Runtime->RefreshTravelPresentation(nullptr, bSurfaceSwap);
            UE_LOG(LogTemp,Display,TEXT("[Travel] Presentation arrival %hs"),Region->GetName());
        }
        PresentedTravelRegion=Region;
    }
    if (!PendingTravelLevel && Package.Equals(MissionSystemPackage,ESearchCase::IgnoreCase)) bTravelPresentationUnavailable=false;
    if (!PendingTravelLevel && !Package.IsEmpty() && !Package.Equals(MissionSystemPackage,ESearchCase::IgnoreCase) && Package!=PendingTravelPackage)
    {
        PendingTravelPackage=Package;
        bTravelPresentationUnavailable=true;
        if (FPackageName::DoesPackageExist(Package))
        {
            bool Started=false;
            PendingTravelLevel=ULevelStreamingDynamic::LoadLevelInstance(GetWorld(),Package,FVector::ZeroVector,FRotator::ZeroRotator,Started);
            if (Started && PendingTravelLevel)
            {
                TravelLoadStarted=GetWorld()->GetTimeSeconds();
                PendingTravelLevel->OnLevelShown.AddDynamic(this,&UMissionBriefingDlg::HandleTravelSystemShown);
                PendingTravelLevel->SetShouldBeLoaded(true); PendingTravelLevel->SetShouldBeVisible(true);
            }
            else { PendingTravelLevel=nullptr; UE_LOG(LogTemp,Error,TEXT("[Travel] Could not stream %s"),*Package); }
        }
        else UE_LOG(LogTemp,Error,TEXT("[Travel] Destination map missing: %s"),*Package);
    }
    if (PendingTravelLevel)
    {
        if (PendingTravelLevel->IsLevelVisible()) HandleTravelSystemShown();
        if (PendingTravelLevel && GetWorld()->GetTimeSeconds()-TravelLoadStarted>30)
        {
            UE_LOG(LogTemp,Error,TEXT("[Travel] Destination presentation timed out: %s"),*PendingTravelPackage);
            PendingTravelLevel->OnLevelShown.RemoveDynamic(this,&UMissionBriefingDlg::HandleTravelSystemShown);
            PendingTravelLevel->SetShouldBeVisible(false);PendingTravelLevel->SetShouldBeLoaded(false);PendingTravelLevel->SetIsRequestingUnloadAndRemoval(true);PendingTravelLevel=nullptr;
        }
    }
    // A failed stream is not an endless loading state. Keep the live simulation
    // and panels available after timeout; the current native builder follows it.
    const bool Loading = PendingTravelLevel != nullptr;
    Ship* TravelSource = ResolveQuantumPresentationShip(Pilot);
    QuantumDrive* Drive = TravelSource->GetQuantumDrive();
    const int32 DrivePhase=Drive ? Drive->ActiveState() : QuantumDrive::ACTIVE_READY;
    const double Warp=TravelSource->GetWarpFactor();
    EQuantumPresentationPhase CameraPhase=EQuantumPresentationPhase::Idle;
    if (Loading) CameraPhase=EQuantumPresentationPhase::Loading;
    else if (DrivePhase==QuantumDrive::ACTIVE_COUNTDOWN) CameraPhase=EQuantumPresentationPhase::Countdown;
    else if (DrivePhase==QuantumDrive::ACTIVE_PREWARP) CameraPhase=EQuantumPresentationPhase::Departure;
    else if (DrivePhase==QuantumDrive::ACTIVE_POSTWARP || GetWorld()->GetTimeSeconds()<TravelRevealAt) CameraPhase=EQuantumPresentationPhase::Arrival;
    else if (Warp>1.01) CameraPhase=Warp>=PreviousPresentedWarp ? EQuantumPresentationPhase::Departure : EQuantumPresentationPhase::Arrival;
    if (!QuantumCameraPresentation) QuantumCameraPresentation=MakeShared<FQuantumCameraPresentation>();
    const bool Aborted = !Loading && DrivePhase==QuantumDrive::ACTIVE_READY && Warp<=1.01 &&
        (PreviousQuantumPhase==QuantumDrive::ACTIVE_COUNTDOWN || PreviousQuantumPhase==QuantumDrive::ACTIVE_PREWARP);
    if (Aborted || bAtmosphereTravel || (bTravelPresentationUnavailable && !PendingTravelLevel)) QuantumCameraPresentation->Reset();
    else QuantumCameraPresentation->Update(GetOwningPlayer(),GetWorld()->IsPaused()?0.f:DeltaSeconds,CameraPhase,
        MissionCameraRig ? static_cast<int32>(MissionCameraRig->Mode) : INDEX_NONE);
    PreviousPresentedWarp=Warp; PreviousQuantumPhase=DrivePhase;
    const bool Transit = bAtmosphereTravel || Warp>1.01 || Loading ||
        GetWorld()->GetTimeSeconds()<TravelRevealAt || QuantumCameraPresentation->IsVisible();
    if (Transit)
    {
        CloseQuantumTravel();
        if (!TravelTransition.IsValid() && GetWorld()->GetGameViewport())
        {
            TravelTransition=SNew(STravelTransition);
            GetWorld()->GetGameViewport()->AddViewportWidgetContent(TravelTransition.ToSharedRef(),1800);
        }
        if (TravelTransition.IsValid())
        {
            if (bAtmosphereTravel && !Loading)
                TravelTransition->UpdateAtmosphere(AtmosphereTravelFade, bAtmosphereDescent);
            else
                TravelTransition->Update(GetWorld()->IsPaused()?0.f:DeltaSeconds,Warp,
                    Loading,UTF8_TO_TCHAR(Region->GetName()),bTravelPresentationUnavailable && !PendingTravelLevel);
        }
    }
    else if (TravelTransition.IsValid())
    {
        if(GetWorld()->GetGameViewport())GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(TravelTransition.ToSharedRef());
        TravelTransition.Reset();
    }
}

void UMissionBriefingDlg::HandleTravelSystemShown()
{
    if (!PendingTravelLevel || !bLiveMissionStarted || !GetWorld()) return;
    ASystemSceneBuilder* Builder=nullptr;
    for(TActorIterator<ASystemSceneBuilder> It(GetWorld());It;++It)
        if(It->GetLevel()==PendingTravelLevel->GetLoadedLevel()){Builder=*It;break;}
    if(!Builder)return;
    auto* Simulation=Sim::GetSim();
    auto* Region=Simulation?Simulation->GetActiveRegion():nullptr;
    if(!Region || !Region->GetSystem())return;
    // Streamed maps may have no authored target names and BuildOnBeginPlay off.
    // Initialize from the arrived simulation system instead of waiting forever.
    Builder->EnsureMissionSystem(Region->GetSystem());
    if(Builder->GetSpawnedBodies().IsEmpty())return;
    if(auto* Runtime=GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>())Runtime->RefreshTravelPresentation(Builder);
    if(MissionSystemLevel)
    {
        MissionSystemLevel->OnLevelShown.RemoveDynamic(this,&UMissionBriefingDlg::HandleMissionSystemLevelShown);
        MissionSystemLevel->SetShouldBeVisible(false);MissionSystemLevel->SetShouldBeLoaded(false);MissionSystemLevel->SetIsRequestingUnloadAndRemoval(true);
    }
    MissionSystemLevel=PendingTravelLevel;
    MissionSystemPackage=PendingTravelPackage;
    bTravelPresentationUnavailable=false;
    PendingTravelLevel->OnLevelShown.RemoveDynamic(this,&UMissionBriefingDlg::HandleTravelSystemShown);
    PendingTravelLevel=nullptr;
    TravelRevealAt=GetWorld()->GetTimeSeconds()+1.5;
    UE_LOG(LogTemp,Display,TEXT("[Travel] Destination system presentation ready: %s"),*MissionSystemPackage);
}

void UMissionBriefingDlg::StartLiveSessionUpdates()
{
    if (!GetWorld()) return;
    FTimerManagerTimerParameters TimerParameters;
    TimerParameters.bLoop=true;
    TimerParameters.bMaxOncePerFrame=true;
    GetWorld()->GetTimerManager().SetTimer(TravelPresentationTimer,
        FTimerDelegate::CreateWeakLambda(this,[this]()
        {
            if (!GetWorld() || !bLiveMissionStarted) return;
            UpdateLiveMission();
            if (bLiveMissionStarted) UpdateTravelPresentation(GetWorld()->GetDeltaSeconds());
        }),0.000001f,TimerParameters);
}

void UMissionBriefingDlg::ToggleFighterInformationPanel()
{
    if(CanUseFighterHUD() && FighterHUDDetails.IsValid())FighterHUDDetails->ToggleInformationPanel();
}

void UMissionBriefingDlg::ToggleSkills()
{
    if (SkillsPopup) { CloseSkills(); return; }
    if (!CanOpenFlightPanel() || !FlightPanelCanvas) return;
    ClearMissionRotationInput();
    SkillsWidget = CreateWidget<USkillsDlg>(GetOwningPlayer(),
        ResolveFlightPopupClass<USkillsDlg>(GetGameInstance(), TEXT("UI.SkillsPopupClass")));
    if (SkillsWidget) AddFloatingPanel(SkillsPopup, SkillsWidget->TakeWidget(), TEXT("Skills"), 2,
        FSimpleDelegate::CreateUObject(this, &UMissionBriefingDlg::CloseSkills));
}
void UMissionBriefingDlg::CloseSkills()
{
    RemoveFloatingPanel(SkillsPopup); SkillsWidget = nullptr;
}

void UMissionBriefingDlg::ToggleShipSkins()
{
    if(ShipSkinPopup){CloseShipSkins();return;}
    if(!CanOpenFlightPanel() || !FlightPanelCanvas)return;
    ClearMissionRotationInput();
    ShipSkinWidget=CreateWidget<UShipSkinDlg>(GetOwningPlayer(),ResolveFlightPopupClass<UShipSkinDlg>(GetGameInstance(),TEXT("UI.ShipSkinPopupClass")));
    if(ShipSkinWidget)AddFloatingPanel(ShipSkinPopup,ShipSkinWidget->TakeWidget(),TEXT("SKIN - Spectral Khromatic Integrated Nanocoating"),2,FSimpleDelegate::CreateUObject(this,&UMissionBriefingDlg::CloseShipSkins));
}
void UMissionBriefingDlg::CloseShipSkins()
{
    RemoveFloatingPanel(ShipSkinPopup);ShipSkinWidget=nullptr;
}
