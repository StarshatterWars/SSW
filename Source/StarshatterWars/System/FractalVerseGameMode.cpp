#include "FractalVerseGameMode.h"
#include "PanelCloseIcon.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "StarshatterInputBindings.h"
#include "FractalVerseSubsystem.h"
#include "FractalVerseIdentity.h"
#include "FractalVerseStatusWidget.h"
#include "SSWGameInstance.h"
#include "Engine/GameViewportClient.h"
#include "HAL/PlatformMisc.h"
#include "Widgets/SOverlay.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Engine/GameInstance.h"
#include "HAL/PlatformTime.h"
#include "TimerManager.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

AFractalVerseController::AFractalVerseController() { bReplicates = true; PrimaryActorTick.bCanEverTick=true; }
void AFractalVerseController::BeginPlay()
{
    APlayerController::BeginPlay();
    if (IsLocalController() && GetNetMode() == NM_Client)
    {
        if (FParse::Param(FCommandLine::Get(),TEXT("FractalVerseSignIn")))
        {
            auto* Instance=GetGameInstance();
            if (auto* Viewport=Instance?Instance->GetGameViewportClient():nullptr)
            {
                TWeakObjectPtr<UFractalVerseSubsystem> Service(Instance->GetSubsystem<UFractalVerseSubsystem>());
                ConnectionReview=SNew(SOverlay)
                    +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
                    [MissionUIStyle::MakeDataPane(SNew(SVerticalBox)
                        +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)
                            [SNew(STextBlock).Text(INVTEXT("FRACTALVERSE · CONNECTION REVIEW")).Font(MissionUIStyle::GetHeaderFont(20))]
                        +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)
                            [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(14)).Text_Lambda([Service]
                            {
                                if (!Service.IsValid()) return INVTEXT("Server unavailable.");
                                const auto& Pilot=Service->Pilot();
                                return Pilot.bAuthenticated ? FText::FromString(FString::Printf(TEXT("PILOT CONFIRMED: %s\nCREDITS: %lld\nHOME: %s / %s"),
                                    *Pilot.Callsign,Pilot.Credits,*Pilot.HomeSystem.ToString(),*Pilot.HomeRegion.ToString()))
                                    : INVTEXT("Waiting for sign-in and server-confirmed pilot data.");
                            })]
                        +SVerticalBox::Slot().AutoHeight()[SNew(SFractalVerseStatusPanel).GameInstance(Instance)]
                        +SVerticalBox::Slot().AutoHeight().Padding(0,12,0,0)
                            [SNew(STextBlock).Text(INVTEXT("Connection check only. Local career saves are not loaded or written.")).Font(MissionUIStyle::GetRowFont(11))]
                        +SVerticalBox::Slot().AutoHeight().Padding(0,12,0,0)
                            [SNew(SButton).ContentPadding(FMargin(8)).HAlign(HAlign_Center).VAlign(VAlign_Center).ButtonColorAndOpacity(FLinearColor(.55f,.09f,.07f)).ToolTipText(INVTEXT("Close this connection check"))
                                .OnClicked_Lambda([](){FPlatformMisc::RequestExit(false);return FReply::Handled();})
                                [SNew(SPanelCloseIcon)]])];
                Viewport->AddViewportWidgetContent(ConnectionReview.ToSharedRef(),100);
                bShowMouseCursor=true; SetInputMode(FInputModeUIOnly());
            }
            if (auto* Game=Cast<USSWGameInstance>(GetGameInstance())) Game->ShowMainMenuScreen();
        }
        if (auto* Online=IOnlineSubsystem::Get(TEXT("EOS"))) if (auto Identity=Online->GetIdentityInterface())
            LoginStatusHandle=Identity->AddOnLoginStatusChangedDelegate_Handle(0,FOnLoginStatusChangedDelegate::CreateWeakLambda(this,
                [this](int32,ELoginStatus::Type,ELoginStatus::Type Current,const FUniqueNetId&)
                {
                    if (Current==ELoginStatus::LoggedIn) {bProofSent=false;return;}
                    bProofSent=true; ServerLogout();
                    if (auto* Service=GetGameInstance()->GetSubsystem<UFractalVerseSubsystem>()) Service->ReceivePilot(FFractalVersePilotView());
                    if (FParse::Param(FCommandLine::Get(),TEXT("FractalVerseSignIn")))
                        if (auto* Game=Cast<USSWGameInstance>(GetGameInstance())) Game->ShowMainMenuScreen();
                }));
        if (auto* Service=GetGameInstance()->GetSubsystem<UFractalVerseSubsystem>()) Service->BeginClientConnection();
        RequestStatus();
        if (FParse::Param(FCommandLine::Get(), TEXT("FractalVerseProbeTransfer")))
        {
            ServerAuthenticate(TEXT("not-an-account"),TEXT("not-a-token"));
            FFractalVerseTransfer Probe; Probe.Request=FGuid::NewGuid(); Probe.Source=TEXT("Untrusted hold");
            Probe.Destination=TEXT("Untrusted locker"); Probe.Item=TEXT("Ore"); Probe.Quantity=1;
            ServerTransfer(Probe);
        }
        GetWorldTimerManager().SetTimer(StatusTimer, this, &AFractalVerseController::RequestStatus, 2, true);
    }
}
void AFractalVerseController::EndPlay(const EEndPlayReason::Type Reason)
{
    GetWorldTimerManager().ClearTimer(StatusTimer);
    if (LoginStatusHandle.IsValid())
    {
        if (auto* Online=IOnlineSubsystem::Get(TEXT("EOS"))) if (auto Identity=Online->GetIdentityInterface())
            Identity->ClearOnLoginStatusChangedDelegate_Handle(0,LoginStatusHandle);
        LoginStatusHandle.Reset();
    }
    if (ConnectionReview)
    {
        if (auto* Instance=GetGameInstance()) if (auto* Viewport=Instance->GetGameViewportClient())
            Viewport->RemoveViewportWidgetContent(ConnectionReview.ToSharedRef());
        ConnectionReview.Reset();
    }
    if (IsLocalController() && GetNetMode()==NM_Client)
        if (auto* Instance=GetGameInstance())
            if (auto* Service=Instance->GetSubsystem<UFractalVerseSubsystem>()) Service->ReceivePilot(FFractalVersePilotView());
    if (auto* Instance=GetGameInstance())
        if (auto* Service=Instance->GetSubsystem<UFractalVerseSubsystem>()) Service->ReleaseShip(ControlledPrincipal);
    ControlledPrincipal=NAME_None;
    Session.Clear(); ++AuthenticationEpoch;
    Super::EndPlay(Reason);
}
void AFractalVerseController::RequestStatus()
{
    ServerRequestStatus(FPlatformTime::Seconds());
    if (!bProofSent)
    {
        FString User,Token;
        if (FFractalVerseIdentity::CopyLocalProof(User,Token)) { bProofSent=true; ServerAuthenticate(User,Token); }
    }
}
void AFractalVerseController::ServerAuthenticate_Implementation(const FString& ProductUserId,const FString& IdToken)
{
    const double Now=FPlatformTime::Seconds();
    if ((!Session.Principal(Now).IsNone() && !bRefreshRequested) || bVerificationPending || AuthenticationAttempts>=3 || Now-LastAuthentication<1) return;
    LastAuthentication=Now; ++AuthenticationAttempts;
    if (!FFractalVerseIdentity::IsProofWellFormed(ProductUserId,IdToken)) { ClientPilot(FFractalVersePilotView()); return; }
    bVerificationPending=true;
    const uint32 Epoch=AuthenticationEpoch;
    TWeakObjectPtr<AFractalVerseController> WeakThis(this);
    FFractalVerseIdentity::VerifyAccount(ProductUserId,IdToken,[WeakThis,Now,Epoch](FFractalVerseVerifiedIdentity Identity)
    {
        auto* Controller=WeakThis.Get();
        if (!Controller || Controller->IsActorBeingDestroyed() || !Controller->GetWorld() || !Controller->GetGameInstance() || Controller->AuthenticationEpoch!=Epoch) return;
        Controller->bVerificationPending=false;
        const FName Principal=Identity.Principal;
        if (Principal.IsNone() || FPlatformTime::Seconds()-Now>20 || !Controller->Session.MayBind(Principal))
        { Controller->Session.Clear(); Controller->ClientPilot(FFractalVersePilotView()); return; }
        // One active connection owns a pilot; reconnect cannot create concurrent writers.
        for (auto Iter=Controller->GetWorld()->GetPlayerControllerIterator();Iter;++Iter)
            if (auto* Other=Cast<AFractalVerseController>(Iter->Get()); Other && Other!=Controller && !Other->IsActorBeingDestroyed() && Other->Session.Principal(FPlatformTime::Seconds())==Principal)
            { Controller->ClientPilot(FFractalVersePilotView()); return; }
        FFractalVersePilotView View;
        auto* Service=Controller->GetGameInstance()->GetSubsystem<UFractalVerseSubsystem>();
        if (Service && Service->BindVerifiedPilot(Principal,Identity.LegacySlot,View))
        {
            if (!Controller->Session.Bind(Principal,FPlatformTime::Seconds())) {Controller->ClientPilot(FFractalVersePilotView());return;}
            Controller->bRefreshRequested=false; Controller->AuthenticationAttempts=0;
            if (Controller->PlayerState) Controller->PlayerState->SetPlayerName(View.Callsign);
        }
        Controller->ClientPilot(View);
    });
}
void AFractalVerseController::ServerLogout_Implementation()
{
    if (auto* Instance=GetGameInstance())
        if (auto* Service=Instance->GetSubsystem<UFractalVerseSubsystem>()) Service->ReleaseShip(ControlledPrincipal);
    ControlledPrincipal=NAME_None;
    Session.Clear(); ++AuthenticationEpoch; bVerificationPending=false; bRefreshRequested=false;
    ClientPilot(FFractalVersePilotView());
}
void AFractalVerseController::ClientAuthRequired_Implementation()
{ bProofSent=false; }
void AFractalVerseController::ClientPilot_Implementation(const FFractalVersePilotView& Pilot)
{
    if (auto* Service=GetGameInstance()->GetSubsystem<UFractalVerseSubsystem>()) Service->ReceivePilot(Pilot);
    UE_LOG(LogTemp,Display,TEXT("[FractalVerseClient] Pilot binding confirmed=%d"),Pilot.bAuthenticated);
}
void AFractalVerseController::ServerRequestStatus_Implementation(double SentSeconds)
{
    if (!FMath::IsFinite(SentSeconds)) return;
    const double Now = FPlatformTime::Seconds();
    if (Now - LastRequest < .5) return;
    LastRequest = Now;
    if (auto* Service = GetGameInstance()->GetSubsystem<UFractalVerseSubsystem>())
    {
        ClientStatus(Service->Status(),SentSeconds);
        FFractalVersePilotView View;
        const FName Principal=Session.Principal(Now);
        if (Session.HasBinding() && Principal.IsNone())
        {
            Session.Clear(); bRefreshRequested=true; ClientPilot(FFractalVersePilotView()); ClientAuthRequired();
        }
        else if (Session.NeedsRefresh(Now) && !bRefreshRequested)
        { bRefreshRequested=true; ClientAuthRequired(); }
        if (!Principal.IsNone() && Service->ReadPilot(Principal,View)) ClientPilot(View);
    }
}
void AFractalVerseController::ClientStatus_Implementation(const FFractalVerseStatus& State,double SentSeconds)
{ if (auto* Service = GetGameInstance()->GetSubsystem<UFractalVerseSubsystem>()) { Service->RecordPing(SentSeconds); Service->ReceiveStatus(State); } }
void AFractalVerseController::ServerTransfer_Implementation(const FFractalVerseTransfer& Command)
{
    // No docking or pilot authorization is accepted from the client. Until the
    // server docking and inventory-access binding is migrated, these requests intentionally fail closed.
    const double Now=FPlatformTime::Seconds();
    if (Now-LastTransfer<.1) return;
    LastTransfer=Now;
    FFractalVerseAccess Access;
    Access.Principal=Session.Principal(Now);
    FString Reason;
    auto* Service = GetGameInstance()->GetSubsystem<UFractalVerseSubsystem>();
    const bool Accepted = Service && Service->Submit(Access, Command, Reason);
    ClientTransferResult(Command.Request, Accepted, Service ? Reason : TEXT("Universe server unavailable."));
}
void AFractalVerseController::ClientTransferResult_Implementation(FGuid Request, bool Accepted, const FString& Reason)
{ UE_LOG(LogTemp, Display, TEXT("[FractalVerseClient] Request %s accepted=%d %s"), *Request.ToString(), Accepted, *Reason); }

AFractalVerseGameMode::AFractalVerseGameMode()
{ PlayerControllerClass=AFractalVerseController::StaticClass(); GameStateClass=ASSWSessionGameState::StaticClass(); PlayerStateClass=ASSWSessionPlayerState::StaticClass(); HUDClass=ASSWSessionFlightHUD::StaticClass(); DefaultPawnClass=nullptr; bPauseable=false; bStartPlayersAsSpectators=true; bDelayedStart=true; }
void AFractalVerseGameMode::InitGame(const FString& MapName,const FString& Options,FString& Error)
{
    Super::InitGame(MapName,Options,Error);
    if (GameSession) { GameSession->MaxPlayers=FFractalVerseStatus::ClientLimit; GameSession->MaxSpectators=0; }
}
void AFractalVerseGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& Error)
{
    Super::PreLogin(Options, Address, UniqueId, Error);
    if (!Error.IsEmpty()) return;
    if (UGameplayStatics::GetIntOption(Options,TEXT("FractalVerseProtocol"),0)!=1)
    { Error=TEXT("FractalVerse client protocol does not match this server."); return; }
    if (GetNumPlayers()+GetNumSpectators()>=FFractalVerseStatus::ClientLimit) { Error=TEXT("FractalVerse is full (8 clients)."); return; }
    // This first phase is local only; no EOS registration, arena lobby or public listener.
    FString Host, Port;
    const bool HasPort = Address.Split(TEXT(":"), &Host, &Port);
    if (UFractalVerseSubsystem::ProcessRole() != EFractalVerseRole::Server || (HasPort ? Host : Address) != TEXT("127.0.0.1"))
        Error = TEXT("FractalVerse currently accepts local clients only.");
}

void AFractalVerseController::ServerPilotInput_Implementation(const FFractalVerseFlightInput& Input)
{
    const FName Principal=Session.Principal(FPlatformTime::Seconds());
    if (!Principal.IsNone()) if (auto* Service=GetGameInstance()->GetSubsystem<UFractalVerseSubsystem>()) Service->SubmitFlight(Principal,Input);
}
void AFractalVerseController::ClientShip_Implementation(const FFractalVerseShipView& View)
{
    if (auto* Service=GetGameInstance()->GetSubsystem<UFractalVerseSubsystem>()) Service->ReceiveShip(View);
}
void AFractalVerseController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!HasAuthority())
    {
        TickFlightProbe();
        if(GetAssignedShipState() && ConnectionReview)
        { if(auto* Instance=GetGameInstance()) if(auto* Viewport=Instance->GetGameViewportClient()) Viewport->RemoveViewportWidgetContent(ConnectionReview.ToSharedRef()); ConnectionReview.Reset(); }
        return;
    }
    auto* Instance=GetGameInstance(); auto* Service=Instance?Instance->GetSubsystem<UFractalVerseSubsystem>():nullptr;
    if (!Service) return;
    const double Now=FPlatformTime::Seconds(); const FName Principal=Session.Principal(Now);
    if (ControlledPrincipal!=Principal)
    { Service->ReleaseShip(ControlledPrincipal); AssignMissionShip(0); ControlledPrincipal=Principal; ClientShip(FFractalVerseShipView()); }
    if (Now-LastShipSnapshot<.05 || Principal.IsNone()) return;
    LastShipSnapshot=Now;
    FFractalVerseShipView View;
    if(!Service->ReadShip(Principal,View)) Service->ProvisionShip(Principal,View);
    if(View.ObjectId && uint32(GetMissionShipId())!=View.ObjectId) AssignMissionShip(View.ObjectId);
    ClientShip(View);
}

void AFractalVerseController::ServerShipCommand_Implementation(FName Action)
{
    const double Now=FPlatformTime::Seconds(); if(Now-LastShipCommand<.05) return;
    const FName Principal=Session.Principal(Now); if(Principal.IsNone()) return;
    LastShipCommand=Now;
    if(auto* Service=GetGameInstance()->GetSubsystem<UFractalVerseSubsystem>()) Service->ShipCommand(Principal,Action);
}

bool AFractalVerseController::CanSendNetworkCommand() const
{ return HasAuthority() && !Session.Principal(FPlatformTime::Seconds()).IsNone(); }
bool AFractalVerseController::CanUseNetworkFlight() const
{
    if(HasAuthority()) return CanSendNetworkCommand();
    const auto* Instance=GetGameInstance(); const auto* Service=Instance?Instance->GetSubsystem<UFractalVerseSubsystem>():nullptr;
    return Service && Service->IsConnected() && Service->Pilot().bAuthenticated;
}
void AFractalVerseController::ServerFlightInput_Implementation(uint64 Packed,uint8 Throttle)
{
    if(!CanSendNetworkCommand() || (Packed>>62)!=0 || ReceivedFlightSequence==MAX_int64) return;
    FFractalVerseFlightInput Input; Input.Sequence=++ReceivedFlightSequence; Input.Throttle=Throttle*(100.f/255.f);
    for(int32 Axis=0;Axis<3;++Axis)
    {Input.Rotation[Axis]=FMath::Clamp((int32((Packed>>(Axis*10))&1023)-511)/511.,-1.,1.);
     Input.Translation[Axis]=FMath::Clamp((int32((Packed>>((Axis+3)*10))&1023)-511)/511.,-1.,1.);}
    Input.bAugmenter=(Packed&(1ull<<60))!=0; Input.bPrimary=(Packed&(1ull<<61))!=0;
    ServerPilotInput_Implementation(Input);
}
void AFractalVerseController::ServerFireSecondary_Implementation() {ServerShipCommand_Implementation(TEXT("IA_FireSecondary"));}
void AFractalVerseController::ServerTargetCommand_Implementation(FName Action) {ServerShipCommand_Implementation(Action);}
void AFractalVerseController::ServerSensorCommand_Implementation(FName Action) {ServerShipCommand_Implementation(Action);}
void AFractalVerseController::ServerSelfDestruct_Implementation()
{
    // Destruction/insurance settlement must migrate before exposing this arena action.
}

bool AFractalVerseController::ShouldPollFlightInput() const
{
#if !UE_BUILD_SHIPPING
    if(FParse::Param(FCommandLine::Get(),TEXT("FractalVerseFlightProbe"))) return false;
#endif
    return true;
}
void AFractalVerseController::TickFlightProbe()
{
#if !UE_BUILD_SHIPPING
    if(!FParse::Param(FCommandLine::Get(),TEXT("FractalVerseFlightProbe")) || !CanUseNetworkFlight()) return;
    const auto* Fighter=GetAssignedShipState(); const auto* Contacts=GetFlightContacts(); if(!Fighter || !Contacts) return;
    if(!HasFlightPresentation()) return;
    const double Now=FPlatformTime::Seconds(); if(ProbeStart==0) ProbeStart=Now;
    const double Elapsed=Now-ProbeStart;
    const auto* Target=Contacts->FindByPredicate([](const auto& Entry){return Entry.Name==TEXT("FractalVerse probe target");});
    bProbeMovement|=Fighter->Velocity.Size()>1;
    bProbeDamage|=Target && Target->DamageEvents>0 && Target->HullPercent<100;
    if(Now-ProbeLastInput>=.05)
    {
        ProbeLastInput=Now; FFractalVerseFlightInput Intent; Intent.Sequence=++ProbeSequence;
        Intent.Throttle=Elapsed<6?30:0; Intent.bPrimary=Elapsed>10 && !bProbeDamage;
        if(bProbeDamage && Elapsed<18) Intent.Translation=FVector(.4,0,0);
        ServerPilotInput(Intent);
    }
    if(Now-ProbeLastLog>=1)
    {
        ProbeLastLog=Now;
        if(Elapsed>4 && !bProbeDamage) ServerShipCommand(TEXT("IA_TargetHostile"));
        UE_LOG(LogTemp,Display,TEXT("[FractalVerseFlightProbe] client object=%lld speed=%.3f shots=%d targetHull=%.3f damage=%d scene=%d"),
            Fighter->ObjectId,Fighter->Velocity.Size(),Fighter->ShotsFired,Target?Target->HullPercent:-1.f,Target?Target->DamageEvents:0,HasFlightPresentation());
    }
    if(!bProbeComplete && bProbeMovement && bProbeDamage && HasFlightPresentation())
    {bProbeComplete=true;UE_LOG(LogTemp,Display,TEXT("[FractalVerseFlightProbe] PASSED movement, rendered ship, HUD and replicated native damage."));}
#endif
}
