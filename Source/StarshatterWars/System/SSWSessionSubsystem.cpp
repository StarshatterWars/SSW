#include "SSWSessionSubsystem.h"
#include "StarshatterGameDataSubsystem.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineSubsystemUtils.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Misc/PackageName.h"
#include "Misc/NetworkVersion.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "SSWGameInstance.h"
#include "SSWSessionLobbyGameMode.h"
#include "UObject/UObjectGlobals.h"
#include "TimerManager.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
namespace {
const FName SSWProjectKey(TEXT("SSW_PROJECT")), SSWTitleKey(TEXT("SSW_TITLE")), SSWMissionKey(TEXT("SSW_MISSION"));

void ApplyEOSNetworkIdentity(APlayerController& Controller)
{
    auto* LocalPlayer = Controller.GetLocalPlayer();
    auto* EOS = IOnlineSubsystem::Get(FName(TEXT("EOS")));
    const auto Identity = EOS ? EOS->GetIdentityInterface() : nullptr;
    if (LocalPlayer && Identity.IsValid())
        LocalPlayer->SetCachedUniqueNetId(FUniqueNetIdRepl(Identity->GetUniquePlayerId(0)));
}
}
void USSWSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection){Super::Initialize(Collection);if(GEngine){NetworkFailureHandle=GEngine->OnNetworkFailure().AddUObject(this,&ThisClass::NetworkFailure);TravelFailureHandle=GEngine->OnTravelFailure().AddUObject(this,&ThisClass::TravelFailure);MapLoadedHandle=FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this,&ThisClass::MapLoaded);}}
bool USSWSessionSubsystem::HasSession() const{return Sessions.IsValid() && Sessions->GetNamedSession(NAME_GameSession)!=nullptr;}
void USSWSessionSubsystem::NetworkFailure(UWorld* World,UNetDriver*,ENetworkFailure::Type,const FString& Message){if(World && World==GetWorld())Change(ESSWSessionState::Failed,TEXT("Connection failed: ")+Message);}
void USSWSessionSubsystem::TravelFailure(UWorld* World,ETravelFailure::Type,const FString& Message){if(World && World==GetWorld())Change(ESSWSessionState::Failed,TEXT("Could not open session: ")+Message);}
bool USSWSessionSubsystem::IsBusy() const {return State==ESSWSessionState::Creating||State==ESSWSessionState::Searching||State==ESSWSessionState::Joining||State==ESSWSessionState::Leaving;}
void USSWSessionSubsystem::Change(ESSWSessionState S,const FString& Message){State=S;Status=Message;OnChanged.Broadcast();}
FOnlineSessionSettings USSWSessionSubsystem::MakeSettings(const FString& Name,int32 Players,bool bLAN){
 FOnlineSessionSettings S;S.NumPublicConnections=FMath::Clamp(Players,1,FS_CampaignMission::MultiplayerConnectionLimit);S.bIsLANMatch=bLAN;S.bShouldAdvertise=true;S.bAllowJoinInProgress=true;S.bAllowInvites=false;S.bUsesPresence=false;S.bUseLobbiesIfAvailable=false;S.BuildUniqueId=GetBuildUniqueId();
 S.Set(SSWProjectKey,FString(TEXT("StarshatterWars")),EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
 S.Set(SSWTitleKey,Name.TrimStartAndEnd().Left(48),EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);return S;
}
bool USSWSessionSubsystem::Acquire(bool bLAN){
 if(IsBusy())return false;
 UWorld* World=GetWorld();
 IOnlineSubsystem* OSS = bLAN ? (World ? Online::GetSubsystem(World, FName(TEXT("NULL"))) : nullptr)
                               : IOnlineSubsystem::Get(FName(TEXT("EOS")));
 if(!OSS || !OSS->GetSessionInterface().IsValid()){Change(ESSWSessionState::Failed,TEXT("Session service is unavailable."));return false;}
 if(!bLAN && OSS->GetSubsystemName()==FName(TEXT("NULL"))){Change(ESSWSessionState::Failed,TEXT("Internet play requires a configured online service. LAN is available."));return false;}
 auto Identity=OSS->GetIdentityInterface();
 if(!Identity.IsValid()){Change(ESSWSessionState::Failed,TEXT("Player identity service is unavailable."));return false;}
 if(!Identity->GetUniquePlayerId(0).IsValid()){
  if(OSS->GetSubsystemName()==FName(TEXT("NULL")))Identity->AutoLogin(0);
  if(!Identity->GetUniquePlayerId(0).IsValid()){Change(ESSWSessionState::Failed,TEXT("Sign in to the online service before creating or joining a game."));return false;}
 }
 Sessions=OSS->GetSessionInterface();bUsingEOS=!bLAN;return true;
}
bool USSWSessionSubsystem::CreateSession(const FString& Name,int32 Players,bool bLAN)
{
    return CreateMissionSession(Name, Players, bLAN, INDEX_NONE);
}

bool USSWSessionSubsystem::CreateMissionSession(const FString& Name,int32 Players,bool bLAN,int32 MissionIndex){
 if(!Acquire(bLAN))return false;
 if(Name.TrimStartAndEnd().IsEmpty()){Change(ESSWSessionState::Failed,TEXT("Enter a session name."));return false;}
 if(!FPackageName::IsValidLongPackageName(LobbyMap)||!FPackageName::DoesPackageExist(LobbyMap)){Change(ESSWSessionState::Failed,TEXT("The lobby map is unavailable."));return false;}
 if(Sessions->GetNamedSession(NAME_GameSession)){Change(ESSWSessionState::Failed,TEXT("Leave the current session before creating another."));return false;}
 CreateHandle=Sessions->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateUObject(this,&ThisClass::Created));
 Change(ESSWSessionState::Creating,TEXT("Creating session..."));
 const auto* Data = GetGameInstance()->GetSubsystem<UStarshatterGameDataSubsystem>();
 const auto* Scenario = Data && Data->GetMultiplayerMissionScenarios().IsValidIndex(MissionIndex)
     ? &Data->GetMultiplayerMissionScenarios()[MissionIndex] : nullptr;
 if (MissionIndex != INDEX_NONE && !Scenario)
 {
     Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
     CreateHandle.Reset();
     Change(ESSWSessionState::Failed, TEXT("The selected mission is unavailable."));
     return false;
 }
 if (!Scenario && Data)
     Scenario = Data->GetMultiplayerMissionScenarios().FindByPredicate([](const FS_CampaignMission& Row)
     {
         return Row.MissionName.Equals(TEXT("Hawk Match"), ESearchCase::IgnoreCase);
     });
 HostingMissionIndex = MissionIndex;
 HostingCapacity = FMath::Clamp(Players, 1, Scenario ? Scenario->GetMultiplayerCapacity() : FS_CampaignMission::MultiplayerConnectionLimit);
 if (!bLAN)
 {
     Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
     CreateHandle.Reset();
     return StartStarServer(Name, HostingCapacity, MissionIndex);
 }
 auto Settings = MakeSettings(Name, HostingCapacity, true);
 Settings.Set(SSWMissionKey, Scenario ? Scenario->MissionName : FString(TEXT("Hawk Match")),
     EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
 if(!Sessions->CreateSession(0,NAME_GameSession,Settings)){Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);CreateHandle.Reset();Change(ESSWSessionState::Failed,TEXT("Session creation could not start."));return false;}return true;
}
void USSWSessionSubsystem::Created(FName Name,bool bOK){
 Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);CreateHandle.Reset();
 if(!bOK){Change(ESSWSessionState::Failed,TEXT("Could not create the session."));return;}
 Sessions->StartSession(Name);Change(ESSWSessionState::Hosting,TEXT("Hosting session."));
 const FString Options = FString::Printf(TEXT("listen?game=/Script/StarshatterWars.SSWSessionLobbyGameMode?SSWMission=%d?SSWPlayers=%d"), HostingMissionIndex, HostingCapacity);
 UGameplayStatics::OpenLevel(this,FName(*LobbyMap),true,Options);
}
bool USSWSessionSubsystem::FindSessions(bool bLAN){
 if(!Acquire(bLAN))return false;
 if(Sessions->GetNamedSession(NAME_GameSession)){Change(ESSWSessionState::Failed,TEXT("Leave the current session before searching."));return false;}
 Results.Reset();SearchResults.Reset();bSearchCancelled=false;
 Search=MakeShared<FOnlineSessionSearch>();Search->bIsLanQuery=bLAN;Search->MaxSearchResults=50;Search->TimeoutInSeconds=10;
 FindHandle=Sessions->AddOnFindSessionsCompleteDelegate_Handle(FOnFindSessionsCompleteDelegate::CreateUObject(this,&ThisClass::Found));
 Change(ESSWSessionState::Searching,TEXT("Searching for sessions..."));
 if(!Sessions->FindSessions(0,Search.ToSharedRef())){Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);FindHandle.Reset();Change(ESSWSessionState::Failed,TEXT("Session search could not start."));return false;}return true;
}
void USSWSessionSubsystem::Found(bool bOK){
 Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);FindHandle.Reset();
 if(bSearchCancelled){Change(ESSWSessionState::Idle,TEXT("Search cancelled."));return;}
 Results.Reset();SearchResults.Reset();
 if(bOK && Search)for(const auto& Match:Search->SearchResults){
  FString Project;Match.Session.SessionSettings.Get(SSWProjectKey,Project);
  if(Project!=TEXT("StarshatterWars") || !Match.IsValid() || Match.Session.SessionSettings.BuildUniqueId!=GetBuildUniqueId())continue;
  FSSWSessionEntry Row;Match.Session.SessionSettings.Get(SSWMissionKey,Row.Mission);Match.Session.SessionSettings.Get(SSWTitleKey,Row.Name);if(Row.Name.IsEmpty())Row.Name=Match.Session.OwningUserName;
  FOnlineSessionSearchResult Result = Match;
  FOnlineSession& Session = Result.Session;
  // The UE 5.7 EOS Sessions adapter copies EOS public slots into the
  // private fields for dedicated session search results.
  if (bUsingEOS && Session.SessionSettings.bIsDedicated && !Session.SessionSettings.bUseLobbiesIfAvailable &&
      Session.SessionSettings.NumPublicConnections > 0 && Session.NumOpenPublicConnections == 0 &&
      Session.NumOpenPrivateConnections > 0)
  {
      Session.NumOpenPublicConnections = Session.NumOpenPrivateConnections;
      Session.NumOpenPrivateConnections = 0;
      Session.SessionSettings.NumPrivateConnections = 0;
  }
  Row.Capacity = Session.SessionSettings.NumPublicConnections;
  Row.Players = FMath::Clamp(Row.Capacity - Session.NumOpenPublicConnections, 0, Row.Capacity);
  Row.Ping = Match.PingInMs;
  Results.Add(Row);
  SearchResults.Add(MoveTemp(Result));
 }
 Change(bOK?ESSWSessionState::Idle:ESSWSessionState::Failed,bOK?FString::Printf(TEXT("%d session(s) found."),Results.Num()):TEXT("Session search failed."));
}
bool USSWSessionSubsystem::JoinSession(int32 Index){
 if(IsBusy()||!Sessions.IsValid()||!SearchResults.IsValidIndex(Index)){Change(IsBusy()?State:ESSWSessionState::Failed,TEXT("Select an available session."));return false;}
 if(Sessions->GetNamedSession(NAME_GameSession)){Change(ESSWSessionState::Failed,TEXT("Leave the current session before joining another."));return false;}
 if(SearchResults[Index].Session.NumOpenPublicConnections<=0){Change(ESSWSessionState::Failed,TEXT("This session is full."));return false;}
 JoinHandle=Sessions->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateUObject(this,&ThisClass::Joined));Change(ESSWSessionState::Joining,TEXT("Joining session..."));
 if(!Sessions->JoinSession(0,NAME_GameSession,SearchResults[Index])){Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);JoinHandle.Reset();Change(ESSWSessionState::Failed,TEXT("Joining could not start."));return false;}return true;
}
void USSWSessionSubsystem::Joined(FName Name,EOnJoinSessionCompleteResult::Type Result){
 Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);JoinHandle.Reset();FString Address;
 if (Result == EOnJoinSessionCompleteResult::SessionIsFull)
 {
     Change(ESSWSessionState::Failed, TEXT("This session is full. Refresh the session list."));
     return;
 }
 if(Result!=EOnJoinSessionCompleteResult::Success||!Sessions->GetResolvedConnectString(Name,Address)){Change(ESSWSessionState::Failed,TEXT("Could not join. Refresh the session list and try again."));return;}
 auto* PC=GetWorld()?GetWorld()->GetFirstPlayerController():nullptr;
 if(!PC){Change(ESSWSessionState::Failed,TEXT("No local player is available to join."));return;}
 if(bUsingEOS)ApplyEOSNetworkIdentity(*PC);
 Change(ESSWSessionState::Joined,TEXT("Connecting to session..."));PC->ClientTravel(Address,TRAVEL_Absolute);
}
bool USSWSessionSubsystem::LeaveSession(){
 if(IsBusy())return false;
 if(!Sessions.IsValid()){IOnlineSubsystem* OSS=GetWorld()?Online::GetSubsystem(GetWorld()):nullptr;if(OSS)Sessions=OSS->GetSessionInterface();}
 if (!Sessions.IsValid() || !Sessions->GetNamedSession(NAME_GameSession))
 {
     if (GetWorld() && GetWorld()->GetFirstPlayerController<ASSWSessionLobbyController>())
     {
         bReturningToMenu = true;
         UGameplayStatics::OpenLevel(this, FName(*MenuMap));
         return true;
     }
     Change(ESSWSessionState::Idle, TEXT("No active session."));
     return false;
 }
 DestroyHandle=Sessions->AddOnDestroySessionCompleteDelegate_Handle(FOnDestroySessionCompleteDelegate::CreateUObject(this,&ThisClass::Left));Change(ESSWSessionState::Leaving,TEXT("Leaving session..."));
 if(!Sessions->DestroySession(NAME_GameSession)){Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);DestroyHandle.Reset();Change(ESSWSessionState::Failed,TEXT("Could not leave session."));return false;}return true;
}
void USSWSessionSubsystem::Left(FName Name,bool bOK){Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);DestroyHandle.Reset();Change(bOK?ESSWSessionState::Idle:ESSWSessionState::Failed,bOK?TEXT("Session closed."):TEXT("Could not close session."));if(bOK){bReturningToMenu=true;UGameplayStatics::OpenLevel(this,FName(*MenuMap));}}
void USSWSessionSubsystem::MapLoaded(UWorld* World){if(!bReturningToMenu||World!=GetWorld())return;bReturningToMenu=false;
 if(bUsingEOS){if(auto* PC=World->GetFirstPlayerController())if(auto* Player=PC->GetLocalPlayer())Player->SetCachedUniqueNetId(FUniqueNetIdRepl());bUsingEOS=false;}if(auto* GI=Cast<USSWGameInstance>(GetGameInstance()))World->GetTimerManager().SetTimerForNextTick(GI,&USSWGameInstance::ShowMainMenuScreen);}
void USSWSessionSubsystem::CancelSearch(){if(State==ESSWSessionState::Searching&&Sessions.IsValid()){bSearchCancelled=true;Sessions->CancelFindSessions();}}
void USSWSessionSubsystem::ClearDelegates(){if(Sessions.IsValid()){
 Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);}
 CreateHandle.Reset();FindHandle.Reset();JoinHandle.Reset();DestroyHandle.Reset();}
void USSWSessionSubsystem::Deinitialize(){if(GetWorld())GetWorld()->GetTimerManager().ClearTimer(DedicatedExitTimeout);ReleaseStarServer(State==ESSWSessionState::Creating);FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(MapLoadedHandle);if(GEngine){GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);GEngine->OnTravelFailure().Remove(TravelFailureHandle);}ClearDelegates();Search.Reset();SearchResults.Reset();Results.Reset();Sessions.Reset();Super::Deinitialize();}

void USSWSessionSubsystem::AdvertiseDedicated(bool bLAN, int32 Players, const FString& Mission)
{
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() != NM_DedicatedServer) return;

    IOnlineSubsystem* OSS = bLAN ? Online::GetSubsystem(World, FName(TEXT("NULL"))) : IOnlineSubsystem::Get(FName(TEXT("EOS")));
    Sessions = OSS ? OSS->GetSessionInterface() : nullptr;
    if (!Sessions.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[StarServer] Session service unavailable"));
        return;
    }

    FString Name = TEXT("StarServer");
    FParse::Value(FCommandLine::Get(), TEXT("ServerName="), Name);
    FOnlineSessionSettings Settings = MakeSettings(Name, Players, bLAN);
    Settings.Set(SSWMissionKey, Mission, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    Settings.bIsDedicated = true;
    Settings.bAllowJoinInProgress = true;

    CreateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
        FOnCreateSessionCompleteDelegate::CreateWeakLambda(this, [this, bLAN](FName SessionName, bool bSuccess)
        {
            FDelegateHandle CompletedHandle = CreateHandle;
            CreateHandle.Reset();
            if (bSuccess)
            {
                Sessions->StartSession(SessionName);
                Change(ESSWSessionState::Hosting, bLAN ? TEXT("StarServer is hosting on LAN.") : TEXT("StarServer is hosting on EOS."));
                FString ReadyFile;
                const auto* Session = Sessions->GetNamedSession(SessionName);
                if (Session && Session->SessionInfo.IsValid() &&
                    FParse::Value(FCommandLine::Get(), TEXT("SSWReadyFile="), ReadyFile))
                {
                    const FString Id = Session->SessionInfo->GetSessionId().ToString();
                    const FString Pending = ReadyFile + TEXT(".tmp");
                    if (FFileHelper::SaveStringToFile(Id, *Pending))
                        IFileManager::Get().Move(*ReadyFile, *Pending, true);
                    UE_LOG(LogTemp, Display, TEXT("[StarServer] Advertised EOS session %s"), *Id);
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[StarServer] Session creation failed"));
            }

            // The NULL backend invokes this callback synchronously. Removing the
            // delegate releases its captured state, so do this after the last use.
            Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CompletedHandle);
        }));
    if (!Sessions->CreateSession(0, NAME_GameSession, Settings))
    {
        Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
        CreateHandle.Reset();
        UE_LOG(LogTemp, Warning, TEXT("[StarServer] Could not advertise the session"));
    }
}

bool USSWSessionSubsystem::StartStarServer(const FString& Name, int32 Players, int32 MissionIndex)
{
#if PLATFORM_WINDOWS
    if (StarServerProcess.IsValid() && FPlatformProcess::IsProcRunning(StarServerProcess))
    {
        Change(ESSWSessionState::Failed, TEXT("A local StarServer is already running."));
        return false;
    }
    ReleaseStarServer(false);

    FString Root = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../../StarServer/WindowsServer"));
    auto ServerExecutable = [](const FString& Directory)
    {
        return Directory / TEXT("StarshatterWars/Binaries/Win64/StarServer.exe");
    };
    auto& Files = IFileManager::Get();
    if (!Files.FileExists(*ServerExecutable(Root)))
        Root = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Build/StarServer/WindowsServer"));

    const FString Executable = ServerExecutable(Root);
    if (!Files.FileExists(*Executable))
    {
        Change(ESSWSessionState::Failed, TEXT("The local StarServer package is missing."));
        return false;
    }

    const FString ConfigDirectory = Root / TEXT("StarshatterWars/Config/Custom/StarServer");
    const TCHAR* PrivateConfigPath = TEXT("Saved/StarServer/Config/Custom/StarServer/DefaultEngine.ini");
    FString PrivateConfig = Root / TEXT("../../") / PrivateConfigPath;
    if (!Files.FileExists(*PrivateConfig))
        PrivateConfig = Root / TEXT("../../../") / PrivateConfigPath;
    const FString InstalledConfig = ConfigDirectory / TEXT("DefaultEngine.ini");
    if (Files.FileExists(*PrivateConfig))
    {
        Files.MakeDirectory(*ConfigDirectory, true);
        if (Files.Copy(*InstalledConfig, *PrivateConfig) != COPY_OK)
        {
            Change(ESSWSessionState::Failed, TEXT("Could not install the local StarServer EOS configuration."));
            return false;
        }
    }
    if (!Files.FileExists(*InstalledConfig))
    {
        Change(ESSWSessionState::Failed, TEXT("The local StarServer EOS configuration is missing."));
        return false;
    }

    FConfigFile ServerConfig;
    ServerConfig.Read(InstalledConfig);
    FString Artifact;
    if (!ServerConfig.GetString(TEXT("/Script/OnlineSubsystemEOS.EOSSettings"), TEXT("DefaultArtifactName"), Artifact) ||
        Artifact.IsEmpty() || Artifact.Contains(TEXT("\"")) || Artifact.Contains(TEXT("\n")) || Artifact.Contains(TEXT("\r")))
    {
        Change(ESSWSessionState::Failed, TEXT("The local StarServer EOS artifact is missing or invalid."));
        return false;
    }

    const FString ReadyDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("StarServer"));
    Files.MakeDirectory(*ReadyDirectory, true);
    StarServerReadyFile = ReadyDirectory / (FGuid::NewGuid().ToString(EGuidFormats::Digits) + TEXT(".ready"));
    FString Title = Name.TrimStartAndEnd().Left(48);
    Title.ReplaceInline(TEXT("\\"), TEXT(" "));
    Title.ReplaceInline(TEXT("\""), TEXT(" "));
    Title.ReplaceInline(TEXT("\r"), TEXT(" "));
    Title.ReplaceInline(TEXT("\n"), TEXT(" "));
    const FString Args = FString::Printf(
        TEXT("\"%s?game=/Script/StarshatterWars.SSWSessionLobbyGameMode?SSWMission=%d?SSWPlayers=%d\" -CustomConfig=StarServer -EOSArtifactNameOverride=\"%s\" -networkversionoverride=%u -SSWEOS -nullrhi -nosound -unattended -log -ServerName=\"%s\" -SSWReadyFile=\"%s\""),
        *LobbyMap, MissionIndex, Players, *Artifact, FNetworkVersion::GetNetworkCompatibleChangelist(), *Title, *StarServerReadyFile);
    StarServerProcess = FPlatformProcess::CreateProc(*Executable, *Args, true, true, true,
        nullptr, 0, *Root, nullptr);
    if (StarServerProcess.IsValid())
    {
        StarServerDeadline = FPlatformTime::Seconds() + 90.0;
        GetWorld()->GetTimerManager().SetTimer(StarServerPoll, this, &ThisClass::PollStarServer, 0.25f, true);
        Change(ESSWSessionState::Creating, TEXT("Starting StarServer and registering the EOS session..."));
        return true;
    }

    ReleaseStarServer(false);
    Change(ESSWSessionState::Failed, TEXT("Could not start the local StarServer."));
#else
    Change(ESSWSessionState::Failed, TEXT("Local StarServer hosting is available on Windows."));
#endif
    return false;
}

void USSWSessionSubsystem::PollStarServer()
{
    if (!StarServerProcess.IsValid() || !FPlatformProcess::IsProcRunning(StarServerProcess) ||
        FPlatformTime::Seconds() >= StarServerDeadline)
    {
        ReleaseStarServer(true);
        Change(ESSWSessionState::Failed, TEXT("StarServer did not register an EOS session. Check the StarServer log."));
        return;
    }

    if (!IFileManager::Get().FileExists(*StarServerReadyFile))
        return;

    GetWorld()->GetTimerManager().ClearTimer(StarServerPoll);
    IFileManager::Get().Delete(*StarServerReadyFile);
    StarServerReadyFile.Reset();
    if (auto* Controller = GetWorld()->GetFirstPlayerController())
    {
        Change(ESSWSessionState::Joined, TEXT("Connecting to StarServer..."));
        ApplyEOSNetworkIdentity(*Controller);
        Controller->ClientTravel(FString::Printf(TEXT("127.0.0.1:%d"), FURL::UrlConfig.DefaultPort), TRAVEL_Absolute);
    }
    else
    {
        ReleaseStarServer(true);
        Change(ESSWSessionState::Failed, TEXT("No local player is available to join StarServer."));
    }
}

void USSWSessionSubsystem::ReleaseStarServer(bool bTerminate)
{
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(StarServerPoll);
    if (!StarServerReadyFile.IsEmpty()) IFileManager::Get().Delete(*StarServerReadyFile);
    StarServerReadyFile.Reset();
    if (StarServerProcess.IsValid())
    {
        if (bTerminate && FPlatformProcess::IsProcRunning(StarServerProcess))
            FPlatformProcess::TerminateProc(StarServerProcess);
        FPlatformProcess::CloseProc(StarServerProcess);
        StarServerProcess.Reset();
    }
}

void USSWSessionSubsystem::CloseDedicatedSessionAndExit()
{
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() != NM_DedicatedServer) return;
    if (!HasSession())
    {
        ExitDedicatedServer();
        return;
    }

    // EOS session removal is asynchronous. Keep ticking while it completes.
    World->GetTimerManager().SetTimer(DedicatedExitTimeout, this, &ThisClass::ExitDedicatedServer, 5.f);
    DestroyHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
        FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::DedicatedSessionClosed));
    if (!Sessions->DestroySession(NAME_GameSession))
    {
        Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
        DestroyHandle.Reset();
        ExitDedicatedServer();
    }
}

void USSWSessionSubsystem::DedicatedSessionClosed(FName Name, bool bOK)
{
    Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
    DestroyHandle.Reset();
    if (!bOK)
        UE_LOG(LogTemp, Warning, TEXT("[StarServer] Online session removal failed during shutdown."));
    ExitDedicatedServer();
}

void USSWSessionSubsystem::ExitDedicatedServer()
{
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(DedicatedExitTimeout);
    UE_LOG(LogTemp, Display, TEXT("[StarServer] Exiting."));
    FPlatformMisc::RequestExit(false);
}
