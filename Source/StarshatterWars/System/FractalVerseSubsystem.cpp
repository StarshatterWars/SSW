#include "FractalVerseSubsystem.h"
#include "Sim.h"
#include "Ship.h"
#include "SimShot.h"
#include "SSWNetworkShipControls.h"
#include "FractalVerseShipProvisioning.h"
#include "SSWRuntimeSubsystem.h"
#include "SSWSessionLobbyGameMode.h"
#include "StarshatterEnvironmentSubsystem.h"
#include "ShipDesignRegistry.h"
#include "ShipDesign.h"
#include "OwnedHullCondition.h"
#include "Mission.h"
#include "StarSystem.h"
#include "JsonObjectConverter.h"
#include "Math/RotationMatrix.h"
#include "FractalVersePilotMigration.h"
#include "FractalVersePilotPromotion.h"
#include "GalaxyTrafficSubsystem.h"
#include "CargoCatalogSubsystem.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformProcess.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/CommandLine.h"
#include "Misc/CoreDelegates.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

namespace
{
SimRegion* FindFractalRegion(Sim* Simulation,FName Name)
{
    if(!Simulation) return nullptr;
    for(ListIter<SimRegion> Regions=Simulation->GetRegions();++Regions;)
        if(auto* Region=Regions.value();Region && FString(UTF8_TO_TCHAR(Region->GetName())).Equals(Name.ToString(),ESearchCase::IgnoreCase)) return Region;
    return nullptr;
}
}

class FFractalVerseShipObserver final : public SimObserver
{
public:
    explicit FFractalVerseShipObserver(UFractalVerseSubsystem* InOwner):Owner(InOwner) {}
    bool Update(SimObject* Object) override
    {
        if(Object && Owner.IsValid()) Owner->ForgetShipObject(Object->GetObjID());
        return SimObserver::Update(Object);
    }
private:
    TWeakObjectPtr<UFractalVerseSubsystem> Owner;
};

UFractalVerseSubsystem::~UFractalVerseSubsystem() = default;

EFractalVerseRole UFractalVerseSubsystem::ProcessRole()
{
#if defined(SSW_FRACTALVERSE) && SSW_FRACTALVERSE
    return EFractalVerseRole::Server;
#else
    if (FParse::Param(FCommandLine::Get(), TEXT("SSWFractalVerseServer"))) return EFractalVerseRole::Server;
    if (FParse::Param(FCommandLine::Get(), TEXT("SSWFractalVerseClient"))) return EFractalVerseRole::Client;
    return EFractalVerseRole::Disabled;
#endif
}

bool UFractalVerseSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{ return Super::ShouldCreateSubsystem(Outer) && ProcessRole() != EFractalVerseRole::Disabled; }

void UFractalVerseSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Role = ProcessRole();
    StartedAt=FPlatformTime::Seconds();
    InstanceId=FGuid::NewGuid().ToString(EGuidFormats::Digits);
    FParse::Value(FCommandLine::Get(),TEXT("FractalVerseInstance="),InstanceId);
    if (Role != EFractalVerseRole::Server) return;
    Collection.InitializeDependency<UCargoCatalogSubsystem>();
    GetGameInstance()->GetSubsystem<UCargoCatalogSubsystem>()->LoadCatalog();
    Collection.InitializeDependency<UGalaxyTrafficSubsystem>();
    Directory = FPaths::ProjectSavedDir() / TEXT("FractalVerse/LocalUniverse");
    FString Override;
    if (FParse::Value(FCommandLine::Get(), TEXT("FractalVerseSaveDir="), Override)) Directory = FPaths::ConvertRelativePathToFull(Override);
    auto& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    PlatformFile.CreateDirectoryTree(*Directory);
    WriterLock.Reset(PlatformFile.OpenWrite(*(Directory / TEXT("writer.lock")), false, false));
    if (!WriterLock) { UE_LOG(LogTemp, Error, TEXT("[FractalVerse] Universe directory already in use or unwritable.")); return; }
    double TestSeconds=0;
    if (FParse::Value(FCommandLine::Get(), TEXT("FractalVerseTestSeconds="), TestSeconds) && TestSeconds>0)
        TestExitDeadline=FPlatformTime::Seconds()+FMath::Clamp(TestSeconds,10.0,3600.0);
    FString Reason;
    FFractalVerseWorld Restored;
    if (FFractalVerseCheckpointStore::Load(Directory, Restored, Reason))
        bReady = Authority.Restore(Restored, Reason);
    else if (!IFileManager::Get().FileExists(*(Directory / TEXT("head"))) && !IFileManager::Get().FileExists(*(Directory / TEXT("previous"))))
        bReady = true;
    if (!bReady)
    { UE_LOG(LogTemp, Error, TEXT("[FractalVerse] Refusing to replace unreadable universe: %s"), *Reason); return; }
    Authority.SynchronizeClock(FDateTime::UtcNow().ToUnixTimestamp());
    bPersistenceReady = Save();
    PreExitHandle = FCoreDelegates::OnEnginePreExit.AddUObject(this, &UFractalVerseSubsystem::SaveBeforeEngineExit);
    UE_LOG(LogTemp, Display, TEXT("[FractalVerse] Server world ready: %s revision %lld. Independent of player saves and loaded missions."),
        *Authority.Read().Universe.ToString(), Authority.Read().Revision);
}

FFractalVerseStatus UFractalVerseSubsystem::Status() const
{
    if (Role!=EFractalVerseRole::Server) return Remote;
    auto State=Authority.Status(bPersistenceReady);
    State.UptimeSeconds=FMath::Max(0.0,FPlatformTime::Seconds()-StartedAt);
    if (auto* World=GetWorld()) for(auto Iter=World->GetPlayerControllerIterator();Iter;++Iter)
        if(auto* Controller=Iter->Get();Controller && State.Players.Num()<State.MaximumClients)
        {
            FFractalVersePeer Peer;
            const auto* Player=Controller->PlayerState.Get();
            Peer.Name=Player?Player->GetPlayerName().Left(64):TEXT("Connecting pilot");
            Peer.PingMs=Player?Player->GetPingInMilliseconds():0;
            State.Players.Add(MoveTemp(Peer));
        }
    State.ConnectedClients=State.Players.Num();
    return State;
}

bool UFractalVerseSubsystem::IsConnected() const
{
    return Role==EFractalVerseRole::Server ? bReady :
        Remote.Universe.IsValid() && LastReceived>0 && FPlatformTime::Seconds()-LastReceived<8 &&
        GetWorld() && GetWorld()->GetNetMode()==NM_Client;
}
void UFractalVerseSubsystem::RecordPing(double Sent)
{
    const double Elapsed=FPlatformTime::Seconds()-Sent;
    if (FMath::IsFinite(Elapsed) && Elapsed>=0 && Elapsed<30) LastPingMs=Elapsed*1000;
}
void UFractalVerseSubsystem::PublishDiscovery()
{
    const auto State=Status();
    auto Json=MakeShared<FJsonObject>();
    Json->SetNumberField(TEXT("protocol"),State.Protocol); Json->SetBoolField(TEXT("online"),bReady);
    Json->SetBoolField(TEXT("checkpointReady"),bPersistenceReady); Json->SetNumberField(TEXT("lastCheckpointUtc"),LastCheckpointUtc);
    Json->SetNumberField(TEXT("inventories"),State.Inventories); Json->SetNumberField(TEXT("journeys"),State.Journeys); Json->SetNumberField(TEXT("jobs"),State.Jobs);
    Json->SetStringField(TEXT("server"),TEXT("FractalVerse (local)")); Json->SetStringField(TEXT("instance"),InstanceId);
    Json->SetNumberField(TEXT("processId"),FPlatformProcess::GetCurrentProcessId());
    Json->SetNumberField(TEXT("revision"),State.Revision); Json->SetNumberField(TEXT("serverUtc"),FDateTime::UtcNow().ToUnixTimestamp());
    Json->SetNumberField(TEXT("uptimeSeconds"),State.UptimeSeconds); Json->SetNumberField(TEXT("maximumClients"),State.MaximumClients);
    Json->SetNumberField(TEXT("connectedClients"),State.ConnectedClients);
    Json->SetStringField(TEXT("universeUtc"),FDateTime::FromUnixTimestamp(State.UniverseBaseUtc+State.UniverseSeconds).ToIso8601());
    TArray<TSharedPtr<FJsonValue>> Players;
    for(const auto& Peer:State.Players)
    {auto Item=MakeShared<FJsonObject>();Item->SetStringField(TEXT("name"),Peer.Name);Item->SetNumberField(TEXT("pingMs"),Peer.PingMs);Players.Add(MakeShared<FJsonValueObject>(Item));}
    Json->SetArrayField(TEXT("players"),Players);
    FString Text;
    if(FJsonSerializer::Serialize(Json,TJsonWriterFactory<>::Create(&Text)) &&
        FFileHelper::SaveStringToFile(Text,*(Directory/TEXT("status.pending")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
        IFileManager::Get().Move(*(Directory/TEXT("status.json")),*(Directory/TEXT("status.pending")),true,true);
}

bool UFractalVerseSubsystem::Save()
{
    if(Role==EFractalVerseRole::Server) CapturePlayerShips();
    if (Role != EFractalVerseRole::Server || !bReady) return false;
    FString Reason;
    bPersistenceReady = FFractalVerseCheckpointStore::Save(Directory, Authority.Read(), Reason);
    if (bPersistenceReady) LastCheckpointUtc=FDateTime::UtcNow().ToUnixTimestamp();
    if (!bPersistenceReady) UE_LOG(LogTemp, Error, TEXT("[FractalVerse] Simulation suspended: %s"), *Reason);
    return bPersistenceReady;
}

bool UFractalVerseSubsystem::ReadPilot(FName Principal, FFractalVersePilotView& View) const
{
    return Role==EFractalVerseRole::Server && bReady && bPersistenceReady && Authority.PilotView(Principal,View);
}

bool UFractalVerseSubsystem::BindVerifiedPilot(FName Principal,const FString& VerifiedLegacySlot,FFractalVersePilotView& View)
{
    View=FFractalVersePilotView();
    if (Role!=EFractalVerseRole::Server || !bReady || !bPersistenceReady || Principal.IsNone()) return false;
    if (Authority.PilotView(Principal,View)) return true;
    // The source slot comes from verified EOS metadata, never an RPC path/account field.
    if (!VerifiedLegacySlot.IsEmpty() && FParse::Param(FCommandLine::Get(),TEXT("FractalVerseImportLegacyPilots")))
    {
        FString SourceDirectory=Directory/TEXT("LegacyImports");
        FParse::Value(FCommandLine::Get(),TEXT("FractalVerseLegacySaveDir="),SourceDirectory);
        FFractalVersePilotCandidate Prepared; FString Reason,Backup;
        if (!FFractalVersePilotMigration::ReadDirectory(SourceDirectory,VerifiedLegacySlot,Prepared,Reason) ||
            !FFractalVersePilotMigration::Stage(Directory/TEXT("PilotImports"),Principal,Prepared,Backup,Reason))
        { UE_LOG(LogTemp,Warning,TEXT("[FractalVerse] Pilot import blocked: %s"),*Reason); return false; }
        if (!FFractalVersePilotCommit::Commit(Authority,Principal,Prepared,[this](){return Save();},Reason))
        { UE_LOG(LogTemp,Warning,TEXT("[FractalVerse] Pilot import blocked: %s"),*Reason); return false; }
        return Authority.PilotView(Principal,View);
    }
    // Existing careers must be migrated before binding, never replaced by a starter wallet.
    // Explicit new-account creation is a separate, opt-in test path for now.
    if (!FParse::Param(FCommandLine::Get(),TEXT("FractalVerseAllowNewPilot")))
    {
        UE_LOG(LogTemp,Display,TEXT("[FractalVerse] Pilot binding awaits migration or explicit new-career authorization."));
        return false;
    }
    const FFractalVerseWorld Before=Authority.Read(); FString Reason;
    if (!Authority.EnsurePilot(Principal,Reason)) return false;
    if (Save()) return Authority.PilotView(Principal,View);
    Authority.Restore(Before,Reason);
    return false;
}

bool UFractalVerseSubsystem::Submit(const FFractalVerseAccess& Access, const FFractalVerseTransfer& Command, FString& Reason)
{
    if (Role != EFractalVerseRole::Server || !bReady || !bPersistenceReady)
    { Reason = TEXT("Universe server is unavailable."); return false; }
    // Reject unauthenticated requests before copying the world.
    if (Access.Principal.IsNone()) { Reason=TEXT("Verified account required."); return false; }
    // Commit receipt and inventory together before acknowledging an operation.
    const FFractalVerseWorld Before = Authority.Read();
    if (!Authority.Transfer(Access, Command, Reason)) return false;
    if (Authority.Read().Revision == Before.Revision) return true;
    if (Save()) return true;
    FString RestoreReason;
    Authority.Restore(Before, RestoreReason);
    Reason = TEXT("Cargo change could not be saved; operation was not acknowledged.");
    return false;
}

void UFractalVerseSubsystem::Tick(float DeltaSeconds)
{
    if (!FMath::IsFinite(DeltaSeconds) || DeltaSeconds<=0) return;
    FlightElapsed+=DeltaSeconds;
    if (FlightElapsed>=.05) { FlightElapsed=0; AdvanceShipControls(); }
    DiscoveryElapsed+=DeltaSeconds;
    if (DiscoveryElapsed>=2)
    {
        DiscoveryElapsed=0;
        FString StopInstance;
        if (FFileHelper::LoadFileToString(StopInstance,*(Directory/TEXT("shutdown.request"))) && StopInstance.TrimStartAndEnd()==InstanceId)
        {
            IFileManager::Get().Delete(*(Directory/TEXT("shutdown.request")));
            if (Save()) FPlatformMisc::RequestExit(false);
            return;
        }
        PublishDiscovery();
    }
    if (TestExitDeadline>0 && FPlatformTime::Seconds()>=TestExitDeadline)
    { TestExitDeadline=0; Save(); FPlatformMisc::RequestExit(false); return; }
    if (!FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0) return;
    if (!bPersistenceReady)
    { SaveElapsed += DeltaSeconds; if (SaveElapsed >= 5) { SaveElapsed = 0; Save(); } return; }
    Accumulator += DeltaSeconds;
    if (Accumulator < 1) return;
    const double Step = FMath::Min(Accumulator, 5.0);
    Accumulator -= Step;
    Authority.SynchronizeClock(FDateTime::UtcNow().ToUnixTimestamp());
    if (auto* Traffic = GetGameInstance()->GetSubsystem<UGalaxyTrafficSubsystem>())
        Authority.Advance(Step, [Traffic](FFractalVerseWorld& State, double Seconds) { Traffic->AdvanceServerWorld(State, Seconds); });
    SaveElapsed += Step;
    if (SaveElapsed >= 300) { SaveElapsed = 0; Save(); }
}

void UFractalVerseSubsystem::BeginClientConnection()
{
    if (Role!=EFractalVerseRole::Client) return;
    LastReceived=0; LastPingMs=-1; RemotePilot=FFractalVersePilotView(); RemoteShip=FFractalVerseShipView(); bAcceptRecoveredSnapshot=true;
}

void UFractalVerseSubsystem::ReceiveStatus(const FFractalVerseStatus& State)
{
    if (Role != EFractalVerseRole::Client || State.Protocol != 1 || !State.Universe.IsValid() ||
        !FMath::IsFinite(State.EconomySeconds) || State.EconomySeconds < 0 || State.Revision < 0) return;
    if (State.UniverseBaseUtc<=0 || State.UniverseSeconds<0 || State.ServerUtc<=0) return;
    if (!bAcceptRecoveredSnapshot && Remote.Universe == State.Universe && (State.Revision < Remote.Revision ||
        State.UniverseSeconds < Remote.UniverseSeconds || State.UniverseBaseUtc!=Remote.UniverseBaseUtc)) return;
    Remote = State; bAcceptRecoveredSnapshot=false;
    LastReceived=FPlatformTime::Seconds();
    UE_LOG(LogTemp, Display, TEXT("[FractalVerseClient] Confirmed revision %lld clock %.1f inventories %d journeys %d jobs %d saved=%d universe=%lld/%lld"),
        State.Revision, State.EconomySeconds, State.Inventories, State.Journeys, State.Jobs, State.bPersistenceReady, State.UniverseBaseUtc, State.UniverseSeconds);
}

void UFractalVerseSubsystem::SaveBeforeEngineExit()
{
    if (Role == EFractalVerseRole::Server && bReady) Save();
}

void UFractalVerseSubsystem::Deinitialize()
{
    CapturePlayerShips();
    if(auto* Simulation=Sim::GetSim()) Simulation->OnShipRetired.RemoveAll(this);
    ShipObserver.Reset(); FlightGate.Reset(); ControlledShips.Empty(); ResidentShips.Empty(); ManualActive.Empty();
    FCoreDelegates::OnEnginePreExit.Remove(PreExitHandle);
    PreExitHandle.Reset();
    if (Role == EFractalVerseRole::Server && bReady) Save();
    if (Role==EFractalVerseRole::Server && WriterLock) IFileManager::Get().Delete(*(Directory/TEXT("status.json")));
    WriterLock.Reset();
    Super::Deinitialize();
}

// Resolve IDs on every use; a removed ship cannot leave a dangling pointer.
bool UFractalVerseSubsystem::BindShip(FName Principal,uint32 ObjectId)
{
    if (Role!=EFractalVerseRole::Server || !bReady || !bPersistenceReady || !Authority.Read().Pilots.Contains(Principal)) return false;
    auto* Simulation=Sim::GetSim();
    auto* Vessel=Simulation?Simulation->FindShipByObjID(ObjectId):nullptr;
    if (!Simulation || !Simulation->HasSimulationAuthority() || !Vessel || Vessel->IsDead() || Vessel->IsDying() || !FlightGate.Bind(Principal,ObjectId)) return false;
    if (!ShipObserver) ShipObserver=MakeShared<FFractalVerseShipObserver>(this);
    ShipObserver->Observe(Vessel);
    ResidentShips.Add(Principal,ObjectId);
    ControlledShips.Add(Principal,ObjectId); Vessel->SetNetworkPilot(true); return true;
}
bool UFractalVerseSubsystem::SubmitFlight(FName Principal,const FFractalVerseFlightInput& Input)
{
    if (Role!=EFractalVerseRole::Server || !bReady || !bPersistenceReady || Principal.IsNone()) return false;
    FFractalVerseShipView View;
    return ReadShip(Principal,View) && FlightGate.Submit(Principal,Input,FPlatformTime::Seconds());
}
bool UFractalVerseSubsystem::ReadShip(FName Principal,FFractalVerseShipView& View) const
{
    View=FFractalVerseShipView();
    if (Role!=EFractalVerseRole::Server || Principal.IsNone()) return false;
    const uint32 Id=FlightGate.Object(Principal);
    auto* Simulation=Sim::GetSim();
    auto* Vessel=Simulation && Id?Simulation->FindShipByObjID(Id):nullptr;
    if (!Vessel || Vessel->IsDead() || Vessel->IsDying()) return false;
    View.ObjectId=Id; View.Position=Vessel->GetLocation(); View.Velocity=Vessel->GetVelocity();
    View.Integrity=Vessel->GetIntegrity(); View.Throttle=Vessel->GetThrottle(); return true;
}
void UFractalVerseSubsystem::ReleaseShip(FName Principal)
{
    CapturePlayerShips();
    if (Role!=EFractalVerseRole::Server) return;
    auto* Simulation=Sim::GetSim(); const uint32 Id=FlightGate.Object(Principal);
    if (auto* Vessel=Simulation && Id?Simulation->FindShipByObjID(Id):nullptr)
    { SSWNetworkShipControls::Clear(*Vessel); Vessel->SetNetworkPilot(false);  }
    FlightGate.Revoke(Principal); ControlledShips.Remove(Principal); ManualActive.Remove(Principal);
}
void UFractalVerseSubsystem::AdvanceShipControls()
{
    auto* Simulation=Sim::GetSim();
    if (!Simulation || !Simulation->HasSimulationAuthority()) { FlightGate.Reset(); ControlledShips.Empty(); return; }
    TArray<FName> Lost;
    for (const auto& Pair:ControlledShips)
    {
        auto* Vessel=Simulation->FindShipByObjID(Pair.Value);
        if (!Vessel || Vessel->IsDead() || Vessel->IsDying()) {Lost.Add(Pair.Key);continue;}
#if !UE_BUILD_SHIPPING
        if(FParse::Param(FCommandLine::Get(),TEXT("FractalVerseFlightProbe")) && FPlatformTime::Seconds()-LastFlightProbeLog>1)
        {
            LastFlightProbeLog=FPlatformTime::Seconds(); const auto* Target=Vessel->GetTarget();
            UE_LOG(LogTemp,Display,TEXT("[FractalVerseFlightProbe] server object=%u speed=%.3f targetIntegrity=%.3f"),
                Vessel->GetObjID(),Vessel->GetVelocity().Size(),Target?Target->GetIntegrity():-1.);
            if(auto* Region=Vessel->GetRegion())
            {
                for(ListIter<Ship> Ships=Region->GetShips();++Ships;)
                    if(auto* Candidate=Ships.value();Candidate && Candidate!=Vessel)
                        UE_LOG(LogTemp,Display,TEXT("[FractalVerseFlightProbe] geometry own=%s heading=%s other=%s radius=%.3f ai=%d contacts=%d"),
                            *Vessel->GetLocation().ToString(),*Vessel->GetHeading().ToString(),*Candidate->GetLocation().ToString(),Candidate->GetRadius(),Vessel->GetAIMode(),Vessel->GetContactList().size());
                const auto Projectiles=Region->GetProjectiles();
                if(!Projectiles.IsEmpty() && Projectiles[0])
                    UE_LOG(LogTemp,Display,TEXT("[FractalVerseFlightProbe] shot origin=%s location=%s velocity=%s"),
                        *Projectiles[0]->GetOrigin().ToString(),*Projectiles[0]->GetLocation().ToString(),*Projectiles[0]->GetVelocity().ToString());
            }
        }
#endif
        const auto Input=bPersistenceReady?FlightGate.Read(Pair.Key,FPlatformTime::Seconds()):FFractalVerseFlightInput();
        if(Input.Sequence==0)
        {
            if(ManualActive.Remove(Pair.Key)) SSWNetworkShipControls::Clear(*Vessel);
            continue;
        }
        ManualActive.Add(Pair.Key);
        SSWNetworkShipControls::Apply(*Vessel,Input.Rotation,Input.Translation,Input.Throttle,Input.bAugmenter,Input.bPrimary);
    }
    for (FName Principal:Lost) ReleaseShip(Principal);
}

void UFractalVerseSubsystem::ForgetShipObject(uint32 ObjectId)
{
    TArray<FName> Removed;
    for(const auto& Pair:ResidentShips) if(Pair.Value==ObjectId) Removed.Add(Pair.Key);
    for(FName Principal:Removed) {FlightGate.Revoke(Principal); ControlledShips.Remove(Principal); ResidentShips.Remove(Principal); ManualActive.Remove(Principal);}
}

bool UFractalVerseSubsystem::ShipCommand(FName Principal,FName Action)
{
    if(Role!=EFractalVerseRole::Server || !bReady || !bPersistenceReady) return false;
    FFractalVerseShipView View; if(!ReadShip(Principal,View)) return false;
    auto* Simulation=Sim::GetSim(); auto* Vessel=Simulation?Simulation->FindShipByObjID(View.ObjectId):nullptr;
    if(!Vessel || !Simulation->HasSimulationAuthority()) return false;
    if(Action==TEXT("IA_FireSecondary")) {Vessel->FireSecondary();return true;}
    static const TSet<FName> Targets={TEXT("SSW_TargetTracking"),TEXT("IA_DropWeaponTarget"),TEXT("IA_TargetNearest"),
        TEXT("IA_TargetHostile"),TEXT("IA_PreviousTarget"),TEXT("IA_NextTarget")};
    static const TSet<FName> Sensors={TEXT("IA_EMCONUp"),TEXT("IA_EMCONDown"),TEXT("IA_SensorRangeUp"),
        TEXT("IA_SensorRangeDown"),TEXT("IA_SensorGroundMode"),TEXT("IA_SensorMode")};
    if(Targets.Contains(Action)) {SSWNetworkShipControls::Target(*Vessel,Action);return true;}
    if(Sensors.Contains(Action)) {SSWNetworkShipControls::SensorCommand(*Vessel,Action);return true;}
    return false;
}

bool UFractalVerseSubsystem::EnsureFlightWorld(FName System,FName RegionName)
{
    auto* Instance=GetGameInstance(); auto* World=GetWorld();
    if(Role!=EFractalVerseRole::Server || !World || !Instance) return false;
    auto* State=World->GetGameState<ASSWSessionGameState>();
    auto* Runtime=Instance->GetSubsystem<USSWRuntimeSubsystem>();
    auto* Environment=Instance->GetSubsystem<UStarshatterEnvironmentSubsystem>();
    if(!State || !Runtime || !Environment) return false;
    if(FlightWorld) return FString(FlightWorld->GetSystem()).Equals(System.ToString(),ESearchCase::IgnoreCase);
    Runtime->StartRuntime();
    auto* Simulation=Sim::GetSim(); if(!Simulation || Simulation->GetMission()) return false;
    Environment->BuildSimRegionsForSim(Simulation);
    auto* Region=FindFractalRegion(Simulation,RegionName); if(!Region || !Region->GetSystem() ||
        !FString(UTF8_TO_TCHAR(Region->GetSystem()->GetName())).Equals(System.ToString(),ESearchCase::IgnoreCase)) return false;
    FlightWorld=MakeShared<Mission>(); FlightWorld->SetName("FractalVerse Live Universe");
    FlightWorld->SetSystem(TCHAR_TO_UTF8(*System.ToString())); FlightWorld->SetRegion(Region->GetName());
    FlightWorld->SetOK(true); Simulation->LoadMission(FlightWorld.Get(),false); Simulation->ActivateRegion(Region);
    Simulation->SetNetworkWorld(World); State->BindSimulation(*Simulation);
    State->MissionMap=TEXT("/Game/Maps/")+System.ToString(); State->ArenaSector=UTF8_TO_TCHAR(Region->GetName());
    State->ArenaOrbitalOrigin=Region->GetLocation(); State->ArenaRadius=0;
    Simulation->OnShipRetired.AddUObject(this,&UFractalVerseSubsystem::PlayerShipRetired);
    State->bMissionStartReady=true; State->ForceNetUpdate();
    UE_LOG(LogTemp,Display,TEXT("[FractalVerseShips] Native flight world ready: %s / %s"),*System.ToString(),*RegionName.ToString());
    return true;
}
bool UFractalVerseSubsystem::ProvisionShip(FName Principal,FFractalVerseShipView& View)
{
    View=FFractalVerseShipView();
    if(Role!=EFractalVerseRole::Server || !bReady || !bPersistenceReady || Principal.IsNone()) return false;
    if(ReadShip(Principal,View)) return true;
    if(const auto* Id=ResidentShips.Find(Principal))
        if(BindShip(Principal,*Id)) return ReadShip(Principal,View);
    const auto* Pilot=Authority.Read().Pilots.Find(Principal); if(!Pilot) return false;
    FFractalVersePlayerShip Plan; FString Reason;
    if(const auto* Saved=Authority.Read().PlayerShips.Find(Principal)) Plan=*Saved;
    else if(!FFractalVerseShipProvisioning::Plan(*Pilot,ControlledShips.Num(),Plan,Reason))
    {
        if(ProvisionErrors.FindRef(Principal)!=Reason) {ProvisionErrors.Add(Principal,Reason);UE_LOG(LogTemp,Display,TEXT("[FractalVerseShips] Provisioning deferred: %s"),*Reason);}
        return false;
    }
    if(!FFractalVerseShipProvisioning::Validate(Plan) || Plan.bDestroyed || Plan.Hull.bLost ||
        !EnsureFlightWorld(Plan.System,Plan.Region)) return false;
    auto* Design=ShipDesignRegistry::FindLegacy(Plan.Hull.Design); if(!Design) return false;
    if(!Plan.Hull.LoadoutStations.IsEmpty() && Plan.Hull.LoadoutStations.Num()!=Design->hardpoints.size()) return false;
    auto* Simulation=Sim::GetSim();
    const FFractalVerseWorld Before=Authority.Read();
    auto* NativeRegion=FindFractalRegion(Simulation,Plan.Region); if(!NativeRegion) return false;
    Ship* Vessel=Simulation->CreateShip(TCHAR_TO_UTF8(*Plan.Hull.Name),"",Design,NativeRegion->GetName(),
        Plan.Position,Plan.IFF,0,Plan.Hull.LoadoutStations.IsEmpty()?nullptr:Plan.Hull.LoadoutStations.GetData());
    if(!Vessel) return false;
    Vessel->SetFlightPhase(EOPSMode::ACTIVE); Vessel->SetNetworkPilot(true); Vessel->SetVelocity(Plan.Velocity);
    Camera Pose; Pose.MoveTo(Plan.Position); Pose.SetOrientation(FVector::CrossProduct(Plan.Up,Plan.Heading),Plan.Up,Plan.Heading); Vessel->CloneCam(Pose);
    if(Plan.Hull.bHasCondition) FOwnedHullCondition::Apply(*Vessel,Plan.Hull);
    else FOwnedHullCondition::ApplyPristine(*Vessel);
    if(!BindShip(Principal,Vessel->GetObjID())) {Simulation->DestroyShip(Vessel);return false;}
    Authority.World.PlayerShips.Add(Principal,Plan); ++Authority.World.Revision;
    if(!Save()) {ReleaseShip(Principal);Simulation->DestroyShip(Vessel);FString RestoreReason;Authority.Restore(Before,RestoreReason);return false;}
    ProvisionErrors.Remove(Principal);
    UE_LOG(LogTemp,Display,TEXT("[FractalVerseShips] Provisioned server hull %s object=%u design=%s"),
        *Plan.Hull.HullId.ToString(),Vessel->GetObjID(),*Plan.Hull.Design.ToString());
#if !UE_BUILD_SHIPPING
    if(FParse::Param(FCommandLine::Get(),TEXT("FractalVerseFlightProbe")))
    {
        if(auto* TargetDesign=ShipDesignRegistry::FindLegacy(FName(TEXT("TargetDrone"))))
            if(auto* Target=Simulation->CreateShip("FractalVerse probe target","",TargetDesign,NativeRegion->GetName(),
                Plan.Position+Plan.Heading*1500.,Plan.IFF==2?1:2,0,nullptr))
            { Target->SetFlightPhase(EOPSMode::ACTIVE); Target->SetNetworkPilot(true); Target->SetManualThrottle(0); Target->MoveTo(Plan.Position+Plan.Heading*1500.); Target->SetVelocity(FVector::ZeroVector); Vessel->LockTarget(Target); }
    }
#endif
    return ReadShip(Principal,View);
}
void UFractalVerseSubsystem::CapturePlayerShips()
{
    auto* Simulation=Sim::GetSim(); if(!Simulation || !Simulation->HasSimulationAuthority()) return;
    for(const auto& Pair:ResidentShips)
    {
        auto* Record=Authority.World.PlayerShips.Find(Pair.Key); auto* Vessel=Simulation->FindShipByObjID(Pair.Value);
        if(!Record || !Vessel) continue;
        Record->Position=Vessel->GetLocation(); Record->Velocity=Vessel->GetVelocity();
        Record->Heading=Vessel->GetHeading(); Record->Up=Vessel->GetCam().vup();
        if(auto* Region=Vessel->GetRegion()) Record->Region=FName(UTF8_TO_TCHAR(Region->GetName()));
        FOwnedHullCondition::Capture(*Vessel,Record->Hull); Record->bDestroyed=Vessel->IsDead() || Vessel->IsDying();
        if(Record->bDestroyed) Record->Hull.bLost=true;
    }
}

void UFractalVerseSubsystem::PlayerShipRetired(const Ship& Vessel,const SimRegion&)
{
    CapturePlayerShips();
    // Retirement is not always destruction (unload/transfer). Do not grant replacements.
    for(const auto& Pair:ResidentShips) if(Pair.Value==Vessel.GetObjID())
        if(auto* Record=Authority.World.PlayerShips.Find(Pair.Key))
        {Record->bDestroyed=Vessel.IsDead() || Vessel.IsDying(); if(Record->bDestroyed) Record->Hull.bLost=true;}
    ForgetShipObject(Vessel.GetObjID());
}
