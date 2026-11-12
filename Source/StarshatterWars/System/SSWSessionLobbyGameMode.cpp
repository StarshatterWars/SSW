#include "SSWSessionLobbyGameMode.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "MultiplayerDlg.h"
#include "MissionUIStyle.h"
#include "GameFramework/SpectatorPawn.h"
#include "GameFramework/GameState.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameSession.h"
#include "SSWRuntimeSubsystem.h"
#include "SSWGameInstance.h"
#include "Engine/GameInstance.h"
#include "Net/UnrealNetwork.h"
#include "Sim.h"
#include "SimRegion.h"
#include "SimShot.h"
#include "Ship.h"
#include "Game.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInterface.h"
#include "Drive.h"
#include "Thruster.h"
#include "NavLight.h"
#include "Sensor.h"
#include "SimSystem.h"
#include "Instruction.h"
#include "TimerManager.h"
#include "Mission.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "SystemSceneBuilder.h"
#include "Misc/PackageName.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "StarSystem.h"
#include "OrbitalBody.h"
#include "OrbitalRegion.h"
#include "Components/StaticMeshComponent.h"
#include "ShipActor.h"
#include "FighterActor.h"
#include "StarshatterInputBindings.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputKeyEventArgs.h"
#include "Engine/LocalPlayer.h"
#include "FighterHUDDetails.h"
#include "Engine/GameViewportClient.h"
#include "ShipDesign.h"
#include "Weapon.h"
#include "WeaponDesign.h"
#include "WeaponGroup.h"
#include "SSWSessionSubsystem.h"
#include "Engine/Canvas.h"
#include "ShipDesignRegistry.h"
#include "ShipVisualCatalog.h"
#include "StarshatterEnvironmentSubsystem.h"
#include "StarshatterGameDataSubsystem.h"
#include "SimElement.h"
#include "StarshatterPlayerSubsystem.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
namespace
{
    ASystemSceneBuilder* BuildLobbySystem(UWorld& World, ULevelStreamingDynamic& Level, const FString& Map)
    {
        if (!Level.IsLevelVisible()) return nullptr;
        const FString SystemName = FPackageName::GetShortName(Map);
        for (TActorIterator<ASystemSceneBuilder> It(&World); It; ++It)
        {
            if (It->GetLevel() != Level.GetLoadedLevel()) continue;
            if (!It->GetLastResolvedSystemName().Equals(SystemName, ESearchCase::IgnoreCase) ||
                It->GetSpawnedBodies().IsEmpty())
            {
                It->bBuildOnBeginPlay = false;
                It->DataSource = ESystemSceneSource::StarSystem;
                It->TargetSystemName = SystemName;
                It->TargetGalaxyName.Reset();
                It->BuildSystemScene();
            }
            return It->GetSpawnedRegions().IsEmpty() ? nullptr : *It;
        }
        return nullptr;
    }
}

ASSWSessionLobbyGameMode::ASSWSessionLobbyGameMode(){PlayerControllerClass=ASSWSessionLobbyController::StaticClass();DefaultPawnClass=ASpectatorPawn::StaticClass();GameStateClass=ASSWSessionGameState::StaticClass();GameSessionClass=AGameSession::StaticClass();PlayerStateClass=ASSWSessionPlayerState::StaticClass();HUDClass=ASSWSessionFlightHUD::StaticClass();bUseSeamlessTravel=true;}
void ASSWSessionLobbyController::BeginPlay(){Super::BeginPlay();if(!IsLocalController())return;
 bAutoManageActiveCameraTarget = false;
 if (const auto* Instance = Cast<USSWGameInstance>(GetGameInstance()))
 {
     const FString Name = Instance->GetPlatformPilotName().TrimStartAndEnd().Left(64);
     if (!Name.IsEmpty()) ServerSetPlatformPilotName(Name);
 }

 if(auto* Runtime=GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>())Runtime->SetPaused(true);
 LobbyDialog=CreateWidget<UMultiplayerDlg>(this,UMultiplayerDlg::StaticClass());
 if(auto* Dialog=LobbyDialog.Get()){Dialog->AddToViewport(300);Dialog->Show();bShowMouseCursor=true;FInputModeUIOnly Input;Input.SetWidgetToFocus(Dialog->TakeWidget());SetInputMode(Input);}
}

void ASSWSessionGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ASSWSessionGameState, MissionShips);
    DOREPLIFETIME(ASSWSessionGameState, MissionProjectiles);
    DOREPLIFETIME(ASSWSessionGameState, ChatHistory);
    DOREPLIFETIME(ASSWSessionGameState, MissionStartDeadline);
    DOREPLIFETIME(ASSWSessionGameState, bMissionStartReady);
    DOREPLIFETIME(ASSWSessionGameState, MissionLaunchError);
    DOREPLIFETIME(ASSWSessionGameState, MissionMap);
    DOREPLIFETIME(ASSWSessionGameState, ArenaSector);
    DOREPLIFETIME(ASSWSessionGameState, ArenaVisualCenter);
    DOREPLIFETIME(ASSWSessionGameState, ArenaOrbitalOrigin);
    DOREPLIFETIME(ASSWSessionGameState, ArenaRadius);
    DOREPLIFETIME(ASSWSessionGameState, GalaxyClockMs);
    DOREPLIFETIME(ASSWSessionGameState, OrbitalPositions);
}

void ASSWSessionGameState::BindSimulation(Sim& Simulation)
{
    if (!HasAuthority() || BoundSimulation == &Simulation) return;
    BoundSimulation = &Simulation;
    Simulation.OnMissionUnloading.AddUObject(this, &ASSWSessionGameState::ClearMissionState);
    Simulation.OnShipTargetChanged.AddUObject(this, &ASSWSessionGameState::ShipStateChanged);
    Simulation.OnShipEmconChanged.AddUObject(this, &ASSWSessionGameState::ShipStateChanged);
    Simulation.OnShipDamageApplied.AddUObject(this, &ASSWSessionGameState::ShipDamaged);
    Simulation.OnProjectileAdded.AddUObject(this, &ASSWSessionGameState::ProjectileAdded);
    Simulation.OnShipTransferred.AddUObject(this, &ASSWSessionGameState::ShipTransferred);
    Simulation.OnShipRetired.AddUObject(this, &ASSWSessionGameState::ShipRetired);
}

void ASSWSessionGameState::UnbindSimulation(Sim& Simulation)
{
    if (BoundSimulation != &Simulation) return;
    Simulation.OnMissionUnloading.RemoveAll(this);
    Simulation.OnShipTargetChanged.RemoveAll(this);
    Simulation.OnShipEmconChanged.RemoveAll(this);
    Simulation.OnShipDamageApplied.RemoveAll(this);
    Simulation.OnProjectileAdded.RemoveAll(this);
    Simulation.OnShipTransferred.RemoveAll(this);
    Simulation.OnShipRetired.RemoveAll(this);
    BoundSimulation = nullptr;
    ClearMissionState();
}

void ASSWSessionGameState::ClearMissionState()
{
    MissionShips.Reset();
    MissionProjectiles.Reset();
    ArenaSector.Reset();
    ArenaOrbitalOrigin = FVector::ZeroVector;
    ArenaRadius = 0.0;
    RetiredShips.Reset();
    ShotCounts.Reset();
    DamageCounts.Reset();
    NextSnapshotTime = 0.0;
    ForceNetUpdate();
}

void ASSWSessionGameState::ShipStateChanged(const Ship&)
{
    NextSnapshotTime = 0.0;
}

void ASSWSessionGameState::ShipDamaged(const Ship& DamagedShip)
{
    ++DamageCounts.FindOrAdd(DamagedShip.GetObjID());
    ShipStateChanged(DamagedShip);
}

void ASSWSessionGameState::ProjectileAdded(const SimShot& Shot, const SimRegion&)
{
    if (const Ship* Shooter = Shot.Owner())
        ++ShotCounts.FindOrAdd(Shooter->GetObjID());
}

void ASSWSessionGameState::ShipTransferred(const Ship& TransferredShip, const SimRegion&, const SimRegion&)
{
    ShipStateChanged(TransferredShip);
}

void ASSWSessionGameState::ShipRetired(const Ship& RetiredShip, const SimRegion&)
{
    FSSWReplicatedShipState State = MakeShipState(RetiredShip);
    State.bRetired = true;
    RetiredShips.Add(RetiredShip.GetObjID(), MoveTemp(State));
    ShipStateChanged(RetiredShip);
}

FSSWReplicatedShipState ASSWSessionGameState::MakeShipState(const Ship& PilotShip) const
{
    FSSWReplicatedShipState State;
    State.ObjectId = PilotShip.GetObjID();
    State.Name = UTF8_TO_TCHAR(PilotShip.GetName());
    if (const SimRegion* Region = PilotShip.GetRegion()) State.Region = UTF8_TO_TCHAR(Region->GetName());
    if (const SimElement* Element = PilotShip.GetElement()) State.Element = UTF8_TO_TCHAR(Element->GetName().data());
    State.IFF = PilotShip.GetIFF();
    State.Position = PilotShip.GetLocation();
    if (const auto* Runtime = GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>())
        State.VisualPosition = Runtime->GetVisualLocationForRegion(PilotShip.GetRegion(), State.Position);
    if (const ShipDesign* Design = PilotShip.Design()) State.Design = UTF8_TO_TCHAR(Design->name);
    State.Velocity = PilotShip.GetVelocity();
    State.Heading = PilotShip.GetHeading();
    State.Up = PilotShip.GetCam().vup();
    State.Integrity = static_cast<float>(PilotShip.GetIntegrity());
    if (const auto* Design = PilotShip.Design(); Design && Design->integrity > 0)
        State.HullPercent = static_cast<float>(FMath::Clamp(PilotShip.GetIntegrity() / Design->integrity * 100.0, 0.0, 100.0));
    State.PrimaryReadout = SFighterHUDDetails::FormatWeaponReadout(PilotShip.GetPrimaryGroup());
    State.SecondaryReadout = SFighterHUDDetails::FormatWeaponReadout(PilotShip.GetSecondaryGroup());
    State.ShieldStrength = PilotShip.GetShieldStrength();
    State.FuelPercent = PilotShip.GetFuelLevel();
    State.Throttle = static_cast<float>(PilotShip.GetThrottle());
    if (const auto* Sensors = PilotShip.GetSensor())
        State.SensorRange = Sensors->GetBeamRange();
    for (int32 Index = 0; Index < FMath::Min(32, PilotShip.navlights.size()); ++Index)
        if (const auto* Light = PilotShip.navlights[Index]; Light && Light->IsEnabled())
            State.EnabledNavLights |= 1u << Index;
    if (const auto* Visual = PilotShip.GetNativePresentation())
        Visual->GetCarriedStoreTypes(State.CarriedStores);
    State.RespawnsRemaining = PilotShip.GetRespawnCount();
    if (const Drive* MainDrive = PilotShip.GetMainDrive())
        State.DrivePower = MainDrive->GetVisualPower();
    if (const Thruster* Jets = PilotShip.GetThruster())
    {
        State.ThrusterBurns.Reserve(Jets->GetNumThrusters());
        for (int32 Port = 0; Port < Jets->GetNumThrusters(); ++Port)
            State.ThrusterBurns.Add(static_cast<uint8>(FMath::RoundToInt(
                255.0f * FMath::Clamp(Jets->GetThrusterBurn(Port), 0.0f, 1.0f))));
    }
    if (const Weapon* Primary = PilotShip.GetPrimary()) State.PrimaryAmmo = Primary->Ammo();
    if (const Weapon* Secondary = PilotShip.GetSecondary()) State.SecondaryAmmo = Secondary->Ammo();
    if (const WeaponDesign* Primary = PilotShip.GetPrimaryDesign())
    {
        State.PrimaryRange = Primary->max_range;
        State.PrimaryShotSpeed = Primary->speed;
    }
    if (const WeaponGroup* Missiles = PilotShip.GetSecondaryGroup(); Missiles && Missiles->IsMissile() && Missiles->Ammo() > 0)
        if (const Weapon* Selected = Missiles->GetSelected())
            State.bMissileLocked = Selected->Locked() != 0;
    if (const SimObject* Target = PilotShip.GetTarget()) State.TargetId = Target->GetObjID();
    State.bAutoTracking = PilotShip.IsAutoTrackEngaged();
    State.Emcon = PilotShip.GetEMCON();
    State.bPlayerControlled = PilotShip.HasNetworkPilot();
    State.FlightPhase = static_cast<int32>(PilotShip.GetFlightPhase());
    State.bAirborne = PilotShip.IsAirborne();
    if (const Ship* Carrier = PilotShip.GetCarrier()) State.CarrierId = Carrier->GetObjID();
    State.bLandingAssigned = PilotShip.GetInbound() != nullptr;
    State.ShotsFired = ShotCounts.FindRef(PilotShip.GetObjID());
    State.DamageEvents = DamageCounts.FindRef(PilotShip.GetObjID());
    State.bDestroyed = PilotShip.IsDead() || PilotShip.IsDying();
    return State;
}

void ASSWSessionGameState::PublishSimulation(Sim& Simulation, double Now)
{
    if (!HasAuthority() || BoundSimulation != &Simulation || Now < NextSnapshotTime) return;
    NextSnapshotTime = Now + 0.1;
    if (!Simulation.GetMission())
    {
        if (!MissionShips.IsEmpty() || !MissionProjectiles.IsEmpty()) ClearMissionState();
        return;
    }

    MissionShips.Reset();
    MissionProjectiles.Reset();
    TMap<int64, FString> PilotNames;
    for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        if (const auto* Pilot = Cast<ASSWSessionLobbyController>(It->Get()); Pilot && Pilot->GetMissionShipId())
            if (const auto* PlayerState = Pilot->GetPlayerState<APlayerState>())
                PilotNames.Add(Pilot->GetMissionShipId(), PlayerState->GetPlayerName());

    ListIter<SimRegion> Regions = Simulation.GetRegions();
    while (++Regions)
    {
        SimRegion* Region = Regions.value();
        if (!Region) continue;
        ListIter<Ship> Ships = Region->GetShips();
        while (++Ships)
        {
            Ship* PilotShip = Ships.value();
            if (!PilotShip) continue;
            FSSWReplicatedShipState State = MakeShipState(*PilotShip);
            if (const FString* PilotName = PilotNames.Find(State.ObjectId))
                State.PilotName = *PilotName;
            ListIter<SimSystem> Systems = PilotShip->GetSystems();
            while (++Systems)
            {
                if (const SimSystem* System = Systems.value())
                {
                    FSSWReplicatedSystemState& SystemState = State.Systems.AddDefaulted_GetRef();
                    SystemState.Name = UTF8_TO_TCHAR(System->GetName());
                    SystemState.Status = static_cast<int32>(System->GetStatus());
                    SystemState.Availability = static_cast<float>(System->GetAvailability());
                    SystemState.bPowered = System->IsPowerOn();
                }
            }
            ListIter<Instruction> Route = PilotShip->GetFlightPlan();
            while (++Route)
            {
                if (const Instruction* Point = Route.value())
                {
                    FSSWReplicatedNavPoint& NavPoint = State.FlightPlan.AddDefaulted_GetRef();
                    NavPoint.Position = Point->GetLocation();
                    NavPoint.Region = UTF8_TO_TCHAR(Point->GetRegionName());
                    NavPoint.Action = static_cast<int32>(Point->GetAction());
                    NavPoint.Status = static_cast<int32>(Point->GetStatus());
                    NavPoint.Speed = static_cast<float>(Point->GetSpeed());
                }
            }
            MissionShips.Add(MoveTemp(State));
        }

        ListIter<SimShot> Shots = Region->GetShots();
        while (++Shots)
        {
            if (const SimShot* Shot = Shots.value())
            {
                FSSWReplicatedProjectileState& State = MissionProjectiles.AddDefaulted_GetRef();
                State.ObjectId = Shot->GetObjID();
                State.Region = UTF8_TO_TCHAR(Region->GetName());
                if (const Ship* Shooter = Shot->Owner()) State.OwnerId = Shooter->GetObjID();
                State.Position = Shot->GetLocation();
                State.Velocity = Shot->GetVelocity();
                State.bMissile = Shot->IsMissile();
                if (const auto* Runtime = GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>())
                {
                    const FVector Tail = Shot->IsBeam() ? Shot->GetOrigin() :
                        State.Position - State.Velocity.GetSafeNormal() * (State.bMissile ? 20.0 : 6.0);
                    State.VisualStart = Runtime->GetVisualLocationForRegion(Region, Tail);
                    State.VisualEnd = Runtime->GetVisualLocationForRegion(Region, State.Position);
                }
            }
        }
    }
    for (const auto& Retired : RetiredShips) MissionShips.Add(Retired.Value);
    ForceNetUpdate();
}

void ASSWSessionLobbyController::SendSessionChat(const FString& Message)
{
    const FString Text = Message.TrimStartAndEnd().Left(256);
    if (IsLocalController() && !Text.IsEmpty()) ServerSendSessionChat(Text);
}

void ASSWSessionLobbyController::ServerSendSessionChat_Implementation(const FString& Message)
{
    UWorld* World = GetWorld();
    const APlayerState* Sender = GetPlayerState<APlayerState>();
    if (!World || !Sender || !AcceptChat(Message)) return;

    const FString Text = Message.TrimStartAndEnd();
    if (Text.IsEmpty()) return;

    if (auto* State = World->GetGameState<ASSWSessionGameState>())
    {
        NextChatTime = FPlatformTime::Seconds() + 0.5;
        State->AddChatMessage(Sender->GetPlayerName(), Text);
    }
}

void ASSWSessionGameState::AddChatMessage(const FString& Sender, const FString& Text)
{
    if (!HasAuthority()) return;
    constexpr int32 HistoryLimit = 32;
    if (ChatHistory.Num() >= HistoryLimit) ChatHistory.RemoveAt(0);
    ChatHistory.Add({Sender, Text});
    OnRep_ChatHistory();
    ForceNetUpdate();
}

void ASSWSessionGameState::OnRep_ChatHistory()
{
    OnChatChanged.Broadcast();
}

bool ASSWSessionLobbyController::AcceptChat(const FString& Message)
{
    const double Now = FPlatformTime::Seconds();
    if (Message.Len() > 256 || Message.TrimStartAndEnd().IsEmpty() || Now < NextChatTime)
        return false;
    NextChatTime = Now + 0.5;
    return true;
}

Ship* ASSWSessionGameState::FindMissionShip(int64 ObjectId) const
{
    if (!HasAuthority() || !BoundSimulation || !BoundSimulation->GetMission() ||
        ObjectId <= 0 || ObjectId > MAX_uint32)
        return nullptr;
    Ship* PilotShip = BoundSimulation->FindShipByObjID(static_cast<uint32>(ObjectId));
    return PilotShip && !PilotShip->IsDead() && !PilotShip->IsDying() ? PilotShip : nullptr;
}

bool ASSWSessionLobbyController::AssignMissionShip(int64 ObjectId)
{
    if (!HasAuthority()) return false;
    if (ObjectId == 0)
    {
        if (const auto* State = GetWorld()->GetGameState<ASSWSessionGameState>())
            if (Ship* Previous = State->FindMissionShip(MissionShipId)) Previous->SetNetworkPilot(false);
        MissionShipId = 0;
        return true;
    }
    const auto* State = GetWorld()->GetGameState<ASSWSessionGameState>();
    if (!State || !State->FindMissionShip(ObjectId)) return false;
    for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        const auto* Other = Cast<ASSWSessionLobbyController>(It->Get());
        if (Other && Other != this && Other->MissionShipId == ObjectId) return false;
    }
    AssignMissionShip(0);
    MissionShipId = ObjectId;
    Ship* Fighter = State->FindMissionShip(ObjectId);
    Fighter->SetNetworkPilot(true);
    if (auto* Mode = GetWorld()->GetAuthGameMode<ASSWSessionLobbyGameMode>())
        Mode->TrackServerPilot(*this, *Fighter);
    if (auto* Visual = Cast<AFighterActor>(Fighter->GetNativePresentation()))
        Visual->UseNetworkChaseView();
    ForceNetUpdate();
    return true;
}

void ASSWSessionLobbyController::SendMissionChat(ESSWChatChannel Channel, const FString& Message, int64 RecipientShipId)
{
    if (IsLocalController())
        ServerSendMissionChat(Channel, Message.TrimStartAndEnd().Left(256), RecipientShipId);
}

void ASSWSessionLobbyController::ServerSendMissionChat_Implementation(
    ESSWChatChannel Channel, const FString& Message, int64 RecipientShipId)
{
    const auto* State = GetWorld()->GetGameState<ASSWSessionGameState>();
    const auto* Sender = GetPlayerState<APlayerState>();
    if (!State || !State->bMissionStartReady || !Sender || !AcceptChat(Message)) return;

    const Ship* Source = State->FindMissionShip(MissionShipId);
    if (!Source) return;

    const FSSWSessionChatMessage Entry{Sender->GetPlayerName(), Message.TrimStartAndEnd(), Channel};
    bool bDelivered = false;
    for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        auto* Recipient = Cast<ASSWSessionLobbyController>(It->Get());
        if (!Recipient || Recipient == this) continue;
        const Ship* Destination = State->FindMissionShip(Recipient->MissionShipId);
        if (!Destination) continue;

        const bool bMatches = Channel == ESSWChatChannel::Broadcast ||
            (Channel == ESSWChatChannel::Team && Source->GetIFF() == Destination->GetIFF()) ||
            (Channel == ESSWChatChannel::Wing && Source->GetElement() &&
                Source->GetElement() == Destination->GetElement()) ||
            (Channel == ESSWChatChannel::Direct && Recipient->MissionShipId == RecipientShipId);
        if (!bMatches) continue;

        Recipient->ReceiveMissionChat(Channel, Entry);
        bDelivered = true;
    }
    if (bDelivered) ReceiveMissionChat(Channel, Entry);
}

void ASSWSessionLobbyController::ReceiveMissionChat_Implementation(
    ESSWChatChannel Channel, const FSSWSessionChatMessage& Message)
{
    if (PrivateChatHistory.Num() >= 32) PrivateChatHistory.RemoveAt(0);
    PrivateChatHistory.Add(Message);
    RefreshMissionChat();
}

void ASSWSessionLobbyController::ReceivePilotDeparture_Implementation(const FString& Message)
{
    const double Now = FPlatformTime::Seconds();
    DepartureNotices.RemoveAll([Now](const auto& Notice) { return Notice.Key <= Now; });
    if (DepartureNotices.Num() >= 4) DepartureNotices.RemoveAt(0);
    DepartureNotices.Emplace(Now + 6.0, Message);
    ReceiveMissionChat_Implementation(ESSWChatChannel::Broadcast,
        {TEXT("System"), Message, ESSWChatChannel::Broadcast});
}

namespace
{
    const TCHAR* MissionChatChannelLabel(ESSWChatChannel Channel)
    {
        switch (Channel)
        {
        case ESSWChatChannel::Team: return TEXT("Team");
        case ESSWChatChannel::Wing: return TEXT("Wing");
        case ESSWChatChannel::Direct: return TEXT("Direct");
        default: return TEXT("All");
        }
    }
}

void ASSWSessionLobbyController::OpenMissionChat()
{
    auto* Viewport = GetWorld()->GetGameViewport();
    if (!Viewport || !FlightCamera || MissionChatPanel) return;

    auto Channels = SNew(SHorizontalBox);
    for (const auto Channel : {ESSWChatChannel::Broadcast, ESSWChatChannel::Team, ESSWChatChannel::Wing, ESSWChatChannel::Direct})
    {
        Channels->AddSlot().AutoWidth().Padding(0, 0, 4, 0)
        [
            SNew(SButton)
            .Text(FText::FromString(MissionChatChannelLabel(Channel)))
            .OnClicked_Lambda([this, Channel]
            {
                MissionChatChannel = Channel;
                return FReply::Handled().SetUserFocus(MissionChatEntry.ToSharedRef());
            })
        ];
    }
    Channels->AddSlot().HAlign(HAlign_Right)
    [
        SNew(SButton).Text(FText::FromString(TEXT("Close")))
        .OnClicked_Lambda([this] { CloseMissionChat(); return FReply::Handled(); })
    ];

    auto Taunts = SNew(SVerticalBox);
    const auto* Players = GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    if (Players)
    {
        const auto& Macros = Players->GetPlayerInfo().ChatMacros;
        for (int32 Index = 0; Index < Macros.Num(); ++Index)
        {
            if (Macros[Index].TrimStartAndEnd().IsEmpty()) continue;
            Taunts->AddSlot().AutoHeight().Padding(0, 1)
            [
                SNew(SButton)
                .Text(FText::FromString(FString::Printf(TEXT("%d. %s"), Index, *Macros[Index])))
                .OnClicked_Lambda([this, Message = Macros[Index]]
                {
                    SubmitMissionChat(Message);
                    return FReply::Handled();
                })
            ];
        }
    }
    if (!Taunts->NumSlots())
        Taunts->AddSlot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("Set chat macros in the Pilot Logbook.")))];

    MissionChatPanel = SNew(SOverlay)
        + SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0, 24, 16, 0)
        [
            SNew(SBox).WidthOverride(360)
            [
                SNew(SBorder).BorderBackgroundColor(FLinearColor(.025f, .07f, .11f, .95f)).Padding(12)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("RADIO")))]
                    + SVerticalBox::Slot().AutoHeight().Padding(0, 8)[Channels]
                    + SVerticalBox::Slot().AutoHeight()[SNew(SBox).HeightOverride(116)[SAssignNew(MissionChatLog, SScrollBox)]]
                    + SVerticalBox::Slot().AutoHeight().Padding(0, 8)
                    [
                        SAssignNew(MissionChatEntry, SEditableTextBox)
                        .HintText_Lambda([this]
                        {
                            return FText::FromString(FString::Printf(TEXT("Message to %s"), MissionChatChannelLabel(MissionChatChannel)));
                        })
                        .OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type Commit)
                        {
                            if (Commit == ETextCommit::OnEnter) SubmitMissionChat(Text.ToString());
                            else if (Commit == ETextCommit::OnCleared && MissionChatPanel) OpenResignDialog();
                        })
                    ]
                    + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("TAUNTS")))]
                    + SVerticalBox::Slot().AutoHeight()[SNew(SBox).MaxDesiredHeight(128)[SNew(SScrollBox) + SScrollBox::Slot()[Taunts]]]
                ]
            ]
        ];
    Viewport->AddViewportWidgetContent(MissionChatPanel.ToSharedRef(), 110);
    RefreshMissionChat();
    bPrimaryPressed = false;
    PendingTranslation = FVector::ZeroVector;
    FInputModeUIOnly Mode;
    Mode.SetWidgetToFocus(MissionChatEntry);
    SetInputMode(Mode);
    bShowMouseCursor = true;
}

void ASSWSessionLobbyController::CloseMissionChat()
{
    if (!MissionChatPanel) return;
    auto Panel = MoveTemp(MissionChatPanel);
    if (auto* Viewport = GetWorld()->GetGameViewport())
        Viewport->RemoveViewportWidgetContent(Panel.ToSharedRef());
    MissionChatEntry.Reset();
    MissionChatLog.Reset();
    SetInputMode(FInputModeGameOnly());
    bShowMouseCursor = false;
}

void ASSWSessionLobbyController::OpenResignDialog(bool bSelfDestruct)
{
    auto* Viewport = GetWorld()->GetGameViewport();
    if (!Viewport || !FlightCamera || ResignDialog) return;

    CloseMissionChat();
    bPrimaryPressed = false;
    PendingTranslation = FVector::ZeroVector;
    auto* Sessions = GetGameInstance()->GetSubsystem<USSWSessionSubsystem>();
    TSharedPtr<SButton> Cancel;
    auto Status = SNew(STextBlock).AutoWrapText(true);
    const FText Prompt = bSelfDestruct
        ? NSLOCTEXT("SSW.Multiplayer", "SelfDestructPrompt", "Destroy your fighter? This uses a life.")
        : GetNetMode() == NM_ListenServer
            ? NSLOCTEXT("SSW.Multiplayer", "ResignHostPrompt", "Resigning as host ends the match for all pilots.")
            : NSLOCTEXT("SSW.Multiplayer", "ResignPrompt", "Leave the current match?");

    ResignDialog = SNew(SBorder)
        .BorderBackgroundColor(FLinearColor(0.01f, 0.02f, 0.035f, 0.65f))
        .HAlign(HAlign_Center).VAlign(VAlign_Center)
        [
            SNew(SBox).WidthOverride(440)
            [
                SNew(SBorder).Padding(24).BorderBackgroundColor(FLinearColor(0.06f, 0.13f, 0.19f))
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight()
                    [
                        SNew(STextBlock).Font(MissionUIStyle::GetHeaderFont(28))
                        .Text(bSelfDestruct ? NSLOCTEXT("SSW.Multiplayer", "SelfDestructTitle", "SELF-DESTRUCT")
                            : NSLOCTEXT("SSW.Multiplayer", "ResignTitle", "RESIGN GAME"))
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(0, 12)
                    [SNew(STextBlock).AutoWrapText(true).Text(Prompt)]
                    + SVerticalBox::Slot().AutoHeight()
                    [Status]
                    + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0, 12, 0, 0)
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
                        [
                            SNew(SButton)
                            .Visibility(bSelfDestruct ? EVisibility::Collapsed : EVisibility::Visible)
                            .Text(NSLOCTEXT("SSW.Multiplayer", "SelfDestruct", "Self-Destruct"))
                            .IsEnabled_Lambda([this, Sessions]
                            {
                                const auto* Fighter = GetAssignedShipState();
                                return Fighter && !Fighter->bDestroyed && !Fighter->bRetired &&
                                    (!Sessions || !Sessions->IsBusy());
                            })
                            .OnClicked_Lambda([this]
                            {
                                CloseResignDialog();
                                OpenResignDialog(true);
                                return FReply::Handled();
                            })
                        ]
                        + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
                        [
                            SAssignNew(Cancel, SButton)
                            .Text(NSLOCTEXT("SSW.Multiplayer", "ResignCancel", "Cancel"))
                            .OnClicked_Lambda([this] { CloseResignDialog(); return FReply::Handled(); })
                            .IsEnabled_Lambda([Sessions] { return !Sessions || !Sessions->IsBusy(); })
                        ]
                        + SHorizontalBox::Slot().AutoWidth()
                        [
                            SNew(SButton)
                            .Text(bSelfDestruct ? NSLOCTEXT("SSW.Multiplayer", "SelfDestructConfirm", "Confirm")
                                : NSLOCTEXT("SSW.Multiplayer", "ResignConfirm", "Resign"))
                            .IsEnabled_Lambda([Sessions, bSelfDestruct] { return bSelfDestruct || (Sessions && !Sessions->IsBusy()); })
                            .OnClicked_Lambda([this, Sessions, Status, bSelfDestruct]
                            {
                                if (bSelfDestruct)
                                {
                                    ServerSelfDestruct();
                                    CloseResignDialog();
                                    return FReply::Handled();
                                }
                                if (Sessions->LeaveSession())
                                    Status->SetText(TAttribute<FText>::CreateLambda([Sessions]
                                    {
                                        return FText::FromString(Sessions->Status);
                                    }));
                                else
                                    Status->SetText(FText::FromString(Sessions->Status));
                                return FReply::Handled();
                            })
                        ]
                    ]
                ]
            ]
        ];
    Viewport->AddViewportWidgetContent(ResignDialog.ToSharedRef(), 120);
    FInputModeUIOnly Mode;
    Mode.SetWidgetToFocus(Cancel);
    SetInputMode(Mode);
    bShowMouseCursor = true;
}

void ASSWSessionLobbyController::CloseResignDialog()
{
    if (!ResignDialog) return;
    auto Dialog = MoveTemp(ResignDialog);
    if (auto* Viewport = GetWorld()->GetGameViewport())
        Viewport->RemoveViewportWidgetContent(Dialog.ToSharedRef());
    SetInputMode(FInputModeGameOnly());
    bShowMouseCursor = false;
}

void ASSWSessionLobbyController::RefreshMissionChat()
{
    if (!MissionChatLog) return;
    MissionChatLog->ClearChildren();
    for (const auto& Message : PrivateChatHistory)
        MissionChatLog->AddSlot()
        [
            SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(FString::Printf(
                TEXT("[%s] %s: %s"), MissionChatChannelLabel(Message.Channel), *Message.Sender, *Message.Text)))
        ];
    MissionChatLog->ScrollToEnd();
}

void ASSWSessionLobbyController::SubmitMissionChat(const FString& Message)
{
    if (Message.TrimStartAndEnd().IsEmpty()) return;
    const auto* Fighter = GetAssignedShipState();
    const int64 Target = Fighter ? Fighter->TargetId : 0;
    const auto* Contacts = GetFlightContacts();
    if (MissionChatChannel == ESSWChatChannel::Direct && (!Target || !Contacts ||
        !Contacts->ContainsByPredicate([Target](const FSSWReplicatedShipState& Contact)
        {
            return Contact.ObjectId == Target && Contact.bPlayerControlled && !Contact.bDestroyed && !Contact.bRetired;
        })))
    {
        if (MissionChatEntry) MissionChatEntry->SetError(FText::FromString(TEXT("Lock a pilot to send a direct message.")));
        return;
    }
    SendMissionChat(MissionChatChannel, Message, Target);
    if (MissionChatEntry)
    {
        MissionChatEntry->SetError(FText::GetEmpty());
        MissionChatEntry->SetText(FText::GetEmpty());
    }
}

void ASSWSessionPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ASSWSessionPlayerState, bReady);
}

void ASSWSessionPlayerState::OnRep_Ready()
{
    if (auto* State = GetWorld()->GetGameState<ASSWSessionGameState>())
        State->OnRosterChanged.Broadcast();
}

void ASSWSessionLobbyController::SetLobbyReady_Implementation(bool bReady)
{
    const auto* Lobby = GetWorld()->GetGameState<ASSWSessionGameState>();
    auto* PilotState = GetPlayerState<ASSWSessionPlayerState>();
    if (!Lobby || Lobby->bMissionStartReady || !PilotState || PilotState->bReady == bReady) return;

    PilotState->bReady = bReady;
    PilotState->OnRep_Ready();
    PilotState->ForceNetUpdate();
    UE_LOG(LogTemp, Log, TEXT("[SessionLobby] Pilot %d ready=%d"), PilotState->GetPlayerId(), bReady);
}

void ASSWSessionGameState::AddPlayerState(APlayerState* PlayerState)
{
    Super::AddPlayerState(PlayerState);
    OnRosterChanged.Broadcast();
}

void ASSWSessionGameState::RemovePlayerState(APlayerState* PlayerState)
{
    Super::RemovePlayerState(PlayerState);
    OnRosterChanged.Broadcast();
}

void ASSWSessionLobbyGameMode::BeginPlay()
{
    Super::BeginPlay();
    if (GetNetMode() == NM_DedicatedServer)
    {
        auto* Runtime = GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>();
        if (!Runtime || !Runtime->Init() || !Runtime->InitGame())
        {
            UE_LOG(LogTemp, Error, TEXT("[StarServer] Simulation startup failed"));
            return;
        }
        Runtime->StartRuntime();
        Runtime->SetGameMode(EGameMode::MENU);
    }

    LobbyMissionIndex = UGameplayStatics::GetIntOption(OptionsString, TEXT("SSWMission"), INDEX_NONE);
    const auto* Data = GetGameInstance()->GetSubsystem<UStarshatterGameDataSubsystem>();
    const auto* Scenario = Data && Data->GetMultiplayerMissionScenarios().IsValidIndex(LobbyMissionIndex)
        ? &Data->GetMultiplayerMissionScenarios()[LobbyMissionIndex] : nullptr;
    if (!Scenario && Data && LobbyMissionIndex == INDEX_NONE)
        Scenario = Data->GetMultiplayerMissionScenarios().FindByPredicate([](const FS_CampaignMission& Row)
        {
            return Row.MissionName.Equals(TEXT("Hawk Match"), ESearchCase::IgnoreCase);
        });
    const int32 MissionCapacity = Scenario ? Scenario->GetMultiplayerCapacity() : FS_CampaignMission::MultiplayerConnectionLimit;
    PlayerCapacity = FMath::Clamp(UGameplayStatics::GetIntOption(OptionsString, TEXT("SSWPlayers"), MissionCapacity), 1, MissionCapacity);

    const FString Map = Scenario
        ? FString::Printf(TEXT("/Game/Maps/%s"), *Scenario->MissionSystem) : TEXT("/Game/Maps/Borova");
    if (auto* State = GetGameState<ASSWSessionGameState>())
    {
        State->MissionMap = Map;
        State->ArenaSector = Scenario ? Scenario->MissionRegion : TEXT("Borova");
    }

    if (GetNetMode() == NM_DedicatedServer)
    {
        if (auto* Sessions = GetGameInstance()->GetSubsystem<USSWSessionSubsystem>())
            Sessions->AdvertiseDedicated(!FParse::Param(FCommandLine::Get(), TEXT("SSWEOS")), PlayerCapacity,
                Scenario ? Scenario->MissionName : FString(TEXT("Hawk Match")));
    }
    else
    {
        bool bStreamingStarted = false;
        LobbySystemLevel = ULevelStreamingDynamic::LoadLevelInstance(
            GetWorld(), Map, FVector::ZeroVector, FRotator::ZeroRotator, bStreamingStarted);
        if (!bStreamingStarted) LobbySystemLevel = nullptr;
    }
    GetWorldTimerManager().SetTimer(MissionCountdownTimer, this,
        &ThisClass::UpdateMissionCountdown, 0.25f, true);
}

void ASSWSessionLobbyGameMode::UpdateMissionCountdown()
{
    auto* State = GetGameState<ASSWSessionGameState>();
    if (!State) return;

    if (State->bMissionStartReady && GetNetMode() == NM_DedicatedServer)
    {
        if (LobbyMission && LobbyMission->IsComplete() && !HasMatchEnded())
            EndMatch();
        if (auto* Runtime = GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>())
            Runtime->SetPaused(GetNumPlayers() == 0);
        return;
    }

    int32 PilotCount = 0;
    bool bAllReady = true;
    if (!LobbySceneBuilder.IsValid() && LobbySystemLevel && LobbySystemLevel->IsLevelVisible())
        LobbySceneBuilder = BuildLobbySystem(*GetWorld(), *LobbySystemLevel, State->MissionMap);
    for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        auto* Controller = Cast<ASSWSessionLobbyController>(It->Get());
        if (!Controller) continue;
        if (Controller->IsLocalController() && LobbySceneBuilder.IsValid())
            Controller->SetLobbySpaceView(*LobbySceneBuilder.Get(), State->ArenaSector);
        ++PilotCount;
        const auto* Player = Controller->GetPlayerState<ASSWSessionPlayerState>();
        bAllReady &= Player && Player->bReady;
    }

    if (PilotCount < MinimumMissionPilots || !bAllReady)
    {
        if (State->MissionStartDeadline > 0.0 || State->bMissionStartReady)
        {
            State->MissionStartDeadline = 0.0;
            State->bMissionStartReady = false;
            State->ForceNetUpdate();
        }
        return;
    }

    if (State->bMissionStartReady) return;

    if (LobbySystemLevel && !LobbySystemLevel->IsLevelVisible()) return;
    const double Now = State->GetServerWorldTimeSeconds();
    if (State->MissionStartDeadline <= 0.0)
    {
        State->MissionStartDeadline = Now + MissionCountdownSeconds;
        State->ForceNetUpdate();
    }
    else if (Now >= State->MissionStartDeadline)
    {
        State->MissionStartDeadline = 0.0;
        if (LaunchLobbyMission(*State))
        {
            State->bMissionStartReady = true;
            if (GetNetMode() != NM_DedicatedServer)
                GetWorldTimerManager().ClearTimer(MissionCountdownTimer);
            OnMissionCountdownCompleted();
        }
        else
        {
            for (APlayerState* Entry : State->PlayerArray)
            {
                if (auto* Player = Cast<ASSWSessionPlayerState>(Entry))
                {
                    Player->bReady = false;
                    Player->OnRep_Ready();
                    Player->ForceNetUpdate();
                }
            }
        }
        State->ForceNetUpdate();
    }
}

bool ASSWSessionLobbyGameMode::LaunchLobbyMission(ASSWSessionGameState& State)
{
    auto* Instance = GetGameInstance();
    auto* Runtime = Instance->GetSubsystem<USSWRuntimeSubsystem>();
    auto* Environment = Instance->GetSubsystem<UStarshatterEnvironmentSubsystem>();
    const auto* Data = Instance->GetSubsystem<UStarshatterGameDataSubsystem>();
    const FS_CampaignMission* Scenario = nullptr;
    if (Data)
    {
        const auto& Missions = Data->GetMultiplayerMissionScenarios();
        if (Missions.IsValidIndex(LobbyMissionIndex)) Scenario = &Missions[LobbyMissionIndex];
        else if (LobbyMissionIndex == INDEX_NONE)
            Scenario = Missions.FindByPredicate([](const FS_CampaignMission& Row)
            {
                return Row.MissionName.Equals(TEXT("Hawk Match"), ESearchCase::IgnoreCase);
            });
    }

    State.MissionLaunchError = TEXT("SELECTED MISSION UNAVAILABLE");
    if (!Runtime || !Environment || !Scenario) return false;

    if (Scenario->MissionSystem.TrimStartAndEnd().IsEmpty() ||
        Scenario->MissionRegion.TrimStartAndEnd().IsEmpty())
    {
        State.MissionLaunchError = TEXT("MISSION SYSTEM OR SECTOR IS MISSING");
        return false;
    }

    TSharedPtr<Mission> Selected = MakeShared<Mission>();
    if (!Selected->LoadFromCampaignMissionData(*Scenario)) return false;

    if (!Sim::GetSim()) Runtime->CreateWorld();
    Sim* Simulation = Sim::GetSim();
    if (!Simulation) return false;
    if (Simulation->GetMission())
    {
        State.MissionLaunchError = TEXT("ANOTHER SIMULATION IS STILL LOADED");
        return false;
    }

    ASystemSceneBuilder* Builder = LobbySceneBuilder.IsValid() ? LobbySceneBuilder.Get() : LobbySystemLevel
        ? BuildLobbySystem(*GetWorld(), *LobbySystemLevel, State.MissionMap) : nullptr;
    if (!Builder && GetNetMode() != NM_DedicatedServer)
    {
        State.MissionLaunchError = TEXT("MISSION SCENE IS UNAVAILABLE OR STILL LOADING");
        return false;
    }

    Simulation->SetNetworkWorld(GetWorld());
    State.BindSimulation(*Simulation);
    Environment->BuildSimRegionsForSim(Simulation);
    SimRegion* MissionRegion = Simulation->FindRegion(Selected->GetRegion());
    if (!MissionRegion || !MissionRegion->GetSystem() ||
        !Scenario->MissionSystem.Equals(ANSI_TO_TCHAR(MissionRegion->GetSystem()->GetName()), ESearchCase::IgnoreCase))
    {
        State.MissionLaunchError = TEXT("MISSION SECTOR IS UNAVAILABLE IN THE SELECTED SYSTEM");
        return false;
    }

    if (Builder) Runtime->BeginMissionPresentation(Builder);
    Simulation->LoadMission(Selected.Get(), false);
    Simulation->ExecMission();

    TArray<ASSWSessionLobbyController*> Pilots;
    for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        if (auto* Pilot = Cast<ASSWSessionLobbyController>(It->Get()))
            Pilots.Add(Pilot);

    TArray<Ship*> Fighters;
    if (SimRegion* Region = Simulation->GetActiveRegion())
    {
        ListIter<Ship> Ships = Region->GetShips();
        while (++Ships)
            if (Ship* Candidate = Ships.value(); Candidate && Candidate->GetElement() &&
                Candidate->GetElement()->IsPlayable() && !Candidate->IsStatic())
                Fighters.Add(Candidate);
    }

    if (Ship* LocalFighter = Simulation->GetPlayerShip())
        Fighters.Sort([LocalFighter](const Ship& A, const Ship& B) {
            return (&A == LocalFighter) > (&B == LocalFighter);
        });

    UE_LOG(LogTemp, Log, TEXT("[SessionLobby] Mission %hs has %d playable ships for %d pilots"),
        Selected->GetName(), Fighters.Num(), Pilots.Num());
    bool bAssigned = Fighters.Num() >= Pilots.Num();
    for (int32 Index = 0; bAssigned && Index < Pilots.Num(); ++Index)
        bAssigned = Pilots[Index]->AssignMissionShip(Fighters[Index]->GetObjID());

    if (!bAssigned)
    {
        for (auto* Pilot : Pilots) Pilot->AssignMissionShip(0);
        Runtime->EndMissionPresentation();
        Simulation->UnloadMission();
        State.MissionLaunchError = TEXT("NOT ENOUGH PLAYABLE FIGHTERS");
        return false;
    }

    LobbyMission = MoveTemp(Selected);
    if (GetNetMode() == NM_ListenServer)
    {
        const auto* Orbit = MissionRegion->GetOrbitalRegion();
        const bool bHawkMatch = Scenario->MissionName.Equals(TEXT("Hawk Match"), ESearchCase::IgnoreCase);
        const auto* Planet = Orbit ? Orbit->Primary() : nullptr;
        if (bHawkMatch && Planet)
        {
            const FVector PlanetCenter = OtherHand(Planet->Location() - MissionRegion->GetLocation());
            for (int32 Index = 0; Index < Fighters.Num(); ++Index)
            {
                Ship& Fighter = *Fighters[Index];
                const FVector Position = Fighter.GetLocation();
                const double HeightOffset = 500.0 * (Index - (Fighters.Num() - 1) * 0.5);
                Fighter.MoveTo(Position + (Position - PlanetCenter).GetSafeNormal() * HeightOffset);
                Fighter.LookAt(PlanetCenter);
            }
        }
        State.ArenaSector = Scenario->MissionRegion;
        State.ArenaVisualCenter = Runtime->GetVisualLocationForRegion(MissionRegion, FVector::ZeroVector);
        State.ArenaOrbitalOrigin = MissionRegion->GetLocation();
        State.ArenaRadius = 50000.0;
        for (ListIter<Ship> Ships = MissionRegion->GetShips(); ++Ships;)
            if (const Ship* Entry = Ships.value())
                State.ArenaRadius = FMath::Max(State.ArenaRadius, Entry->GetLocation().Size() + 25000.0);
        for (int32 Index = 0; Index < Pilots.Num(); ++Index)
        {
            Ship& Fighter = *Fighters[Index];
            FPilotSpawn& Spawn = PilotSpawns.Add(Pilots[Index]);
            Spawn.Name = UTF8_TO_TCHAR(Fighter.GetName());
            Spawn.Registry = UTF8_TO_TCHAR(Fighter.Registry());
            Spawn.Design = const_cast<ShipDesign*>(Fighter.Design());
            Spawn.Position = Fighter.GetLocation();
            Spawn.LastPosition = Spawn.Position;
            Spawn.Radius = FMath::Min(4000.0, (State.ArenaRadius - Spawn.Position.Size()) * 0.5);
            for (const Ship* Other : Fighters)
                if (Other != &Fighter)
                {
                    const double Separation = FVector::Dist(Spawn.Position, Other->GetLocation());
                    if (Separation > UE_SMALL_NUMBER)
                        Spawn.Radius = FMath::Min(Spawn.Radius, Separation * 0.25);
                }
            Fighter.GetCam().Orientation().ComputeEulerAngles(Spawn.Roll, Spawn.Pitch, Spawn.Yaw);
            Spawn.Iff = Fighter.GetIFF();
            Spawn.bWasSimulationPlayer = &Fighter == Simulation->GetPlayerShip();
            Spawn.CommandAI = Fighter.GetCommandAILevel();
            Spawn.ElementIndex = Fighter.GetOrigElementIndex();
            Spawn.ElementName = UTF8_TO_TCHAR(Fighter.GetElement()->GetName().data());
            if (const int* Loadout = Fighter.GetLoadout())
                Spawn.Loadout.Append(Loadout, Spawn.Design->hardpoints.size());
            Fighter.SetRespawnCount(Spawn.Remaining);
        }
        GetWorldTimerManager().SetTimer(ArenaTimer, this, &ThisClass::UpdateArena, 0.05f, true);
    }
    State.MissionLaunchError.Reset();
    Runtime->SetGameMode(EGameMode::PLAY);
    Runtime->SetPaused(false);
    return true;
}

void ASSWSessionLobbyGameMode::UpdateArena()
{
    auto* State = GetGameState<ASSWSessionGameState>();
    Sim* Simulation = Sim::GetSim();
    if (!State || !State->bMissionStartReady || !Simulation ||
        Simulation->GetMission() != LobbyMission.Get()) return;
    SimRegion* Region = Simulation->FindRegion(State->ArenaSector);
    if (!Region) return;
    State->ArenaOrbitalOrigin = Region->GetLocation();

    for (ListIter<Ship> Ships = Region->GetShips(); ++Ships;)
        if (Ship* Fighter = Ships.value(); Fighter && !Fighter->IsStatic() &&
            !Fighter->IsDead() && !Fighter->IsDying() && Fighter->GetFlightPhase() == EOPSMode::ACTIVE &&
            Fighter->GetLocation().Size() + Fighter->GetNavigationHullRadius() >= State->ArenaRadius)
        {
            Fighter->SetIntegrity(0);
            Fighter->DeathSpiral();
        }

    const double Now = State->GetServerWorldTimeSeconds();
    for (auto It = PilotSpawns.CreateIterator(); It; ++It)
    {
        auto* Pilot = It.Key().Get();
        if (!Pilot) { It.RemoveCurrent(); continue; }
        FPilotSpawn& Spawn = It.Value();
        if (!Pilot->MissionShipId || Spawn.Remaining <= 0 ||
            Simulation->FindShipByObjID(static_cast<uint32>(Pilot->MissionShipId))) continue;
        if (Spawn.RespawnAt == 0.0)
        {
            Spawn.RespawnAt = Now + 3.0;
            Pilot->RespawnDeadline = Spawn.RespawnAt;
            Pilot->ForceNetUpdate();
        }
        if (Now < Spawn.RespawnAt) continue;
        TOptional<FVector> RespawnPosition;
        const double Clearance = FMath::Min(1500.0, Spawn.Radius * 0.5);
        for (int32 CandidateIndex = 0; CandidateIndex < 12; ++CandidateIndex)
        {
            const double Angle = Spawn.Remaining * 2.399963229728653 + CandidateIndex * (UE_TWO_PI / 12.0);
            const FVector Candidate = Spawn.Position +
                FVector(FMath::Cos(Angle), 0.0, FMath::Sin(Angle)) * Spawn.Radius;
            if (FVector::DistSquared(Candidate, Spawn.LastPosition) <= FMath::Square(Clearance)) continue;
            bool bClear = true;
            for (ListIter<Ship> Ships = Region->GetShips(); ++Ships;)
            {
                const Ship* Other = Ships.value();
                if (Other && !Other->IsDead() && !Other->IsDying() &&
                    FVector::DistSquared(Candidate, Other->GetLocation()) <=
                        FMath::Square(Clearance + Other->GetNavigationHullRadius()))
                {
                    bClear = false;
                    break;
                }
            }
            if (bClear)
            {
                RespawnPosition = Candidate;
                break;
            }
        }
        if (!RespawnPosition.IsSet()) continue;
        Ship* Fighter = Simulation->CreateShip(TCHAR_TO_UTF8(*Spawn.Name), TCHAR_TO_UTF8(*Spawn.Registry),
            Spawn.Design, Region->GetName(), OtherHand(RespawnPosition.GetValue()), Spawn.Iff, Spawn.CommandAI,
            Spawn.Loadout.IsEmpty() ? nullptr : Spawn.Loadout.GetData());
        if (!Fighter) continue;
        Fighter->SetHeading(Spawn.Roll, Spawn.Pitch, Spawn.Yaw);
        Fighter->SetFlightPhase(EOPSMode::ACTIVE);
        if (SimElement* Element = Simulation->FindElement(TCHAR_TO_UTF8(*Spawn.ElementName)))
            Element->AddShip(Fighter, Spawn.ElementIndex);
        Fighter->SetRespawnCount(--Spawn.Remaining);
        if (Spawn.bWasSimulationPlayer) Region->SetPlayerShip(Fighter);
        Pilot->AssignMissionShip(Fighter->GetObjID());
        Spawn.LastPosition = RespawnPosition.GetValue();
        Spawn.RespawnAt = 0.0;
        Pilot->RespawnDeadline = 0.0;
        Pilot->ForceNetUpdate();
    }
}

int32 ASSWSessionLobbyGameMode::CountPendingArenaRespawns(Sim& Simulation, int32 Iff) const
{
    if (!LobbyMission || Simulation.GetMission() != LobbyMission.Get()) return 0;
    int32 Count = 0;
    for (const auto& Entry : PilotSpawns)
    {
        const auto* Pilot = Entry.Key.Get();
        const auto& Spawn = Entry.Value;
        if (Pilot && Pilot->MissionShipId && Spawn.Remaining > 0 && (Iff < 0 || Spawn.Iff == Iff) &&
            !Simulation.FindShipByObjID(static_cast<uint32>(Pilot->MissionShipId)))
            ++Count;
    }
    return Count;
}

void ASSWSessionLobbyGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearTimer(MissionCountdownTimer);
    GetWorldTimerManager().ClearTimer(ArenaTimer);
    GetWorldTimerManager().ClearTimer(DedicatedShutdownTimer);
    for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        if (auto* Pilot = Cast<ASSWSessionLobbyController>(It->Get())) SaveServerPilot(*Pilot, true);
    if (Sim* Simulation = Sim::GetSim(); LobbyMission && Simulation && Simulation->GetMission() == LobbyMission.Get())
    {
        if (auto* Runtime = GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>())
            Runtime->EndMissionPresentation();
        Simulation->UnloadMission();
    }
    LobbyMission.Reset();
    Super::EndPlay(EndPlayReason);
}

void ASSWSessionLobbyController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(ASSWSessionLobbyController, MissionShipId, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(ASSWSessionLobbyController, RespawnDeadline, COND_OwnerOnly);
}

namespace
{
    constexpr int32 FlightAxisBits = 10;
    constexpr int32 FlightAxisMax = 511;
    constexpr uint64 FlightAxisMask = (1ull << FlightAxisBits) - 1;
    constexpr uint64 AugmenterBit = 1ull << 60;
    constexpr uint64 PrimaryFireBit = 1ull << 61;

    uint64 EncodeFlightAxis(double Axis)
    {
        const double Value = FMath::IsFinite(Axis) ? FMath::Clamp(Axis, -1.0, 1.0) : 0.0;
        return uint64(FMath::RoundToInt(Value * FlightAxisMax) + FlightAxisMax);
    }

    double DecodeFlightAxis(uint64 Packed, int32 Index)
    {
        const int32 Value = int32((Packed >> (Index * FlightAxisBits)) & FlightAxisMask);
        return FMath::Clamp(double(Value - FlightAxisMax) / FlightAxisMax, -1.0, 1.0);
    }
}

void ASSWSessionLobbyController::ServerFlightInput_Implementation(uint64 PackedControls, uint8 Throttle)
{
    const double Now = FPlatformTime::Seconds();
    if (Now - LastFlightInput < 0.035) return;

    const auto* State = GetWorld()->GetGameState<ASSWSessionGameState>();
    Ship* Fighter = State && State->bMissionStartReady ? State->FindMissionShip(MissionShipId) : nullptr;
    if (!Fighter || Fighter->IsDead() || Fighter->IsDying()) return;

    LastFlightInput = Now;
    if (!Fighter->IsAutoTrackEngaged())
    {
        Fighter->SetManualThrottle(Throttle * (100.0f / 255.0f));
        Fighter->SetManualAugmenter((PackedControls & AugmenterBit) != 0);
        for (int32 Axis = 0; Axis < 3; ++Axis)
        {
            Fighter->SetManualRotationAxis(Axis, DecodeFlightAxis(PackedControls, Axis));
            Fighter->SetManualTranslationAxis(Axis, DecodeFlightAxis(PackedControls, Axis + 3));
        }
    }
    if (PackedControls & PrimaryFireBit) Fighter->FirePrimary();
}

void ASSWSessionLobbyController::ServerFireSecondary_Implementation()
{
    const auto* State = GetWorld()->GetGameState<ASSWSessionGameState>();
    Ship* Fighter = State && State->bMissionStartReady ? State->FindMissionShip(MissionShipId) : nullptr;
    if (Fighter && !Fighter->IsDead() && !Fighter->IsDying()) Fighter->FireSecondary();
}

void ASSWSessionLobbyController::BindFlightInputs()
{
    auto* LocalPlayer = GetLocalPlayer();
    auto* Input = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
    if (!Input || FlightInputComponent) return;

    FlightInputComponent = NewObject<UEnhancedInputComponent>(this);
    FlightInputComponent->RegisterComponent();
    FlightInputComponent->Priority = 1000;
    PushInputComponent(FlightInputComponent);
    for (const TCHAR* Path : {TEXT("/Game/Input/IMC_FighterHUD.IMC_FighterHUD"), TEXT("/Game/Input/IMC_Game.IMC_Game")})
    {
        auto* Source = LoadObject<UInputMappingContext>(nullptr, Path);
        auto* Context = StarshatterInputBindings::RuntimeContext(Source, this);
        if (!Context) continue;
        for (int32 Index = 0; Index < Context->GetMappings().Num(); ++Index)
            StarshatterInputBindings::ConfigureAugmenterHold(Context->GetMapping(Index), Context);
        FlightInputContexts.Add(Context);
        Input->AddMappingContext(Context, 1000);
    }

    auto* RadioContext = NewObject<UInputMappingContext>(this);
    for (const auto& Context : FlightInputContexts)
        for (const auto& Mapping : Context->GetMappings())
            if (Mapping.Action && Mapping.Action->GetFName() == TEXT("IA_RadioMenu"))
                RadioContext->MapKey(Mapping.Action, Mapping.Key) = Mapping;
    auto* ZoomAction = NewObject<UInputAction>(RadioContext, TEXT("SSW_TargetZoom"), RF_Transient);
    ZoomAction->ValueType = EInputActionValueType::Axis1D;
    RadioContext->MapKey(ZoomAction, EKeys::MouseWheelAxis);
    StarshatterInputBindings::RegisterContext(RadioContext);
    FlightInputContexts.Insert(RadioContext, 0);
    Input->AddMappingContext(RadioContext, 1105);

    auto* TrackingContext = NewObject<UInputMappingContext>(this);
    StarshatterInputBindings::MapTargetTracking(TrackingContext);
    StarshatterInputBindings::RegisterContext(TrackingContext);
    FlightInputContexts.Add(TrackingContext);
    Input->AddMappingContext(TrackingContext, 1000);

    for (const FName Name : {FName(TEXT("IA_Pitch")), FName(TEXT("IA_Yaw")), FName(TEXT("IA_Roll")),
        FName(TEXT("SSW_Strafe")), FName(TEXT("SSW_ForwardThrust")), FName(TEXT("SSW_VerticalThrust")),
        FName(TEXT("IA_Augmenter")), FName(TEXT("IA_Throttle")), FName(TEXT("IA_ThrottleFull")),
        FName(TEXT("IA_ThrottleZero")), FName(TEXT("IA_FirePrimary")), FName(TEXT("IA_FireSecondary")),
        FName(TEXT("IA_NextTarget")), FName(TEXT("IA_PreviousTarget")), FName(TEXT("IA_TargetNearest")),
        FName(TEXT("IA_TargetHostile")), FName(TEXT("IA_DropWeaponTarget")), FName(TEXT("SSW_TargetTracking")),
        FName(TEXT("IA_RadioMenu")), FName(TEXT("SSW_TargetZoom")), FName(TEXT("IA_MFDLeftCycle")),
        FName(TEXT("IA_MFDRightCycle")), FName(TEXT("IA_SensorMode")), FName(TEXT("IA_SensorGroundMode")),
        FName(TEXT("IA_SensorRangeUp")), FName(TEXT("IA_SensorRangeDown")),
        FName(TEXT("IA_EMCONUp")), FName(TEXT("IA_EMCONDown"))})
    {
        for (const auto& Context : FlightInputContexts)
            if (auto* Action = StarshatterInputBindings::FindAction(Context, Name))
            {
                FlightActions.Add(Name, Action);
                const bool bRange = Name == TEXT("IA_SensorRangeUp") || Name == TEXT("IA_SensorRangeDown");
                const bool bContinuous = bRange || Name == TEXT("IA_FirePrimary") || Name == TEXT("SSW_TargetZoom");
                if (Action->ValueType == EInputActionValueType::Boolean || Name == TEXT("IA_Throttle") || bContinuous)
                    FlightInputComponent->BindAction(Action, bContinuous ? ETriggerEvent::Triggered : ETriggerEvent::Started,
                        this, &ThisClass::OnFlightButton, Name);
                if (bRange)
                {
                    FlightInputComponent->BindAction(Action, ETriggerEvent::Completed, this, &ThisClass::ReleaseSensorRange, Name);
                    FlightInputComponent->BindAction(Action, ETriggerEvent::Canceled, this, &ThisClass::ReleaseSensorRange, Name);
                }
                break;
            }
    }
}

bool ASSWSessionLobbyController::InputKey(const FInputKeyEventArgs& Params)
{
    if (Params.Key == EKeys::Escape && FlightCamera && IsLocalController())
    {
        if (Params.Event == IE_Pressed) OpenResignDialog();
        return true;
    }
    return Super::InputKey(Params);
}

void ASSWSessionLobbyController::OnFlightButton(const FInputActionValue& Value, FName Action)
{
    if (Action == TEXT("IA_Throttle"))
    {
        const float Axis = Value.Get<float>();
        if (FMath::Abs(Axis) >= 0.1f)
            FlightThrottle = FMath::Clamp(FlightThrottle + (Axis > 0.0f ? 5.0 : -5.0), 0.0, 100.0);
    }
    else if (Action == TEXT("IA_ThrottleFull")) FlightThrottle = 100.0;
    else if (Action == TEXT("IA_ThrottleZero")) FlightThrottle = 0.0;
    else if (Action == TEXT("IA_FirePrimary")) bPrimaryPressed = true;
    else if (Action == TEXT("IA_FireSecondary")) ServerFireSecondary();
    else if (Action == TEXT("IA_RadioMenu")) OpenMissionChat();
    else if (Action == TEXT("SSW_TargetZoom"))
    {
        const double Wheel = FMath::Clamp(Value.Get<float>(), -10.0f, 10.0f);
        ChaseZoom = FMath::Clamp(ChaseZoom * FMath::Pow(0.85, Wheel), 0.5, 3.0);
    }
    else if (Action == TEXT("IA_MFDLeftCycle") || Action == TEXT("IA_MFDRightCycle"))
    {
        if (FlightMiniHUD) FlightMiniHUD->CycleMFD(Action == TEXT("IA_MFDRightCycle") ? 1 : 0);
    }
    else if (Action == TEXT("IA_SensorRangeUp") || Action == TEXT("IA_SensorRangeDown"))
    {
        if (!HeldSensorRange.Contains(Action))
        {
            HeldSensorRange.Add(Action);
            ServerSensorCommand(Action);
        }
    }
    else if (Action == TEXT("IA_SensorMode"))
    {
        if (!StarshatterInputBindings::IsModifierDown(this, TEXT("Shift")) &&
            !StarshatterInputBindings::IsModifierDown(this, TEXT("Control")))
            ServerSensorCommand(Action);
    }
    else if (Action == TEXT("IA_SensorGroundMode") || Action == TEXT("IA_EMCONUp") || Action == TEXT("IA_EMCONDown"))
        ServerSensorCommand(Action);
    else if (Action != TEXT("IA_Augmenter")) ServerTargetCommand(Action);
}

void ASSWSessionLobbyController::ReleaseSensorRange(const FInputActionValue&, FName Action)
{
    HeldSensorRange.Remove(Action);
}

void ASSWSessionLobbyController::ServerSensorCommand_Implementation(FName Action)
{
    const auto* State = GetWorld()->GetGameState<ASSWSessionGameState>();
    Ship* Fighter = State && State->bMissionStartReady ? State->FindMissionShip(MissionShipId) : nullptr;
    if (!Fighter) return;
    if (Action == TEXT("IA_EMCONUp")) Fighter->SetEMCON(Fighter->GetEMCON() + 1);
    else if (Action == TEXT("IA_EMCONDown")) Fighter->SetEMCON(Fighter->GetEMCON() - 1);
    else if (auto* Sensors = Fighter->GetSensor())
    {
        if (Action == TEXT("IA_SensorRangeUp")) Sensors->IncreaseRange();
        else if (Action == TEXT("IA_SensorRangeDown")) Sensors->DecreaseRange();
        else if (Action == TEXT("IA_SensorGroundMode"))
        {
            if (Fighter->IsAirborne() && Sensors->GetMode() < ESensorMode::PST) Fighter->SetSensorMode(ESensorMode::GM);
        }
        else if (Action == TEXT("IA_SensorMode"))
        {
            const auto Mode = Sensors->GetMode();
            if (Mode >= ESensorMode::PST) Fighter->DropTarget();
            else Fighter->SetSensorMode(Mode == ESensorMode::GM ? ESensorMode::PAS :
                static_cast<ESensorMode>(static_cast<int32>(Mode) + 1));
        }
    }
}

void ASSWSessionLobbyController::ServerSelfDestruct_Implementation()
{
    const auto* State = GetWorld()->GetGameState<ASSWSessionGameState>();
    Ship* Fighter = State && State->bMissionStartReady ? State->FindMissionShip(MissionShipId) : nullptr;
    if (!Fighter || !Fighter->HasNetworkPilot() || Fighter->GetFlightPhase() != EOPSMode::ACTIVE) return;
    Fighter->SetIntegrity(0);
    Fighter->DeathSpiral();
}

void ASSWSessionLobbyController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);

    const auto* State = GetWorld()->GetGameState<ASSWSessionGameState>();
    if (!State) return;
    if (GetNetMode() == NM_Client && IsLocalController() && !bMissionMapRequested && !State->MissionMap.IsEmpty())
    {
        bMissionMapRequested = true;
        bool bStreamingStarted = false;
        ClientMissionLevel = ULevelStreamingDynamic::LoadLevelInstance(GetWorld(), State->MissionMap,
            FVector::ZeroVector, FRotator::ZeroRotator, bStreamingStarted);
        if (bStreamingStarted && ClientMissionLevel)
        {
            ClientMissionLevel->OnLevelShown.AddUniqueDynamic(this, &ThisClass::BuildClientMissionScene);
            if (ClientMissionLevel->IsLevelVisible()) BuildClientMissionScene();
        }
    }
    if (!State->bMissionStartReady)
    {
        if (ClientSceneBuilder.IsValid()) SetLobbySpaceView(*ClientSceneBuilder.Get(), State->ArenaSector);
        return;
    }
    if (!MissionShipId) return;

    const double Now = FPlatformTime::Seconds();
    if (HasAuthority() && LastFlightInput > 0.0 && Now - LastFlightInput > 0.25)
    {
        if (Ship* Fighter = State->FindMissionShip(MissionShipId))
            Fighter->ClearManualRotationInput();
        LastFlightInput = 0.0;
    }
    if (!IsLocalController()) return;

    const auto* Fighter = State->MissionShips.FindByPredicate([this](const FSSWReplicatedShipState& Entry)
    {
        return Entry.ObjectId == MissionShipId;
    });
    if (!Fighter) return;

    if (LastControlledShipId != MissionShipId)
    {
        LastControlledShipId = MissionShipId;
        FlightThrottle = 0.0;
        PendingTranslation = FVector::ZeroVector;
        bPrimaryPressed = false;
    }
    UpdateFlightView(*State, *Fighter, DeltaTime);
    if (MissionChatPanel || ResignDialog) return;
    if (Fighter->bDestroyed || Fighter->bRetired) return;

    if (const auto* Radio = FlightActions.Find(TEXT("IA_RadioMenu"));
        Radio && *Radio && (*Radio)->Triggers.IsEmpty() && (*Radio)->Modifiers.IsEmpty())
        for (const auto& Mapping : FlightInputContexts[0]->GetMappings())
            if (Mapping.Action == *Radio && Mapping.Triggers.IsEmpty() && Mapping.Modifiers.IsEmpty() &&
                WasInputKeyJustPressed(Mapping.Key))
            {
                OpenMissionChat();
                return;
            }

    const auto* Input = Cast<UEnhancedPlayerInput>(PlayerInput);
    const auto Read = [this, Input](FName Name)
    {
        const auto* Action = FlightActions.Find(Name);
        return Input && Action && *Action ? Input->GetActionValue(Action->Get()) : FInputActionValue();
    };
    const FVector Translation(
        Read(TEXT("SSW_Strafe")).Get<float>(),
        Read(TEXT("SSW_ForwardThrust")).Get<float>(),
        Read(TEXT("SSW_VerticalThrust")).Get<float>());
    for (int32 Axis = 0; Axis < 3; ++Axis)
        if (Translation[Axis] != 0.0) PendingTranslation[Axis] = Translation[Axis];
    if (Now < NextFlightInput) return;

    NextFlightInput = Now + 0.05;
    const FVector Rotation(
        Read(TEXT("IA_Pitch")).Get<float>(), Read(TEXT("IA_Yaw")).Get<float>(), Read(TEXT("IA_Roll")).Get<float>());
    const bool bAugmenter = Read(TEXT("IA_Augmenter")).Get<bool>();
    if (bAugmenter) FlightThrottle = 100.0;
    uint64 PackedControls = (bAugmenter ? AugmenterBit : 0) | (bPrimaryPressed ? PrimaryFireBit : 0);
    for (int32 Axis = 0; Axis < 3; ++Axis)
    {
        PackedControls |= EncodeFlightAxis(Rotation[Axis]) << (Axis * FlightAxisBits);
        PackedControls |= EncodeFlightAxis(PendingTranslation[Axis]) << ((Axis + 3) * FlightAxisBits);
    }
    const float SafeThrottle = FMath::IsFinite(FlightThrottle) ? FMath::Clamp(FlightThrottle, 0.0f, 100.0f) : 0.0f;
    ServerFlightInput(PackedControls, uint8(FMath::RoundToInt(SafeThrottle * (255.0f / 100.0f))));
    PendingTranslation = FVector::ZeroVector;
    bPrimaryPressed = false;
}

void ASSWSessionLobbyController::UpdateFlightView(
    const ASSWSessionGameState& State, const FSSWReplicatedShipState& Fighter, float DeltaTime)
{
    const auto ToUE = [](const FVector& V) { return FVector(V.Z, V.X, V.Y); };
    const double Blend = 1.0 - FMath::Exp(-12.0 * FMath::Max(0.0f, DeltaTime));
    constexpr double SnapDistanceSquared = 1000.0 * 1000.0;

    if (!FlightCamera)
    {
        FActorSpawnParameters Params;
        Params.Owner = this;
        FlightCamera = GetWorld()->SpawnActor<ACameraActor>(Params);
        if (!FlightCamera) return;
        FlightCamera->GetCameraComponent()->SetFieldOfView(95.0f);
        if (LobbyDialog) LobbyDialog->RemoveFromParent();
        bShowMouseCursor = false;
        SetInputMode(FInputModeGameOnly());
        BindFlightInputs();
        FlightCamera->SetActorLocation(Fighter.VisualPosition);
        auto* Fill = NewObject<UPointLightComponent>(FlightCamera);
        FlightCamera->AddInstanceComponent(Fill);
        Fill->SetupAttachment(FlightCamera->GetRootComponent());
        Fill->SetMobility(EComponentMobility::Movable);
        Fill->SetRelativeLocation(FVector(0.0, 26.0, 55.0));
        Fill->SetLightingChannels(false, true, false);
        Fill->SetAttenuationRadius(420.0f);
        Fill->SetUseInverseSquaredFalloff(false);
        Fill->SetLightFalloffExponent(2.0f);
        Fill->SetIntensity(3.0f);
        Fill->SetInverseExposureBlend(0.85f);
        Fill->SetCastShadows(false);
        Fill->RegisterComponent();
        SetViewTarget(FlightCamera);
        if (LobbyCamera) { LobbyCamera->Destroy(); LobbyCamera = nullptr; }
        if (auto* Viewport = GetWorld()->GetGameViewport())
        {
            FlightMiniHUD = SNew(SFighterHUDDetails)
                .ShowHUD(true)
                .ThirdPerson(true)
                .NetworkController(TWeakObjectPtr<ASSWSessionLobbyController>(this));
            FlightMiniHUD->SetVisibility(EVisibility::HitTestInvisible);
            Viewport->AddViewportWidgetContent(FlightMiniHUD.ToSharedRef(), 100);
        }
    }

    const bool bInArena = State.ArenaRadius > 0.0 && Fighter.Region == State.ArenaSector;
    if (bInArena && !ArenaBarrier)
    {
        auto* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
        auto* Material = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/sA_EnergyShieldFxPack/Materials/MI_ShieldBase_2_a.MI_ShieldBase_2_a"));
        if (Mesh && Material)
        {
            auto* Barrier = GetWorld()->SpawnActor<AStaticMeshActor>();
            if (Barrier)
            {
                auto* Component = Barrier->GetStaticMeshComponent();
                Component->SetMobility(EComponentMobility::Movable);
                Component->SetStaticMesh(Mesh);
                Component->SetMaterial(0, Material);
                Component->SetReverseCulling(true);
                Component->SetCastShadow(false);
                Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                Barrier->SetActorLocation(State.ArenaVisualCenter);
                Barrier->SetActorScale3D(FVector(State.ArenaRadius / 50.0));
                ArenaBarrier = Barrier;
            }
        }
    }
    if (ArenaBarrier) ArenaBarrier->SetActorHiddenInGame(!bInArena);

    const FVector Forward = ToUE(Fighter.Heading).GetSafeNormal();
    const FVector Up = ToUE(Fighter.Up).GetSafeNormal();
    if (ClientSceneBuilder.IsValid() && Fighter.Region == State.ArenaSector)
        ClientSceneBuilder->UpdateMissionSpace(State.ArenaOrbitalOrigin, ToUE(Fighter.Position));
    const FVector Eye = Fighter.VisualPosition - Forward * (130.0 * ChaseZoom) + Up * 32.0;
    const FRotator View = FRotationMatrix::MakeFromXZ(Forward, Up).Rotator();
    const FVector CameraPosition = FlightCamera->GetActorLocation();
    const bool bSnapCamera = FVector::DistSquared(CameraPosition, Eye) > SnapDistanceSquared;
    FlightCamera->SetActorLocationAndRotation(
        bSnapCamera ? Eye : FMath::Lerp(CameraPosition, Eye, Blend),
        bSnapCamera ? View.Quaternion() : FQuat::Slerp(FlightCamera->GetActorQuat(), View.Quaternion(), Blend));

    if (HasAuthority()) return;
    TSet<int64> Visible;
    for (const auto& ShipState : State.MissionShips)
    {
        if (ShipState.bRetired || ShipState.bDestroyed || ShipState.Region != Fighter.Region) continue;
        Visible.Add(ShipState.ObjectId);
        auto& Actor = RemoteFighters.FindOrAdd(ShipState.ObjectId);
        const FRotator Rotation = FRotationMatrix::MakeFromXZ(ToUE(ShipState.Heading), ToUE(ShipState.Up)).Rotator();
        if (!Actor)
        {
            const FShipDesign* Row = ShipDesignRegistry::Find(ShipState.Design);
            UClass* VisualClass = Row ? ShipVisualCatalog::Resolve(ShipState.Design, Row->Model) : nullptr;
            if (!VisualClass) continue;

            FActorSpawnParameters Spawn;
            Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            Actor = GetWorld()->SpawnActor<AShipActor>(VisualClass, ShipState.VisualPosition, Rotation, Spawn);
            if (!Actor) continue;
            Actor->SetActorEnableCollision(false);
            if (auto* Visual = Cast<AFighterActor>(Actor.Get())) Visual->UseNetworkChaseView();
            Actor->InitializeSnapshotPropulsion(ShipDesign::GetDesignName(TCHAR_TO_UTF8(*ShipState.Design)));
        }
        const FVector CurrentPosition = Actor->GetActorLocation();
        const bool bSnapShip = FVector::DistSquared(CurrentPosition, ShipState.VisualPosition) > SnapDistanceSquared;
        Actor->SetActorLocationAndRotation(
            bSnapShip ? ShipState.VisualPosition : FMath::Lerp(CurrentPosition, ShipState.VisualPosition, Blend),
            bSnapShip ? Rotation.Quaternion() : FQuat::Slerp(Actor->GetActorQuat(), Rotation.Quaternion(), Blend));
        Actor->UpdateSnapshotPropulsion(DeltaTime, ShipState.DrivePower, ShipState.Throttle,
            ShipState.ThrusterBurns, ShipState.EnabledNavLights);
        Actor->UpdateSnapshotStores(ShipState.CarriedStores);
    }
    for (auto It = RemoteFighters.CreateIterator(); It; ++It)
    {
        if (Visible.Contains(It.Key())) continue;
        if (It.Value()) It.Value()->Destroy();
        It.RemoveCurrent();
    }
}

void ASSWSessionLobbyController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CloseResignDialog();
    CloseMissionChat();
    if (FlightInputComponent) PopInputComponent(FlightInputComponent);
    if (auto* LocalPlayer = GetLocalPlayer())
        if (auto* Input = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
            for (const auto& Context : FlightInputContexts) Input->RemoveMappingContext(Context);
    if (HasAuthority()) AssignMissionShip(0);
    if (FlightMiniHUD.IsValid())
    {
        if (auto* Viewport = GetWorld()->GetGameViewport())
            Viewport->RemoveViewportWidgetContent(FlightMiniHUD.ToSharedRef());
        FlightMiniHUD.Reset();
    }
    if (FlightCamera) FlightCamera->Destroy();
    if (LobbyCamera) LobbyCamera->Destroy();
    if (ArenaBarrier) ArenaBarrier->Destroy();
    for (auto& Entry : RemoteFighters)
        if (Entry.Value) Entry.Value->Destroy();
    if (LobbyDialog) LobbyDialog->RemoveFromParent();
    Super::EndPlay(EndPlayReason);
}

namespace
{
    FString OrbitalPositionKey(const Orbital& Body)
    {
        const FString System = Body.System() ? UTF8_TO_TCHAR(Body.System()->GetName()) : TEXT("");
        FString Path;
        for (const Orbital* Parent = &Body; Parent; Parent = Parent->Primary())
            Path = FString::Printf(TEXT("/%d:%s"), int32(Parent->GetType()), UTF8_TO_TCHAR(Parent->GetName())) + Path;
        return System + Path;
    }

    void AppendOrbitalBody(OrbitalBody& Body, TArray<FSSWOrbitalPosition>& Positions)
    {
        Positions.Add({OrbitalPositionKey(Body), Body.Location(), Body.Phase()});
        for (auto Moons = Body.Satellites(); ++Moons;)
            if (auto* Moon = Moons.value()) AppendOrbitalBody(*Moon, Positions);
    }
}

void ASSWSessionGameState::PublishGalaxy(double Now)
{
    if (!HasAuthority() || Now < NextGalaxyUpdate) return;
    NextGalaxyUpdate = Now + 0.5;

    const auto* Environment = GetGameInstance()->GetSubsystem<UStarshatterEnvironmentSubsystem>();
    if (!Environment) return;

    GalaxyClockMs = Environment->GetSimulationClockMs();
    OrbitalPositions.Reset();
    for (StarSystem* System : Environment->GetRuntimeStarSystems())
    {
        if (!System) continue;
        OrbitalPositions.Add({UTF8_TO_TCHAR(System->GetName()), System->GetLocation(), 0.0});
        for (ListIter<OrbitalBody> Bodies = System->GetBodies(); ++Bodies;)
        {
            if (auto* Body = Bodies.value())
            {
                Body->Update();
                AppendOrbitalBody(*Body, OrbitalPositions);
            }
        }
        for (ListIter<OrbitalRegion> Regions = System->GetAllRegions(); ++Regions;)
        {
            if (auto* Region = Regions.value())
            {
                Region->Update();
                OrbitalPositions.Add({OrbitalPositionKey(*Region), Region->Location(), Region->Phase()});
            }
        }
    }
}

const FSSWOrbitalPosition* ASSWSessionGameState::FindOrbitalPosition(const Orbital& Body) const
{
    const int32* Index = OrbitalPositionIndices.Find(OrbitalPositionKey(Body));
    return Index && OrbitalPositions.IsValidIndex(*Index) ? &OrbitalPositions[*Index] : nullptr;
}

void ASSWSessionGameState::OnRep_OrbitalPositions()
{
    OrbitalPositionIndices.Reset();
    for (int32 Index = 0; Index < OrbitalPositions.Num(); ++Index)
        OrbitalPositionIndices.Add(OrbitalPositions[Index].Key, Index);

    const auto* Environment = GetGameInstance()->GetSubsystem<UStarshatterEnvironmentSubsystem>();
    if (!Environment) return;
    for (StarSystem* System : Environment->GetRuntimeStarSystems())
    {
        if (!System) continue;
        if (const int32* Index = OrbitalPositionIndices.Find(UTF8_TO_TCHAR(System->GetName())))
            System->SetLocation(OrbitalPositions[*Index].Position);
        for (ListIter<OrbitalBody> Bodies = System->GetBodies(); ++Bodies;)
            if (auto* Body = Bodies.value()) Body->Update();
        for (ListIter<OrbitalRegion> Regions = System->GetAllRegions(); ++Regions;)
            if (auto* Region = Regions.value()) Region->Update();
    }
}

void ASSWSessionLobbyController::ServerTargetCommand_Implementation(FName Action)
{
    const auto* State = GetWorld()->GetGameState<ASSWSessionGameState>();
    Ship* Fighter = State && State->bMissionStartReady ? State->FindMissionShip(MissionShipId) : nullptr;
    if (!Fighter || Fighter->IsDead() || Fighter->IsDying()) return;

    if (Action == TEXT("SSW_TargetTracking"))
    {
        Fighter->SetAutoTrack(!Fighter->IsAutoTrackEngaged());
        return;
    }
    if (Action == TEXT("IA_DropWeaponTarget"))
    {
        Fighter->DropTarget();
        return;
    }
    if (Action == TEXT("IA_TargetNearest") || Action == TEXT("IA_TargetHostile"))
    {
        Fighter->LockTarget(ESimObject::SHIP, true, Action == TEXT("IA_TargetHostile"));
        return;
    }
    const bool bPrevious = Action == TEXT("IA_PreviousTarget");
    SimRegion* Region = Fighter->GetRegion();
    if (!Region || (!bPrevious && Action != TEXT("IA_NextTarget"))) return;

    TArray<Ship*> Candidates;
    int32 CurrentIndex = INDEX_NONE;
    for (ListIter<Ship> Ships = Region->GetShips(); ++Ships;)
    {
        Ship* Candidate = Ships.value();
        if (!Candidate || Candidate == Fighter || Candidate->IsDead() || Candidate->IsDying() ||
            Candidate->GetIFF() == Fighter->GetIFF() || Candidate->GetIFF() == 0) continue;
        if (Candidate == Fighter->GetTarget()) CurrentIndex = Candidates.Num();
        Candidates.Add(Candidate);
    }
    if (Candidates.IsEmpty()) return;
    const int32 Offset = bPrevious ? -1 : 1;
    const int32 NextIndex = CurrentIndex == INDEX_NONE ? (bPrevious ? Candidates.Num() - 1 : 0) :
        (CurrentIndex + Offset + Candidates.Num()) % Candidates.Num();
    Fighter->LockTarget(Candidates[NextIndex]);
}

void ASSWSessionFlightHUD::DrawHUD()
{
    Super::DrawHUD();
    const auto* Controller = Cast<ASSWSessionLobbyController>(GetOwningPlayerController());
    const auto* State = GetWorld()->GetGameState<ASSWSessionGameState>();
    if (!Canvas || !Controller || !State || !State->bMissionStartReady) return;

    const int64 FighterId = Controller->GetMissionShipId();
    const auto* Fighter = State->MissionShips.FindByPredicate([FighterId](const FSSWReplicatedShipState& Entry)
    {
        return Entry.ObjectId == FighterId;
    });
    const FLinearColor TextColor(0.7f, 0.9f, 1.0f);
    const FLinearColor HitColor(1.0f, 0.35f, 0.2f);
    if (const double Deadline = Controller->GetRespawnDeadline(); Deadline > 0.0)
    {
        const int32 Seconds = FMath::Max(1, FMath::CeilToInt(Deadline - State->GetServerWorldTimeSeconds()));
        const FString Message = FString::Printf(TEXT("RESPAWN IN %d"), Seconds);
        float Width = 0.0f, Height = 0.0f;
        GetTextSize(Message, Width, Height, nullptr, 2.0f);
        DrawText(Message, TextColor, (Canvas->ClipX - Width) * 0.5f,
            (Canvas->ClipY - Height) * 0.5f, nullptr, 2.0f);
    }
    float NoticeY = Canvas->ClipY * 0.22f;
    const double Now = FPlatformTime::Seconds();
    for (const auto& Notice : Controller->GetDepartureNotices())
    {
        if (Notice.Key <= Now) continue;
        float Width = 0.0f, Height = 0.0f;
        GetTextSize(Notice.Value, Width, Height, nullptr, 1.15f);
        const float NoticeX = (Canvas->ClipX - Width) * 0.5f;
        DrawText(Notice.Value, FLinearColor::Black, NoticeX + 1.0f, NoticeY + 1.0f, nullptr, 1.15f);
        DrawText(Notice.Value, TextColor, NoticeX, NoticeY, nullptr, 1.15f);
        NoticeY += Height + 4.0f;
    }
    if (!Fighter) return;
    const float PanelScale = FMath::Min(Canvas->ClipX / 1280.0f, Canvas->ClipY / 720.0f);
    const float ScoreTop = Canvas->ClipY - 215.0f * PanelScale - 125.0f;
    float Row = ScoreTop;
    for (const auto& Entry : State->MissionShips)
    {
        if (Entry.Region != Fighter->Region) continue;
        const TCHAR* Status = Entry.bDestroyed ? TEXT(" DESTROYED") : Entry.bRetired ? TEXT(" RETIRED") : TEXT("");
        const bool bTarget = Entry.ObjectId == Fighter->TargetId;
        DrawText(FString::Printf(TEXT("%s%s   HULL %.1f   SHIELDS %d   HITS TAKEN %d   SHOTS %d%s"),
            bTarget ? TEXT("> ") : TEXT(""), *Entry.Name, Entry.Integrity,
            Entry.ShieldStrength, Entry.DamageEvents, Entry.ShotsFired, Status),
            bTarget || Entry.bDestroyed ? HitColor : TextColor, 24.0f, Row);
        Row += 18.0f;
    }

    if (Fighter->bDestroyed || Fighter->bRetired) return;
    const float X = Canvas->ClipX * 0.5f;
    const float Y = Canvas->ClipY * 0.5f;
    DrawLine(X - 12.0f, Y, X + 12.0f, Y, TextColor);
    DrawLine(X, Y - 12.0f, X, Y + 12.0f, TextColor);

    FVector Eye;
    FRotator View;
    Controller->GetPlayerViewPoint(Eye, View);
    const FRotationMatrix CameraBasis(View);
    const FVector Right = CameraBasis.GetScaledAxis(EAxis::Y);
    const FVector Up = CameraBasis.GetScaledAxis(EAxis::Z);
    constexpr double Margin = 75.0;
    const FVector2D Bounds(FMath::Max(1.0, X - Margin), FMath::Max(1.0, Y - Margin));
    for (const auto& Contact : State->MissionShips)
    {
        if (Contact.ObjectId == FighterId || Contact.bDestroyed || Contact.bRetired ||
            Contact.Region != Fighter->Region) continue;

        FVector2D Point;
        const bool bProjected = Controller->ProjectWorldLocationToScreen(Contact.VisualPosition, Point);
        if (!bProjected || FMath::Abs(Point.X - X) > Bounds.X || FMath::Abs(Point.Y - Y) > Bounds.Y)
        {
            if (!Contact.bPlayerControlled && Contact.ObjectId != Fighter->TargetId) continue;
            const FVector Delta = Contact.VisualPosition - Eye;
            FVector2D Direction = bProjected ? Point - FVector2D(X, Y) :
                FVector2D(FVector::DotProduct(Delta, Right), -FVector::DotProduct(Delta, Up));
            if (Direction.IsNearlyZero()) Direction.Y = 1.0;
            const double Edge = FMath::Max(FMath::Abs(Direction.X) / Bounds.X,
                FMath::Abs(Direction.Y) / Bounds.Y);
            Point = FVector2D(X, Y) + Direction / Edge;
        }

        const FLinearColor Marker = Contact.ObjectId == Fighter->TargetId ? HitColor : TextColor;
        const bool bLocked = Contact.ObjectId == Fighter->TargetId;
        const float Radius = bLocked ? 13.0f : 8.0f;
        DrawLine(Point.X, Point.Y - Radius, Point.X + Radius, Point.Y, Marker);
        DrawLine(Point.X + Radius, Point.Y, Point.X, Point.Y + Radius, Marker);
        DrawLine(Point.X, Point.Y + Radius, Point.X - Radius, Point.Y, Marker);
        DrawLine(Point.X - Radius, Point.Y, Point.X, Point.Y - Radius, Marker);
        const double RangeKm = FVector::Dist(Contact.Position, Fighter->Position) / 1000.0;
        const TCHAR* Tracking = bLocked ? (Fighter->bAutoTracking ? TEXT("  AUTO TRACK") : TEXT("  LOCKED")) : TEXT("");
        const FString Caption = FString::Printf(TEXT("%s  %.3f km%s"), *Contact.GetPilotLabel(), RangeKm, Tracking);
        float Width = 0.0f, Height = 0.0f;
        GetTextSize(Caption, Width, Height);
        const float CaptionX = FMath::Clamp(static_cast<float>(Point.X + 13.0), 0.0f,
            FMath::Max(0.0f, Canvas->ClipX - Width));
        DrawText(Caption, Marker, CaptionX, Point.Y + 12.0f);
    }

    for (const auto& Shot : State->MissionProjectiles)
    {
        if (Shot.Region != Fighter->Region) continue;
        FVector2D Start, End;
        if (Controller->ProjectWorldLocationToScreen(Shot.VisualStart, Start) &&
            Controller->ProjectWorldLocationToScreen(Shot.VisualEnd, End))
            DrawLine(Start.X, Start.Y, End.X, End.Y, Shot.bMissile ? HitColor : TextColor, 2.0f);
    }
}

void ASSWSessionLobbyGameMode::PreLogin(const FString& Options, const FString& Address,
    const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
    Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
    if (!ErrorMessage.IsEmpty()) return;
    if (GetNetMode() == NM_DedicatedServer && (bDedicatedShutdownStarted || HasMatchEnded()))
        ErrorMessage = TEXT("This match has ended.");
    else if (GetNumPlayers() + GetNumSpectators() >= PlayerCapacity)
        ErrorMessage = TEXT("This mission is full.");
}

void ASSWSessionLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    if (GetNetMode() != NM_DedicatedServer) return;

    if (!HasMatchEnded())
        GetWorldTimerManager().ClearTimer(DedicatedShutdownTimer);
    auto* Pilot = Cast<ASSWSessionLobbyController>(NewPlayer);
    if (Pilot) SaveServerPilot(*Pilot, false);
    const auto* State = GetGameState<ASSWSessionGameState>();
    Sim* Simulation = Sim::GetSim();
    if (!Pilot || !State || !State->bMissionStartReady || !Simulation) return;

    for (ListIter<SimRegion> Regions = Simulation->GetRegions(); ++Regions;)
    {
        if (!Regions.value()) continue;
        for (ListIter<Ship> Ships = Regions->GetShips(); ++Ships;)
        {
            Ship* Fighter = Ships.value();
            if (Fighter && Fighter->GetElement() && Fighter->GetElement()->IsPlayable() &&
                !Fighter->IsStatic() &&
                Pilot->AssignMissionShip(Fighter->GetObjID())) return;
        }
    }
}

void ASSWSessionLobbyGameMode::Logout(AController* Exiting)
{
    auto* Pilot = Cast<ASSWSessionLobbyController>(Exiting);
    if (Pilot) SaveServerPilot(*Pilot, true);
    const auto* PilotState = Pilot ? Pilot->GetPlayerState<APlayerState>() : nullptr;
    if (PilotState && !GetWorld()->bIsTearingDown)
    {
        if (auto* State = GetGameState<ASSWSessionGameState>())
        {
            const FString Message = FString::Printf(TEXT("%s left the game."), *PilotState->GetPlayerName());
            if (State->bMissionStartReady)
            {
                PilotSpawns.Remove(Pilot);
                const int64 ShipId = Pilot->GetMissionShipId();
                Pilot->AssignMissionShip(0);
                if (Sim* Simulation = Sim::GetSim(); Simulation && Simulation->GetMission() == LobbyMission.Get() &&
                    ShipId > 0 && ShipId <= MAX_uint32)
                {
                    if (Ship* Fighter = Simulation->FindShipByObjID(static_cast<uint32>(ShipId)))
                    {
                        if (SimRegion* Region = Fighter->GetRegion())
                            for (ListIter<Ship> Ships = Region->GetShips(); ++Ships;)
                                if (Ship* Remaining = Ships.value(); Remaining && Remaining->GetTarget() == Fighter)
                                    Remaining->LockTarget(static_cast<SimObject*>(nullptr));
                        Fighter->SetRespawnCount(0);
                        if (SimElement* Element = Fighter->GetElement()) Element->DelShip(Fighter);
                        Simulation->DestroyShip(Fighter);
                    }
                }
                for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
                    if (auto* Remaining = Cast<ASSWSessionLobbyController>(It->Get()); Remaining && Remaining != Exiting)
                        Remaining->ReceivePilotDeparture(Message);
            }
            else
                State->AddChatMessage(TEXT("System"), Message);
        }
    }
    Super::Logout(Exiting);
    if (GetNetMode() == NM_DedicatedServer && !GetWorld()->bIsTearingDown &&
        GetNumPlayers() + GetNumSpectators() == 0)
        ScheduleDedicatedShutdown();
}

void ASSWSessionLobbyGameMode::HandleMatchHasEnded()
{
    Super::HandleMatchHasEnded();
    if (GetNetMode() == NM_DedicatedServer)
    {
        // A concluded match gets its own full grace period.
        GetWorldTimerManager().ClearTimer(DedicatedShutdownTimer);
        ScheduleDedicatedShutdown();
    }
}

void ASSWSessionLobbyGameMode::ScheduleDedicatedShutdown()
{
    if (bDedicatedShutdownStarted || GetWorldTimerManager().IsTimerActive(DedicatedShutdownTimer))
        return;

    UE_LOG(LogTemp, Display, TEXT("[StarServer] Shutdown scheduled in 60 seconds."));
    GetWorldTimerManager().SetTimer(DedicatedShutdownTimer, this, &ThisClass::ShutdownDedicatedServer, 60.f);
}

void ASSWSessionLobbyGameMode::ShutdownDedicatedServer()
{
    if (!HasMatchEnded() && GetNumPlayers() + GetNumSpectators() > 0)
        return;

    bDedicatedShutdownStarted = true;
    if (auto* Sessions = GetGameInstance()->GetSubsystem<USSWSessionSubsystem>())
        Sessions->CloseDedicatedSessionAndExit();
    else
        FPlatformMisc::RequestExit(false);
}

void ASSWSessionLobbyController::ServerSetPlatformPilotName_Implementation(const FString& DisplayName)
{
    auto* PilotState = GetPlayerState<ASSWSessionPlayerState>();
    if (!PilotState || DisplayName.Len() > 64) return;

    FString Name = DisplayName.TrimStartAndEnd();
    for (TCHAR CodeUnit : Name)
        if (FChar::IsControl(CodeUnit)) return;

    if (Name.IsEmpty() || PilotState->GetPlayerName() == Name) return;
    PilotState->SetPlayerName(Name);
    PilotState->OnRep_Ready();
    PilotState->ForceNetUpdate();
}

void ASSWSessionPlayerState::OnRep_PlayerName()
{
    Super::OnRep_PlayerName();
    if (auto* State = GetWorld()->GetGameState<ASSWSessionGameState>())
        State->OnRosterChanged.Broadcast();
}

void ASSWSessionLobbyController::BuildClientMissionScene()
{
    const auto* State = GetWorld()->GetGameState<ASSWSessionGameState>();
    if (!ClientMissionLevel || !State) return;

    ClientMissionLevel->OnLevelShown.RemoveDynamic(this, &ThisClass::BuildClientMissionScene);
    if (auto* Builder = BuildLobbySystem(*GetWorld(), *ClientMissionLevel, State->MissionMap))
    {
        ClientSceneBuilder = Builder;
        SetLobbySpaceView(*Builder, State->ArenaSector);
    }
    else
        UE_LOG(LogTemp, Warning, TEXT("[SessionLobby] Could not build client scene for %s"), *State->MissionMap);
}

void ASSWSessionLobbyController::SetLobbySpaceView(const ASystemSceneBuilder& Builder, const FString& Sector)
{
    if (FlightCamera || !IsLocalController() || Sector.IsEmpty()) return;
    const auto* Region = Builder.GetSpawnedRegions().FindByPredicate([&Sector](const FSpawnedSystemRegion& Entry)
    {
        return Entry.RegionName.Equals(Sector, ESearchCase::IgnoreCase);
    });
    const FString& Anchor = Region && !Region->AnchorBodyName.IsEmpty() ? Region->AnchorBodyName : Sector;
    const auto& Bodies = Builder.GetSpawnedBodies();
    const auto* Body = Bodies.FindByPredicate([&Anchor](const FSpawnedSystemBody& Entry)
    {
        return Entry.Actor && !Entry.bIsOrbit && Entry.BodyName.Equals(Anchor, ESearchCase::IgnoreCase);
    });
    if (!Body) return;
    const FVector Target = Body->Actor->GetActorLocation();
    const AActor* Illuminator = Body->ParentActor;
    const FVector TowardLight = Illuminator
        ? (Illuminator->GetActorLocation() - Target).GetSafeNormal() : FVector::BackwardVector;
    FVector ViewDirection = TowardLight;
    if (Anchor.Equals(TEXT("Borova"), ESearchCase::IgnoreCase))
    {
        const auto* State = GetWorld()->GetGameState<ASSWSessionGameState>();
        const auto* Pilot = GetPlayerState<ASSWSessionPlayerState>();
        int32 ViewIndex = 0;
        if (State && Pilot)
            for (const APlayerState* Entry : State->PlayerArray)
                if (Entry && Entry->GetPlayerId() < Pilot->GetPlayerId()) ++ViewIndex;

        // Fan the four lobby cameras across the sunlit side in stable pilot-ID order.
        constexpr double ViewAngles[] = {-55.0, 55.0, -20.0, 20.0};
        const FVector Axis = FVector::CrossProduct(TowardLight, FVector::RightVector).GetSafeNormal();
        ViewDirection = TowardLight.RotateAngleAxis(ViewAngles[ViewIndex % UE_ARRAY_COUNT(ViewAngles)],
            Axis.IsNearlyZero() ? FVector::UpVector : Axis);
    }
    const FVector Offset = (ViewDirection * 4.0 + FVector::UpVector * 0.85) *
        FMath::Max(100.0f, Body->VisualRadiusUnits);
    if (LobbyCamera)
    {
        LobbyCamera->SetActorLocationAndRotation(Target + Offset, (-Offset).Rotation());
        if (GetViewTarget() != LobbyCamera) SetViewTarget(LobbyCamera);
        return;
    }
    LobbyCamera = GetWorld()->SpawnActor<ACameraActor>(Target + Offset, (-Offset).Rotation());
    if (LobbyCamera)
    {
        SetViewTarget(LobbyCamera);
        UE_LOG(LogTemp, Display, TEXT("[SessionLobby] Lobby view sector='%s' body='%s' camera=%s"),
            *Sector, *Body->BodyName, *(Target + Offset).ToString());
    }
}

const FSSWReplicatedShipState* ASSWSessionLobbyController::GetAssignedShipState() const
{
    const auto* State = GetWorld()->GetGameState<ASSWSessionGameState>();
    return State ? State->MissionShips.FindByPredicate([this](const FSSWReplicatedShipState& Entry)
    {
        return Entry.ObjectId == MissionShipId;
    }) : nullptr;
}

const TArray<FSSWReplicatedShipState>* ASSWSessionLobbyController::GetFlightContacts() const
{
    const auto* State = GetWorld()->GetGameState<ASSWSessionGameState>();
    return State ? &State->MissionShips : nullptr;
}
