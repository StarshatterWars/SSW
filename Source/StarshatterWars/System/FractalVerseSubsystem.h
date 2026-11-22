#pragma once

#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "FractalVerseAuthority.h"
#include "FractalVerseFlightGate.h"
#include "FractalVerseSubsystem.generated.h"

class IFileHandle;

enum class EFractalVerseRole : uint8 { Disabled, Server, Client };

UCLASS()
class STARSHATTERWARS_API UFractalVerseSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
    GENERATED_BODY()
public:
    virtual ~UFractalVerseSubsystem() override;
    static EFractalVerseRole ProcessRole();
    bool ShouldCreateSubsystem(UObject* Outer) const override;
    void Initialize(FSubsystemCollectionBase&) override;
    void Deinitialize() override;
    void Tick(float DeltaSeconds) override;
    bool IsTickable() const override { return !IsTemplate() && Role == EFractalVerseRole::Server && bReady; }
    UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
    TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UFractalVerseSubsystem, STATGROUP_Tickables); }
    FFractalVerseStatus Status() const;
    bool Submit(const FFractalVerseAccess&, const FFractalVerseTransfer&, FString& Reason);
    bool Save();
    // Trusted native provisioning only; no client assignment RPC exists.
    bool BindShip(FName Principal, uint32 ObjectId);
    bool ProvisionShip(FName Principal,FFractalVerseShipView& View);
    bool SubmitFlight(FName Principal,const FFractalVerseFlightInput& Input);
    bool ShipCommand(FName Principal,FName Action);
    bool ReadShip(FName Principal,FFractalVerseShipView& View) const;
    void ReleaseShip(FName Principal);
    void ForgetShipObject(uint32 ObjectId);

    bool BindVerifiedPilot(FName Principal,const FString& VerifiedLegacySlot,FFractalVersePilotView& View);
    bool ReadPilot(FName Principal, FFractalVersePilotView& View) const;
    void ReceivePilot(const FFractalVersePilotView& View) { if (Role==EFractalVerseRole::Client) {RemotePilot=View; if(!View.bAuthenticated) RemoteShip=FFractalVerseShipView();} }
    const FFractalVersePilotView& Pilot() const { return RemotePilot; }
    void ReceiveShip(const FFractalVerseShipView& View) { if(Role==EFractalVerseRole::Client) RemoteShip=View; }
    const FFractalVerseShipView& ShipView() const { return RemoteShip; }
    // Receive-only projection. Clients never obtain a mutable server-world reference.
    void BeginClientConnection();
    void ReceiveStatus(const FFractalVerseStatus&);
    bool IsConnected() const;
    double PingMilliseconds() const { return LastPingMs; }
    void RecordPing(double SentSeconds);
private:
    EFractalVerseRole Role = EFractalVerseRole::Disabled;
    FFractalVerseAuthority Authority;
    TSharedPtr<class FFractalVerseShipObserver> ShipObserver;
    FFractalVerseFlightGate FlightGate;
    TSet<FName> ManualActive;
    TMap<FName,uint32> ControlledShips;
    TMap<FName,uint32> ResidentShips;
    double FlightElapsed=0;
    double LastFlightProbeLog=0;
    void AdvanceShipControls();
    bool EnsureFlightWorld(FName System,FName Region);
    void CapturePlayerShips();
    void PlayerShipRetired(const class Ship& Vessel,const class SimRegion& Region);
    TSharedPtr<class Mission> FlightWorld;
    TMap<FName,FString> ProvisionErrors;

    FFractalVerseStatus Remote;
    FFractalVersePilotView RemotePilot;
    FFractalVerseShipView RemoteShip;
    FString Directory;
    bool bReady = false;
    bool bPersistenceReady = false;
    TUniquePtr<IFileHandle> WriterLock;
    double TestExitDeadline = 0;
    double Accumulator = 0;
    double SaveElapsed = 0;
    double StartedAt = 0;
    double LastReceived = 0;
    bool bAcceptRecoveredSnapshot = true;
    double LastPingMs = -1;
    double DiscoveryElapsed = 0;
    FString InstanceId;
    int64 LastCheckpointUtc = 0;
    FDelegateHandle PreExitHandle;
    void SaveBeforeEngineExit();
    void PublishDiscovery();
};
