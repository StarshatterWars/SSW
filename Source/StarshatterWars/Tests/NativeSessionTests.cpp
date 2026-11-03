#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "SSWSessionSubsystem.h"
#include "MultiplayerDlg.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "HAL/PlatformTime.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNativeSessionSettingsTest,"Starshatter.Sessions.SettingsAndValidation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNativeSessionSettingsTest::RunTest(const FString&){
 auto S=USSWSessionSubsystem::MakeSettings(TEXT(" Test game "),99,true);TestEqual(TEXT("Player cap bounded"),S.NumPublicConnections,16);TestTrue(TEXT("LAN advertises"),S.bIsLANMatch&&S.bShouldAdvertise);TestTrue(TEXT("Join in progress allowed"),S.bAllowJoinInProgress);FString Name,Project;S.Get(FName(TEXT("SSW_TITLE")),Name);S.Get(FName(TEXT("SSW_PROJECT")),Project);TestEqual(TEXT("Trimmed title"),Name,FString(TEXT("Test game")));TestEqual(TEXT("Cross-game search tag"),Project,FString(TEXT("StarshatterWars")));
 auto* Service=NewObject<USSWSessionSubsystem>(NewObject<UGameInstance>());TestFalse(TEXT("No session by default"),Service->HasSession());TestFalse(TEXT("Invalid join does not travel"),Service->JoinSession(-1));TestEqual(TEXT("Error is surfaced"),Service->State,ESSWSessionState::Failed);TestFalse(TEXT("No stale busy state"),Service->IsBusy());return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNativeSessionUITest,"Starshatter.Sessions.NativeDialog",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNativeSessionUITest::RunTest(const FString&){
 auto* World=UWorld::CreateWorld(EWorldType::Game,false);World->SetGameInstance(NewObject<UGameInstance>());ON_SCOPE_EXIT{World->DestroyWorld(false);};auto* Screen=NewObject<UMultiplayerDlg>(World);Screen->Initialize();auto Widget=Screen->TakeWidget();TestEqual(TEXT("Native session root"),Screen->WidgetTree->RootWidget->GetFName(),FName(TEXT("SSWNativeSessionScreen")));TArray<UWidget*> Widgets;Screen->WidgetTree->GetAllWidgets(Widgets);TSet<FString> Labels;for(auto* W:Widgets)if(auto* T=Cast<UTextBlock>(W))Labels.Add(T->GetText().ToString());TestTrue(TEXT("Host tab"),Labels.Contains(TEXT("CREATE SESSION")));TestTrue(TEXT("Join tab"),Labels.Contains(TEXT("JOIN SESSION")));TestTrue(TEXT("Host action"),Labels.Contains(TEXT("CREATE GAME")));TestTrue(TEXT("Search action"),Labels.Contains(TEXT("FIND GAMES / REFRESH")));TestTrue(TEXT("Join action"),Labels.Contains(TEXT("JOIN SELECTED GAME")));TestFalse(TEXT("No legacy mission/briefing selection"),Screen->HasSelectedMission());return true;
}
namespace {
struct FSessionLANState {
 IOnlineSessionPtr Host,Client;FName HostID,ClientID;UWorld* HostWorld=nullptr;UWorld* ClientWorld=nullptr;FDelegateHandle Create,Find,Join;
 TSharedPtr<FOnlineSessionSearch> Search=MakeShared<FOnlineSessionSearch>();
 bool bCreated=false,bFound=false,bJoined=false,bCreateOK=false,bFindOK=false;EOnJoinSessionCompleteResult::Type JoinResult=EOnJoinSessionCompleteResult::UnknownError;
 int Stage=0;double Start=FPlatformTime::Seconds();FString Title;
 void Cleanup(){if(Host.IsValid()){Host->ClearOnCreateSessionCompleteDelegate_Handle(Create);Host->DestroySession(NAME_GameSession);}if(Client.IsValid()){Client->ClearOnFindSessionsCompleteDelegate_Handle(Find);Client->ClearOnJoinSessionCompleteDelegate_Handle(Join);Client->CancelFindSessions();Client->DestroySession(NAME_GameSession);}Host.Reset();Client.Reset();IOnlineSubsystem::Destroy(HostID);IOnlineSubsystem::Destroy(ClientID);
 if(HostWorld){GEngine->DestroyWorldContext(HostWorld);HostWorld->DestroyWorld(false);HostWorld=nullptr;}
 if(ClientWorld){GEngine->DestroyWorldContext(ClientWorld);ClientWorld->DestroyWorld(false);ClientWorld=nullptr;}}
};
class FNativeLANCommand:public IAutomationLatentCommand {
 FAutomationTestBase* Test;TSharedPtr<FSessionLANState> S=MakeShared<FSessionLANState>();
public:explicit FNativeLANCommand(FAutomationTestBase* T):Test(T){}
 virtual bool Update()override{
 if(FPlatformTime::Seconds()-S->Start>25){Test->AddError(TEXT("LAN create/search/join timed out"));S->Cleanup();return true;}
 if(S->Stage==0){const FString ID=FGuid::NewGuid().ToString(EGuidFormats::Digits);S->HostWorld=UWorld::CreateWorld(EWorldType::Game,false);S->ClientWorld=UWorld::CreateWorld(EWorldType::Game,false);
 auto& HostContext=GEngine->CreateNewWorldContext(EWorldType::Game);HostContext.SetCurrentWorld(S->HostWorld);S->HostID=FName(*(TEXT("NULL:")+HostContext.ContextHandle.ToString()));
 auto& ClientContext=GEngine->CreateNewWorldContext(EWorldType::Game);ClientContext.SetCurrentWorld(S->ClientWorld);S->ClientID=FName(*(TEXT("NULL:")+ClientContext.ContextHandle.ToString()));S->Title=TEXT("SSW LAN smoke ")+ID;auto* H=IOnlineSubsystem::Get(S->HostID);auto* C=IOnlineSubsystem::Get(S->ClientID);
 if(!H||!C){Test->AddError(TEXT("OnlineSubsystemNull instances unavailable"));S->Cleanup();return true;}H->GetIdentityInterface()->AutoLogin(0);C->GetIdentityInterface()->AutoLogin(0);S->Host=H->GetSessionInterface();S->Client=C->GetSessionInterface();if(!S->Host.IsValid()||!S->Client.IsValid()){Test->AddError(TEXT("Session interfaces unavailable"));S->Cleanup();return true;}
 S->Create=S->Host->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateLambda([State=S](FName,bool OK){State->bCreated=true;State->bCreateOK=OK;}));S->Stage=1;
 if(!S->Host->CreateSession(0,NAME_GameSession,USSWSessionSubsystem::MakeSettings(S->Title,4,true))){Test->AddError(TEXT("LAN creation rejected"));S->Cleanup();return true;}
 }
 if(S->Stage==1&&S->bCreated){if(!S->bCreateOK){Test->AddError(TEXT("LAN creation failed"));S->Cleanup();return true;}S->Host->ClearOnCreateSessionCompleteDelegate_Handle(S->Create);S->Host->StartSession(NAME_GameSession);S->Search->bIsLanQuery=true;S->Search->MaxSearchResults=50;S->Search->TimeoutInSeconds=10;S->Find=S->Client->AddOnFindSessionsCompleteDelegate_Handle(FOnFindSessionsCompleteDelegate::CreateLambda([State=S](bool OK){State->bFound=true;State->bFindOK=OK;}));S->Stage=2;if(!S->Client->FindSessions(0,S->Search.ToSharedRef())){Test->AddError(TEXT("LAN discovery rejected"));S->Cleanup();return true;}}
 if(S->Stage==2&&S->bFound){S->Client->ClearOnFindSessionsCompleteDelegate_Handle(S->Find);int Index=INDEX_NONE;for(int I=0;I<S->Search->SearchResults.Num();++I){FString Name;S->Search->SearchResults[I].Session.SessionSettings.Get(FName(TEXT("SSW_TITLE")),Name);if(Name==S->Title){Index=I;break;}}
 if(!S->bFindOK||Index==INDEX_NONE){Test->AddError(TEXT("LAN discovery did not return the advertised SSW session"));S->Cleanup();return true;}S->Join=S->Client->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateLambda([State=S](FName,EOnJoinSessionCompleteResult::Type Result){State->bJoined=true;State->JoinResult=Result;}));S->Stage=3;if(!S->Client->JoinSession(0,NAME_GameSession,S->Search->SearchResults[Index])){Test->AddError(TEXT("LAN join rejected"));S->Cleanup();return true;}}
 if(S->Stage==3&&S->bJoined){Test->TestEqual(TEXT("Native LAN join completes"),S->JoinResult,EOnJoinSessionCompleteResult::Success);FString URL;Test->TestTrue(TEXT("Join resolves a connect address"),S->Client->GetResolvedConnectString(NAME_GameSession,URL)&&!URL.IsEmpty());Test->AddInfo(TEXT("LAN advertisement, discovery, join and connect-string resolution verified; this test does not launch combat replication."));S->Cleanup();return true;}return false;
 }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNativeLANSessionTest,"Starshatter.Sessions.LANCreateFindJoin",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNativeLANSessionTest::RunTest(const FString&){ADD_LATENT_AUTOMATION_COMMAND(FNativeLANCommand(this));return true;}
#endif
