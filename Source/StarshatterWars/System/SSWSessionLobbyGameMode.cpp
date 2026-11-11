#include "SSWSessionLobbyGameMode.h"
#include "MultiplayerDlg.h"
#include "GameFramework/SpectatorPawn.h"
#include "GameFramework/GameState.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameSession.h"
#include "SSWRuntimeSubsystem.h"
#include "SSWGameInstance.h"
#include "Engine/GameInstance.h"
#include "Net/UnrealNetwork.h"
#include "Sim.h"
#include "SimShot.h"
#include "Ship.h"
#include "SimSystem.h"
#include "Instruction.h"
#include "TimerManager.h"
#include "Mission.h"
#include "Engine/LevelStreamingDynamic.h"
#include "EngineUtils.h"
#include "SystemSceneBuilder.h"
#include "Camera/CameraActor.h"
#include "StarSystem.h"
#include "OrbitalBody.h"
#include "OrbitalRegion.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "ShipActor.h"
#include "ShipDesign.h"
#include "Weapon.h"
#include "SSWSessionSubsystem.h"
#include "Engine/Canvas.h"
#include "ShipDesignRegistry.h"
#include "ShipVisualCatalog.h"
#include "StarshatterEnvironmentSubsystem.h"
#include "StarshatterGameDataSubsystem.h"
#include "SimElement.h"
ASSWSessionLobbyGameMode::ASSWSessionLobbyGameMode(){PlayerControllerClass=ASSWSessionLobbyController::StaticClass();DefaultPawnClass=ASpectatorPawn::StaticClass();GameStateClass=ASSWSessionGameState::StaticClass();GameSessionClass=AGameSession::StaticClass();PlayerStateClass=ASSWSessionPlayerState::StaticClass();HUDClass=ASSWSessionFlightHUD::StaticClass();bUseSeamlessTravel=true;}
void ASSWSessionLobbyController::BeginPlay(){Super::BeginPlay();if(!IsLocalController())return;
 if (const auto* Instance = Cast<USSWGameInstance>(GetGameInstance()))
 {
     const FString Name = Instance->GetPlatformPilotName().TrimStartAndEnd().Left(64);
     if (!Name.IsEmpty()) ServerSetPlatformPilotName(Name);
 }
 if (GetNetMode() == NM_Client)
 {
     bool bStreamingStarted = false;
     ULevelStreamingDynamic::LoadLevelInstance(GetWorld(), TEXT("/Game/Maps/Borova"),
         FVector::ZeroVector, FRotator::ZeroRotator, bStreamingStarted);
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
    State.ShieldStrength = PilotShip.GetShieldStrength();
    State.FuelPercent = PilotShip.GetFuelLevel();
    State.Throttle = static_cast<float>(PilotShip.GetThrottle());
    if (const Weapon* Primary = PilotShip.GetPrimary()) State.PrimaryAmmo = Primary->Ammo();
    if (const Weapon* Secondary = PilotShip.GetSecondary()) State.SecondaryAmmo = Secondary->Ammo();
    if (const SimObject* Target = PilotShip.GetTarget()) State.TargetId = Target->GetObjID();
    State.Emcon = PilotShip.GetEMCON();
    State.FlightPhase = static_cast<int32>(PilotShip.GetFlightPhase());
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
    State->FindMissionShip(ObjectId)->SetNetworkPilot(true);
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
    if (!State || !Sender || !AcceptChat(Message)) return;

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
    if (auto* State = GetPlayerState<ASSWSessionPlayerState>())
    {
        State->bReady = bReady;
        State->OnRep_Ready();
        State->ForceNetUpdate();
    }
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
        if (auto* Sessions = GetGameInstance()->GetSubsystem<USSWSessionSubsystem>())
            Sessions->AdvertiseDedicatedLAN();
    }
    else
    {
        bool bStreamingStarted = false;
        LobbySystemLevel = ULevelStreamingDynamic::LoadLevelInstance(
            GetWorld(), TEXT("/Game/Maps/Borova"), FVector::ZeroVector, FRotator::ZeroRotator, bStreamingStarted);
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
        if (auto* Runtime = GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>())
            Runtime->SetPaused(GetNumPlayers() == 0);
        return;
    }

    int32 PilotCount = 0;
    bool bAllReady = true;
    for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        const auto* Controller = Cast<ASSWSessionLobbyController>(It->Get());
        if (!Controller) continue;
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
    const FS_CampaignMission* Scenario = Data ? Data->GetMultiplayerMissionScenarios().FindByPredicate(
        [](const FS_CampaignMission& Row) { return Row.MissionName.Equals(TEXT("Hawk Match"), ESearchCase::IgnoreCase); }) : nullptr;

    State.MissionLaunchError = TEXT("HAWK MATCH MISSION UNAVAILABLE");
    if (!Runtime || !Environment || !Scenario) return false;

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

    ASystemSceneBuilder* Builder = nullptr;
    if (LobbySystemLevel && LobbySystemLevel->IsLevelVisible())
    {
        for (TActorIterator<ASystemSceneBuilder> It(GetWorld()); It; ++It)
        {
            if (It->GetLevel() == LobbySystemLevel->GetLoadedLevel() && !It->GetSpawnedRegions().IsEmpty())
            {
                Builder = *It;
                break;
            }
        }
    }
    if (!Builder && GetNetMode() != NM_DedicatedServer)
    {
        State.MissionLaunchError = TEXT("BOROVA SCENE IS UNAVAILABLE OR STILL LOADING");
        return false;
    }

    Simulation->SetNetworkWorld(GetWorld());
    State.BindSimulation(*Simulation);
    Environment->BuildSimRegionsForSim(Simulation);
    if (!Simulation->FindRegion(Selected->GetRegion()))
    {
        State.MissionLaunchError = TEXT("HAWK MATCH REGION UNAVAILABLE");
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
                Candidate->GetElement()->IsPlayable() && !Candidate->IsStarship() && !Candidate->IsDropship())
                Fighters.Add(Candidate);
    }

    if (Ship* LocalFighter = Simulation->GetPlayerShip())
        Fighters.Sort([LocalFighter](const Ship& A, const Ship& B) {
            return (&A == LocalFighter) > (&B == LocalFighter);
        });

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
    State.MissionLaunchError.Reset();
    Runtime->SetGameMode(EGameMode::PLAY);
    Runtime->SetPaused(false);
    return true;
}

void ASSWSessionLobbyGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearTimer(MissionCountdownTimer);
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
}

void ASSWSessionLobbyController::ServerFlightInput_Implementation(FVector Rotation, float Throttle, bool bFirePrimary, bool bFireSecondary)
{
    const double Now = FPlatformTime::Seconds();
    if (Rotation.ContainsNaN() || !FMath::IsFinite(Throttle) || Now - LastFlightInput < 0.035) return;

    const auto* State = GetWorld()->GetGameState<ASSWSessionGameState>();
    Ship* Fighter = State && State->bMissionStartReady ? State->FindMissionShip(MissionShipId) : nullptr;
    if (!Fighter) return;

    LastFlightInput = Now;
    Fighter->SetManualThrottle(FMath::Clamp(Throttle, 0.0f, 100.0f));
    for (int32 Axis = 0; Axis < 3; ++Axis)
        Fighter->SetManualRotationAxis(Axis, FMath::Clamp(Rotation[Axis], -1.0, 1.0));
    if (bFirePrimary) Fighter->FirePrimary();
    if (bFireSecondary) Fighter->FireSecondary();
}

void ASSWSessionLobbyController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);

    const auto* State = GetWorld()->GetGameState<ASSWSessionGameState>();
    if (!State || !State->bMissionStartReady || !MissionShipId) return;

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

    UpdateFlightView(*State, *Fighter, DeltaTime);
    if (Fighter->bDestroyed || Fighter->bRetired || Now < NextFlightInput) return;

    NextFlightInput = Now + 0.05;
    const FVector Rotation(
        int32(IsInputKeyDown(EKeys::Down)) - int32(IsInputKeyDown(EKeys::Up)),
        int32(IsInputKeyDown(EKeys::Right)) - int32(IsInputKeyDown(EKeys::Left)),
        int32(IsInputKeyDown(EKeys::NumPadNine)) - int32(IsInputKeyDown(EKeys::NumPadSeven)));
    const int32 ThrottleInput = int32(IsInputKeyDown(EKeys::A)) - int32(IsInputKeyDown(EKeys::Z));
    FlightThrottle = FMath::Clamp(FlightThrottle + 2.5 * ThrottleInput, 0.0, 100.0);
    ServerFlightInput(Rotation, FlightThrottle, IsInputKeyDown(EKeys::LeftControl), IsInputKeyDown(EKeys::SpaceBar));

    const bool bTargetPressed = IsInputKeyDown(EKeys::T);
    if (bTargetPressed && !bTargetKeyDown) ServerCycleCombatTarget();
    bTargetKeyDown = bTargetPressed;
}

void ASSWSessionLobbyController::UpdateFlightView(
    const ASSWSessionGameState& State, const FSSWReplicatedShipState& Fighter, float DeltaTime)
{
    const auto ToUE = [](const FVector& V) { return FVector(V.Z, V.X, V.Y); };

    if (!FlightCamera)
    {
        FActorSpawnParameters Params;
        Params.Owner = this;
        FlightCamera = GetWorld()->SpawnActor<ACameraActor>(Params);
        if (!FlightCamera) return;
        if (LobbyDialog) LobbyDialog->RemoveFromParent();
        bShowMouseCursor = false;
        SetInputMode(FInputModeGameOnly());
        FlightCamera->SetActorLocation(Fighter.VisualPosition);
        SetViewTarget(FlightCamera);
    }

    const FVector Forward = ToUE(Fighter.Heading).GetSafeNormal();
    const FVector Up = ToUE(Fighter.Up).GetSafeNormal();
    const FVector Eye = Fighter.VisualPosition - Forward * 130.0 + Up * 32.0;
    const FRotator View = FRotationMatrix::MakeFromXZ(Forward, Up).Rotator();
    FlightCamera->SetActorLocationAndRotation(
        FMath::VInterpTo(FlightCamera->GetActorLocation(), Eye, DeltaTime, 15.0f),
        FMath::RInterpTo(FlightCamera->GetActorRotation(), View, DeltaTime, 15.0f));

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
            const auto* Template = VisualClass ? VisualClass->GetDefaultObject<AShipActor>() : nullptr;
            if (!Template || !Template->HullMesh || !Template->HullMesh->GetStaticMesh()) continue;

            Actor = GetWorld()->SpawnActor<AStaticMeshActor>(ShipState.VisualPosition, Rotation);
            if (!Actor) continue;
            auto* Mesh = Actor->GetStaticMeshComponent();
            Mesh->SetMobility(EComponentMobility::Movable);
            Mesh->SetStaticMesh(Template->HullMesh->GetStaticMesh());
            for (int32 Index = 0; Index < Template->HullMesh->GetNumMaterials(); ++Index)
                Mesh->SetMaterial(Index, Template->HullMesh->GetMaterial(Index));
            Actor->SetActorEnableCollision(false);
        }
        Actor->SetActorLocationAndRotation(ShipState.VisualPosition, Rotation);
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
    if (HasAuthority()) AssignMissionShip(0);
    if (FlightCamera) FlightCamera->Destroy();
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

void ASSWSessionLobbyController::ServerCycleCombatTarget_Implementation()
{
    const auto* State = GetWorld()->GetGameState<ASSWSessionGameState>();
    Ship* Fighter = State && State->bMissionStartReady ? State->FindMissionShip(MissionShipId) : nullptr;
    SimRegion* Region = Fighter ? Fighter->GetRegion() : nullptr;
    if (!Region) return;

    Ship* First = nullptr;
    Ship* Next = nullptr;
    const SimObject* Current = Fighter->GetTarget();
    bool bPassedCurrent = Current == nullptr;
    for (ListIter<Ship> Ships = Region->GetShips(); ++Ships;)
    {
        Ship* Candidate = Ships.value();
        if (Candidate == Current)
        {
            bPassedCurrent = true;
            continue;
        }
        if (!Candidate || Candidate == Fighter || Candidate->IsDead() || Candidate->IsDying() ||
            Candidate->GetIFF() == Fighter->GetIFF() || Candidate->GetIFF() == 0) continue;
        if (!First) First = Candidate;
        if (bPassedCurrent)
        {
            Next = Candidate;
            break;
        }
    }
    if (Next || First || (Current && Current->GetRegion() != Region))
        Fighter->SetTarget(Next ? Next : First);
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
    if (!Fighter) return;

    const FLinearColor TextColor(0.7f, 0.9f, 1.0f);
    const FLinearColor HitColor(1.0f, 0.35f, 0.2f);
    float Row = 24.0f;
    DrawText(FString::Printf(TEXT("%s   HULL %.1f   SHIELDS %d   FUEL %d%%"),
        *Fighter->Name, Fighter->Integrity, Fighter->ShieldStrength, Fighter->FuelPercent), TextColor, 24.0f, Row);
    Row += 20.0f;
    DrawText(FString::Printf(TEXT("SPEED %.1f m/s   THROTTLE %.0f%%   GUN %d   MISSILE %d"),
        Fighter->Velocity.Size(), Fighter->Throttle, Fighter->PrimaryAmmo, Fighter->SecondaryAmmo), TextColor, 24.0f, Row);
    Row += 20.0f;
    DrawText(TEXT("CTRL Fire   SPACE Missile   T Target"), TextColor, 24.0f, Row);
    Row += 24.0f;

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

    for (const auto& System : Fighter->Systems)
    {
        if (System.Availability >= 100.0f && System.bPowered) continue;
        if (Row > Canvas->ClipY * 0.7f) break;
        DrawText(FString::Printf(TEXT("%s   CONDITION %.0f%%   %s"),
            *System.Name, System.Availability, System.bPowered ? TEXT("ON") : TEXT("OFF")), TextColor, 24.0f, Row);
        Row += 18.0f;
    }

    if (Fighter->bDestroyed || Fighter->bRetired) return;
    const float X = Canvas->ClipX * 0.5f;
    const float Y = Canvas->ClipY * 0.5f;
    DrawLine(X - 12.0f, Y, X + 12.0f, Y, TextColor);
    DrawLine(X, Y - 12.0f, X, Y + 12.0f, TextColor);

    for (const auto& Shot : State->MissionProjectiles)
    {
        if (Shot.Region != Fighter->Region) continue;
        FVector2D Start, End;
        if (Controller->ProjectWorldLocationToScreen(Shot.VisualStart, Start) &&
            Controller->ProjectWorldLocationToScreen(Shot.VisualEnd, End))
            DrawLine(Start.X, Start.Y, End.X, End.Y, Shot.bMissile ? HitColor : TextColor, 2.0f);
    }
}

void ASSWSessionLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    if (GetNetMode() != NM_DedicatedServer) return;

    auto* Pilot = Cast<ASSWSessionLobbyController>(NewPlayer);
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
                !Fighter->IsStarship() && !Fighter->IsDropship() &&
                Pilot->AssignMissionShip(Fighter->GetObjID())) return;
        }
    }
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
