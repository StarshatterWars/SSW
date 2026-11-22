#pragma once

#include "GameFramework/GameModeBase.h"
#include "SSWSessionLobbyGameMode.h"
#include "FractalVerseState.h"
#include "FractalVerseSession.h"
#include "FractalVerseGameMode.generated.h"

// First transport slice: status and validated cargo requests over UE owned RPCs.
UCLASS()
class STARSHATTERWARS_API AFractalVerseController : public ASSWSessionLobbyController
{
    GENERATED_BODY()
public:
    AFractalVerseController();
    void BeginPlay() override;
    void Tick(float DeltaSeconds) override;
    void EndPlay(const EEndPlayReason::Type) override;
    UFUNCTION(Server, Reliable) void ServerAuthenticate(const FString& ProductUserId, const FString& IdToken);
    UFUNCTION(Server, Reliable) void ServerLogout();
    UFUNCTION(Server, Unreliable) void ServerPilotInput(const FFractalVerseFlightInput& Input);
    UFUNCTION(Server, Reliable) void ServerShipCommand(FName Action);
    UFUNCTION(Client, Unreliable) void ClientShip(const FFractalVerseShipView& View);

    UFUNCTION(Client, Reliable) void ClientAuthRequired();
    UFUNCTION(Client, Reliable) void ClientPilot(const FFractalVersePilotView& Pilot);
    UFUNCTION(Server, Reliable) void ServerRequestStatus(double SentSeconds);
    UFUNCTION(Client, Reliable) void ClientStatus(const FFractalVerseStatus& Status, double SentSeconds);
    UFUNCTION(Server, Reliable) void ServerTransfer(const FFractalVerseTransfer& Command);
    UFUNCTION(Client, Reliable) void ClientTransferResult(FGuid Request, bool bAccepted, const FString& Reason);
private:
    void RequestStatus();
    bool CanUseNetworkFlight() const override;
    bool ShouldPollFlightInput() const override;
    void TickFlightProbe();
    double ProbeStart=0,ProbeLastInput=0,ProbeLastLog=0;
    int64 ProbeSequence=0;
    bool bProbeMovement=false,bProbeDamage=false,bProbeComplete=false;
    bool CanSendNetworkCommand() const override;
    bool UsesVerifiedPilotIdentity() const override { return true; }
    void ServerFlightInput_Implementation(uint64 PackedControls,uint8 Throttle) override;
    void ServerFireSecondary_Implementation() override;
    void ServerTargetCommand_Implementation(FName Action) override;
    void ServerSensorCommand_Implementation(FName Action) override;
    void ServerSelfDestruct_Implementation() override;
    int64 ReceivedFlightSequence=0;
    double LastShipCommand=-100;
    TSharedPtr<class SWidget> ConnectionReview;
    FFractalVerseSession Session;
    bool bRefreshRequested = false;
    uint32 AuthenticationEpoch = 0;
    bool bProofSent = false;
    bool bVerificationPending = false;
    int32 AuthenticationAttempts = 0;
    double LastAuthentication = -100;
    FDelegateHandle LoginStatusHandle;
    FTimerHandle StatusTimer;
    double LastShipSnapshot = -100;
    FName ControlledPrincipal;
    double LastRequest = -100;
    double LastTransfer = -100;
};

UCLASS()
class STARSHATTERWARS_API AFractalVerseGameMode : public AGameMode
{
    GENERATED_BODY()
public:
    AFractalVerseGameMode();
    bool SetPause(APlayerController*, FCanUnpause = FCanUnpause()) override { return false; }
    void InitGame(const FString& MapName,const FString& Options,FString& ErrorMessage) override;
    void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
};
