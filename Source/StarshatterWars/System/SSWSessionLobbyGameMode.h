#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "GameFramework/HUD.h"
#include "GameFramework/GameState.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "SSWSessionLobbyGameMode.generated.h"
class Sim;
class Ship;
class ShipDesign;
class Mission;
class ULevelStreamingDynamic;
class ACameraActor;
class AShipActor;
class ASystemSceneBuilder;
class UEnhancedInputComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;
class SFighterHUDDetails;
class SEditableTextBox;
class SScrollBox;
class SWidget;
class UMultiplayerDlg;
class Orbital;
class SimShot;
class SimRegion;

USTRUCT(BlueprintType)
struct FSSWReplicatedSystemState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FString Name;
    UPROPERTY(BlueprintReadOnly) int32 Status = 0;
    UPROPERTY(BlueprintReadOnly) float Availability = 0;
    UPROPERTY(BlueprintReadOnly) bool bPowered = false;
};

USTRUCT(BlueprintType)
struct FSSWReplicatedProjectileState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) int64 ObjectId = 0;
    UPROPERTY(BlueprintReadOnly) int64 OwnerId = 0;
    UPROPERTY(BlueprintReadOnly) FVector VisualStart = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) FVector VisualEnd = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) bool bMissile = false;
    UPROPERTY(BlueprintReadOnly) FString Region;
    UPROPERTY(BlueprintReadOnly) FVector Position = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) FVector Velocity = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct FSSWReplicatedNavPoint
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FVector Position = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) FString Region;
    UPROPERTY(BlueprintReadOnly) int32 Action = 0;
    UPROPERTY(BlueprintReadOnly) int32 Status = 0;
    UPROPERTY(BlueprintReadOnly) float Speed = 0;
};

USTRUCT(BlueprintType)
struct FSSWReplicatedShipState
{
    GENERATED_BODY()

    FString GetPilotLabel() const
    {
        return PilotName.IsEmpty() ? Name : FString::Printf(TEXT("%s (%s)"), *Name, *PilotName);
    }

    UPROPERTY(BlueprintReadOnly) int64 ObjectId = 0;
    UPROPERTY(BlueprintReadOnly) FString Name;
    UPROPERTY(BlueprintReadOnly) FString PilotName;
    UPROPERTY(BlueprintReadOnly) FString Region;
    UPROPERTY(BlueprintReadOnly) FString Element;
    UPROPERTY(BlueprintReadOnly) int32 IFF = 0;
    UPROPERTY(BlueprintReadOnly) FVector Position = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) FVector Velocity = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) FVector VisualPosition = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) FString Design;
    UPROPERTY(BlueprintReadOnly) FVector Heading = FVector::ForwardVector;
    UPROPERTY(BlueprintReadOnly) FVector Up = FVector::UpVector;
    UPROPERTY(BlueprintReadOnly) float Integrity = 0;
    UPROPERTY(BlueprintReadOnly) float HullPercent = 0;
    UPROPERTY(BlueprintReadOnly) FString PrimaryReadout;
    UPROPERTY(BlueprintReadOnly) FString SecondaryReadout;
    UPROPERTY(BlueprintReadOnly) int32 ShieldStrength = 0;
    UPROPERTY(BlueprintReadOnly) int32 FuelPercent = 0;
    UPROPERTY(BlueprintReadOnly) float Throttle = 0;
    UPROPERTY() float DrivePower = 0;
    UPROPERTY() TArray<uint8> ThrusterBurns;
    UPROPERTY() uint32 EnabledNavLights = 0;
    UPROPERTY() TArray<FName> CarriedStores;
    UPROPERTY() float SensorRange = 1.0f;
    UPROPERTY() int32 RespawnsRemaining = 0;
    UPROPERTY(BlueprintReadOnly) int32 PrimaryAmmo = 0;
    UPROPERTY(BlueprintReadOnly) int32 SecondaryAmmo = 0;
    UPROPERTY() float PrimaryRange = 0;
    UPROPERTY() float PrimaryShotSpeed = 0;
    UPROPERTY() bool bMissileLocked = false;
    UPROPERTY(BlueprintReadOnly) int64 TargetId = 0;
    UPROPERTY(BlueprintReadOnly) bool bAutoTracking = false;
    UPROPERTY(BlueprintReadOnly) int32 Emcon = 0;
    UPROPERTY(BlueprintReadOnly) int32 ShotsFired = 0;
    UPROPERTY(BlueprintReadOnly) int32 DamageEvents = 0;
    UPROPERTY(BlueprintReadOnly) bool bDestroyed = false;
    UPROPERTY(BlueprintReadOnly) bool bRetired = false;
    UPROPERTY(BlueprintReadOnly) bool bPlayerControlled = false;
    UPROPERTY(BlueprintReadOnly) TArray<FSSWReplicatedSystemState> Systems;
    UPROPERTY(BlueprintReadOnly) TArray<FSSWReplicatedNavPoint> FlightPlan;
    UPROPERTY(BlueprintReadOnly) int32 FlightPhase = 0;
    UPROPERTY() bool bAirborne = false;
    UPROPERTY(BlueprintReadOnly) int64 CarrierId = 0;
    UPROPERTY(BlueprintReadOnly) bool bLandingAssigned = false;
};

UENUM(BlueprintType)
enum class ESSWChatChannel : uint8
{
    Broadcast,
    Team,
    Wing,
    Direct
};

UCLASS()
class STARSHATTERWARS_API ASSWSessionPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(ReplicatedUsing=OnRep_Ready, BlueprintReadOnly)
    bool bReady = false;

    UFUNCTION() void OnRep_Ready();
    virtual void OnRep_PlayerName() override;
};

USTRUCT(BlueprintType)
struct FSSWSessionChatMessage
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FString Sender;
    UPROPERTY(BlueprintReadOnly) FString Text;
    UPROPERTY(BlueprintReadOnly) ESSWChatChannel Channel = ESSWChatChannel::Broadcast;
};

USTRUCT()
struct FSSWOrbitalPosition
{
    GENERATED_BODY()

    UPROPERTY() FString Key;
    UPROPERTY() FVector Position = FVector::ZeroVector;
    UPROPERTY() double Phase = 0.0;
};

UCLASS()
class STARSHATTERWARS_API ASSWSessionGameState : public AGameState
{
    GENERATED_BODY()
public:
    UPROPERTY(Replicated, BlueprintReadOnly) TArray<FSSWReplicatedShipState> MissionShips;
    UPROPERTY(Replicated, BlueprintReadOnly) TArray<FSSWReplicatedProjectileState> MissionProjectiles;
    UPROPERTY(Replicated) FString ArenaSector;
    UPROPERTY(Replicated) FVector ArenaVisualCenter = FVector::ZeroVector;
    UPROPERTY(Replicated) FVector ArenaOrbitalOrigin = FVector::ZeroVector;
    UPROPERTY(Replicated) double ArenaRadius = 0.0;

    UPROPERTY(ReplicatedUsing=OnRep_ChatHistory, BlueprintReadOnly)
    TArray<FSSWSessionChatMessage> ChatHistory;

    virtual void AddPlayerState(APlayerState* PlayerState) override;
    virtual void RemovePlayerState(APlayerState* PlayerState) override;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="Multiplayer")
    double MissionStartDeadline = 0.0;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="Multiplayer")
    bool bMissionStartReady = false;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="Multiplayer")
    FString MissionLaunchError;

    FSimpleMulticastDelegate OnRosterChanged;
    FSimpleMulticastDelegate OnChatChanged;
    Ship* FindMissionShip(int64 ObjectId) const;
    void AddChatMessage(const FString& Sender, const FString& Text);
    void BindSimulation(Sim& Simulation);
    void PublishGalaxy(double Now);
    UPROPERTY(Replicated, BlueprintReadOnly) FString MissionMap;
    const FSSWOrbitalPosition* FindOrbitalPosition(const Orbital& Body) const;
    UPROPERTY(Replicated, BlueprintReadOnly) int64 GalaxyClockMs = 0;
    void PublishSimulation(Sim& Simulation, double Now);
    void UnbindSimulation(Sim& Simulation);
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    UFUNCTION() void OnRep_ChatHistory();
    UFUNCTION() void OnRep_OrbitalPositions();

    UPROPERTY(ReplicatedUsing=OnRep_OrbitalPositions)
    TArray<FSSWOrbitalPosition> OrbitalPositions;

    TMap<FString, int32> OrbitalPositionIndices;
    double NextGalaxyUpdate = 0.0;
    void ClearMissionState();
    void ShipStateChanged(const Ship& ChangedShip);
    void ShipDamaged(const Ship& DamagedShip);
    void ProjectileAdded(const SimShot& Shot, const SimRegion& Region);
    void ShipTransferred(const Ship& TransferredShip, const SimRegion& Origin, const SimRegion& Destination);
    void ShipRetired(const Ship& RetiredShip, const SimRegion& Region);
    FSSWReplicatedShipState MakeShipState(const Ship& Source) const;

    Sim* BoundSimulation = nullptr;
    TMap<uint32, int32> ShotCounts;
    TMap<uint32, int32> DamageCounts;
    TMap<uint32, FSSWReplicatedShipState> RetiredShips;
    double NextSnapshotTime = 0.0;
};

UCLASS()
class STARSHATTERWARS_API ASSWSessionLobbyController:public APlayerController {
 GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Multiplayer")
    void SendSessionChat(const FString& Message);

    UFUNCTION(BlueprintCallable, Category="Multiplayer")
    void SendMissionChat(ESSWChatChannel Channel, const FString& Message, int64 RecipientShipId);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Multiplayer")
    bool AssignMissionShip(int64 ObjectId);
    int64 GetMissionShipId() const { return MissionShipId; }
    double GetRespawnDeadline() const { return RespawnDeadline; }
    const TArray<TPair<double, FString>>& GetDepartureNotices() const { return DepartureNotices; }
    const FSSWReplicatedShipState* GetAssignedShipState() const;
    const TArray<FSSWReplicatedShipState>* GetFlightContacts() const;

    UFUNCTION(Server, Reliable, BlueprintCallable, Category="Multiplayer")
    void SetLobbyReady(bool bReady);

    UPROPERTY(BlueprintReadOnly, Category="Multiplayer")
    TArray<FSSWSessionChatMessage> PrivateChatHistory;

protected:
    virtual void BeginPlay() override;
    virtual bool InputKey(const FInputKeyEventArgs& Params) override;

private:
    UFUNCTION(Server, Reliable)
    void ServerSendSessionChat(const FString& Message);

    UFUNCTION(Server, Reliable)
    void ServerSetPlatformPilotName(const FString& DisplayName);

    UFUNCTION(Server, Reliable)
    void ServerSendMissionChat(ESSWChatChannel Channel, const FString& Message, int64 RecipientShipId);

    UFUNCTION(Client, Reliable)
    void ReceiveMissionChat(ESSWChatChannel Channel, const FSSWSessionChatMessage& Message);

    UFUNCTION(Client, Reliable)
    void ReceivePilotDeparture(const FString& Message);

    TArray<TPair<double, FString>> DepartureNotices;

    bool AcceptChat(const FString& Message);

    friend class ASSWSessionLobbyGameMode;
    UPROPERTY(Replicated) int64 MissionShipId = 0;
    UPROPERTY(Replicated) double RespawnDeadline = 0.0;
    double NextChatTime = 0.0;
    bool bMissionMapRequested = false;

    UPROPERTY(Transient) TObjectPtr<ULevelStreamingDynamic> ClientMissionLevel;
    TWeakObjectPtr<ASystemSceneBuilder> ClientSceneBuilder;
    UFUNCTION() void BuildClientMissionScene();
    double LastFlightInput = 0.0;
    double NextFlightInput = 0.0;
    double FlightThrottle = 0.0;
    FVector PendingTranslation = FVector::ZeroVector;
    UPROPERTY(Transient) TObjectPtr<UEnhancedInputComponent> FlightInputComponent;
    UPROPERTY(Transient) TArray<TObjectPtr<UInputMappingContext>> FlightInputContexts;
    UPROPERTY(Transient) TMap<FName, TObjectPtr<UInputAction>> FlightActions;
    void BindFlightInputs();
    void OnFlightButton(const FInputActionValue& Value, FName Action);
    void ReleaseSensorRange(const FInputActionValue& Value, FName Action);
    TSet<FName> HeldSensorRange;
    double ChaseZoom = 1.0;
    void OpenMissionChat();
    void CloseMissionChat();
    void RefreshMissionChat();
    void SubmitMissionChat(const FString& Message);
    void OpenResignDialog(bool bSelfDestruct = false);
    void CloseResignDialog();
    TSharedPtr<SWidget> ResignDialog;
    TSharedPtr<SWidget> MissionChatPanel;
    TSharedPtr<SEditableTextBox> MissionChatEntry;
    TSharedPtr<SScrollBox> MissionChatLog;
    ESSWChatChannel MissionChatChannel = ESSWChatChannel::Broadcast;
    int64 LastControlledShipId = 0;

    UFUNCTION(Server, Unreliable)
    void ServerFlightInput(FVector Rotation, FVector Translation, float Throttle,
        bool bAugmenter, bool bFirePrimary);

    UFUNCTION(Server, Reliable)
    void ServerFireSecondary();

    UFUNCTION(Server, Reliable)
    void ServerTargetCommand(FName Action);

    UFUNCTION(Server, Reliable)
    void ServerSensorCommand(FName Action);

    UFUNCTION(Server, Reliable)
    void ServerSelfDestruct();

    bool bPrimaryPressed = false;

    UPROPERTY(Transient) TObjectPtr<UMultiplayerDlg> LobbyDialog;
    UPROPERTY(Transient) TObjectPtr<ACameraActor> FlightCamera;
    UPROPERTY(Transient) TObjectPtr<ACameraActor> LobbyCamera;
    UPROPERTY(Transient) TObjectPtr<AActor> ArenaBarrier;
    TSharedPtr<SFighterHUDDetails> FlightMiniHUD;
    UPROPERTY(Transient) TMap<int64, TObjectPtr<AShipActor>> RemoteFighters;

    virtual void PlayerTick(float DeltaTime) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    void UpdateFlightView(const ASSWSessionGameState& State, const FSSWReplicatedShipState& Fighter, float DeltaTime);
    void SetLobbySpaceView(const ASystemSceneBuilder& Builder, const FString& Sector);
};
UCLASS()
class STARSHATTERWARS_API ASSWSessionFlightHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
};
UCLASS()
class STARSHATTERWARS_API ASSWSessionLobbyGameMode:public AGameMode {
 GENERATED_BODY()
public:
    ASSWSessionLobbyGameMode();
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;
    int32 CountPendingArenaRespawns(Sim& Simulation, int32 Iff) const;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(EditDefaultsOnly, Category="Multiplayer", meta=(ClampMin="2"))
    int32 MinimumMissionPilots = 2;

    UPROPERTY(EditDefaultsOnly, Category="Multiplayer", meta=(ClampMin="1"))
    float MissionCountdownSeconds = 10.0f;

    UFUNCTION(BlueprintImplementableEvent, Category="Multiplayer")
    void OnMissionCountdownCompleted();

private:
    void UpdateMissionCountdown();
    void UpdateArena();
    bool LaunchLobbyMission(ASSWSessionGameState& State);
    TSharedPtr<Mission> LobbyMission;
    int32 LobbyMissionIndex = INDEX_NONE;
    UPROPERTY(Transient) TObjectPtr<ULevelStreamingDynamic> LobbySystemLevel;
    TWeakObjectPtr<ASystemSceneBuilder> LobbySceneBuilder;

    FTimerHandle MissionCountdownTimer;
    FTimerHandle ArenaTimer;

    struct FPilotSpawn
    {
        FString Name;
        FString Registry;
        ShipDesign* Design = nullptr;
        FVector Position = FVector::ZeroVector;
        FVector LastPosition = FVector::ZeroVector;
        double Radius = 0.0;
        double Roll = 0.0;
        double Pitch = 0.0;
        double Yaw = 0.0;
        int32 Iff = 0;
        int32 CommandAI = 0;
        int32 Remaining = 3;
        double RespawnAt = 0.0;
        bool bWasSimulationPlayer = false;
        int32 ElementIndex = 0;
        FString ElementName;
        TArray<int32> Loadout;
    };
    TMap<TWeakObjectPtr<ASSWSessionLobbyController>, FPilotSpawn> PilotSpawns;
};
