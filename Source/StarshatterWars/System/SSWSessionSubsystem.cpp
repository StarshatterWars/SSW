#include "SSWSessionSubsystem.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineSubsystemUtils.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Misc/PackageName.h"
#include "Misc/NetworkVersion.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "SSWGameInstance.h"
#include "UObject/UObjectGlobals.h"
#include "TimerManager.h"
namespace { const FName SSWProjectKey(TEXT("SSW_PROJECT")),SSWTitleKey(TEXT("SSW_TITLE")); }
void USSWSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection){Super::Initialize(Collection);if(GEngine){NetworkFailureHandle=GEngine->OnNetworkFailure().AddUObject(this,&ThisClass::NetworkFailure);TravelFailureHandle=GEngine->OnTravelFailure().AddUObject(this,&ThisClass::TravelFailure);MapLoadedHandle=FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this,&ThisClass::MapLoaded);}}
bool USSWSessionSubsystem::HasSession() const{return Sessions.IsValid() && Sessions->GetNamedSession(NAME_GameSession)!=nullptr;}
void USSWSessionSubsystem::NetworkFailure(UWorld* World,UNetDriver*,ENetworkFailure::Type,const FString& Message){if(World && World==GetWorld())Change(ESSWSessionState::Failed,TEXT("Connection failed: ")+Message);}
void USSWSessionSubsystem::TravelFailure(UWorld* World,ETravelFailure::Type,const FString& Message){if(World && World==GetWorld())Change(ESSWSessionState::Failed,TEXT("Could not open session: ")+Message);}
bool USSWSessionSubsystem::IsBusy() const {return State==ESSWSessionState::Creating||State==ESSWSessionState::Searching||State==ESSWSessionState::Joining||State==ESSWSessionState::Leaving;}
void USSWSessionSubsystem::Change(ESSWSessionState S,const FString& Message){State=S;Status=Message;OnChanged.Broadcast();}
FOnlineSessionSettings USSWSessionSubsystem::MakeSettings(const FString& Name,int32 Players,bool bLAN){
 FOnlineSessionSettings S;S.NumPublicConnections=FMath::Clamp(Players,2,16);S.bIsLANMatch=bLAN;S.bShouldAdvertise=true;S.bAllowJoinInProgress=true;S.bAllowInvites=false;S.bUsesPresence=false;S.bUseLobbiesIfAvailable=false;S.BuildUniqueId=GetBuildUniqueId();
 S.Set(SSWProjectKey,FString(TEXT("StarshatterWars")),EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
 S.Set(SSWTitleKey,Name.TrimStartAndEnd().Left(48),EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);return S;
}
bool USSWSessionSubsystem::Acquire(bool bLAN){
 if(IsBusy())return false;
 UWorld* World=GetWorld();IOnlineSubsystem* OSS=World?Online::GetSubsystem(World):nullptr;
 if(!OSS || !OSS->GetSessionInterface().IsValid()){Change(ESSWSessionState::Failed,TEXT("Session service is unavailable."));return false;}
 if(!bLAN && OSS->GetSubsystemName()==FName(TEXT("NULL"))){Change(ESSWSessionState::Failed,TEXT("Internet play requires a configured online service. LAN is available."));return false;}
 auto Identity=OSS->GetIdentityInterface();
 if(!Identity.IsValid()){Change(ESSWSessionState::Failed,TEXT("Player identity service is unavailable."));return false;}
 if(!Identity->GetUniquePlayerId(0).IsValid()){
  if(OSS->GetSubsystemName()==FName(TEXT("NULL")))Identity->AutoLogin(0);
  if(!Identity->GetUniquePlayerId(0).IsValid()){Change(ESSWSessionState::Failed,TEXT("Sign in to the online service before creating or joining a game."));return false;}
 }
 Sessions=OSS->GetSessionInterface();return true;
}
bool USSWSessionSubsystem::CreateSession(const FString& Name,int32 Players,bool bLAN){
 if(!Acquire(bLAN))return false;
 if(Name.TrimStartAndEnd().IsEmpty()){Change(ESSWSessionState::Failed,TEXT("Enter a session name."));return false;}
 if(!FPackageName::IsValidLongPackageName(LobbyMap)||!FPackageName::DoesPackageExist(LobbyMap)){Change(ESSWSessionState::Failed,TEXT("The lobby map is unavailable."));return false;}
 if(Sessions->GetNamedSession(NAME_GameSession)){Change(ESSWSessionState::Failed,TEXT("Leave the current session before creating another."));return false;}
 CreateHandle=Sessions->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateUObject(this,&ThisClass::Created));
 Change(ESSWSessionState::Creating,TEXT("Creating session..."));
 if(!Sessions->CreateSession(0,NAME_GameSession,MakeSettings(Name,Players,bLAN))){Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);CreateHandle.Reset();Change(ESSWSessionState::Failed,TEXT("Session creation could not start."));return false;}return true;
}
void USSWSessionSubsystem::Created(FName Name,bool bOK){
 Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);CreateHandle.Reset();
 if(!bOK){Change(ESSWSessionState::Failed,TEXT("Could not create the session."));return;}
 Sessions->StartSession(Name);Change(ESSWSessionState::Hosting,TEXT("Hosting session."));
 UGameplayStatics::OpenLevel(this,FName(*LobbyMap),true,TEXT("listen?game=/Script/StarshatterWars.SSWSessionLobbyGameMode"));
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
  FSSWSessionEntry Row;Match.Session.SessionSettings.Get(SSWTitleKey,Row.Name);if(Row.Name.IsEmpty())Row.Name=Match.Session.OwningUserName;
  Row.Capacity=Match.Session.SessionSettings.NumPublicConnections;Row.Players=FMath::Max(0,Row.Capacity-Match.Session.NumOpenPublicConnections);Row.Ping=Match.PingInMs;
  Results.Add(Row);SearchResults.Add(Match);
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
 if(Result!=EOnJoinSessionCompleteResult::Success||!Sessions->GetResolvedConnectString(Name,Address)){Change(ESSWSessionState::Failed,TEXT("Could not join. Refresh the session list and try again."));return;}
 auto* PC=GetWorld()?GetWorld()->GetFirstPlayerController():nullptr;
 if(!PC){Change(ESSWSessionState::Failed,TEXT("No local player is available to join."));return;}
 Change(ESSWSessionState::Joined,TEXT("Connecting to session..."));PC->ClientTravel(Address,TRAVEL_Absolute);
}
bool USSWSessionSubsystem::LeaveSession(){
 if(IsBusy())return false;
 if(!Sessions.IsValid()){IOnlineSubsystem* OSS=GetWorld()?Online::GetSubsystem(GetWorld()):nullptr;if(OSS)Sessions=OSS->GetSessionInterface();}
 if(!Sessions.IsValid()||!Sessions->GetNamedSession(NAME_GameSession)){Change(ESSWSessionState::Idle,TEXT("No active session."));return false;}
 DestroyHandle=Sessions->AddOnDestroySessionCompleteDelegate_Handle(FOnDestroySessionCompleteDelegate::CreateUObject(this,&ThisClass::Left));Change(ESSWSessionState::Leaving,TEXT("Leaving session..."));
 if(!Sessions->DestroySession(NAME_GameSession)){Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);DestroyHandle.Reset();Change(ESSWSessionState::Failed,TEXT("Could not leave session."));return false;}return true;
}
void USSWSessionSubsystem::Left(FName Name,bool bOK){Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);DestroyHandle.Reset();Change(bOK?ESSWSessionState::Idle:ESSWSessionState::Failed,bOK?TEXT("Session closed."):TEXT("Could not close session."));if(bOK){bReturningToMenu=true;UGameplayStatics::OpenLevel(this,FName(*MenuMap));}}
void USSWSessionSubsystem::MapLoaded(UWorld* World){if(!bReturningToMenu||World!=GetWorld())return;bReturningToMenu=false;if(auto* GI=Cast<USSWGameInstance>(GetGameInstance()))World->GetTimerManager().SetTimerForNextTick(GI,&USSWGameInstance::ShowMainMenuScreen);}
void USSWSessionSubsystem::CancelSearch(){if(State==ESSWSessionState::Searching&&Sessions.IsValid()){bSearchCancelled=true;Sessions->CancelFindSessions();}}
void USSWSessionSubsystem::ClearDelegates(){if(Sessions.IsValid()){
 Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);}
 CreateHandle.Reset();FindHandle.Reset();JoinHandle.Reset();DestroyHandle.Reset();}
void USSWSessionSubsystem::Deinitialize(){FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(MapLoadedHandle);if(GEngine){GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);GEngine->OnTravelFailure().Remove(TravelFailureHandle);}ClearDelegates();Search.Reset();SearchResults.Reset();Results.Reset();Sessions.Reset();Super::Deinitialize();}

void USSWSessionSubsystem::AdvertiseDedicatedLAN()
{
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() != NM_DedicatedServer) return;

    IOnlineSubsystem* OSS = Online::GetSubsystem(World, FName(TEXT("NULL")));
    Sessions = OSS ? OSS->GetSessionInterface() : nullptr;
    if (!Sessions.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[StarServer] LAN advertisement unavailable; direct connections remain available"));
        return;
    }

    FString Name = TEXT("StarServer");
    FParse::Value(FCommandLine::Get(), TEXT("ServerName="), Name);
    FOnlineSessionSettings Settings = MakeSettings(Name, 4, true);
    Settings.bIsDedicated = true;
    Settings.bAllowJoinInProgress = true;

    CreateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
        FOnCreateSessionCompleteDelegate::CreateWeakLambda(this, [this](FName SessionName, bool bSuccess)
        {
            Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
            CreateHandle.Reset();
            if (bSuccess)
            {
                Sessions->StartSession(SessionName);
                Change(ESSWSessionState::Hosting, TEXT("StarServer is hosting on LAN."));
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[StarServer] LAN session creation failed"));
            }
        }));
    if (!Sessions->CreateSession(0, NAME_GameSession, Settings))
    {
        Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
        CreateHandle.Reset();
        UE_LOG(LogTemp, Warning, TEXT("[StarServer] Could not advertise the LAN session"));
    }
}
