#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "Engine/EngineBaseTypes.h"
#include "HAL/PlatformProcess.h"
#include "TimerManager.h"
#include "SSWSessionSubsystem.generated.h"
UENUM(BlueprintType)
enum class ESSWSessionState:uint8 {Idle,Creating,Searching,Joining,Hosting,Joined,Leaving,Failed};
USTRUCT(BlueprintType)
struct FSSWSessionEntry {
 GENERATED_BODY()
 UPROPERTY(BlueprintReadOnly) FString Name;
 UPROPERTY(BlueprintReadOnly) FString Mission;
 UPROPERTY(BlueprintReadOnly) int32 Players=0;
 UPROPERTY(BlueprintReadOnly) int32 Capacity=0;
 UPROPERTY(BlueprintReadOnly) int32 Ping=0;
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSSWSessionChanged);
// Native asynchronous session flow, independent from legacy mission-roster selection.
UCLASS(Config=Game)
class STARSHATTERWARS_API USSWSessionSubsystem:public UGameInstanceSubsystem {
 GENERATED_BODY()
public:
 virtual void Initialize(FSubsystemCollectionBase& Collection) override;
 virtual void Deinitialize() override;
 UPROPERTY(BlueprintAssignable) FSSWSessionChanged OnChanged;
 UPROPERTY(BlueprintReadOnly) ESSWSessionState State=ESSWSessionState::Idle;
 UPROPERTY(BlueprintReadOnly) FString Status=TEXT("Create a session or search for games.");
 UPROPERTY(BlueprintReadOnly) TArray<FSSWSessionEntry> Results;
 UPROPERTY(Config,EditAnywhere,Category="Sessions") FString LobbyMap=TEXT("/Game/Maps/Transition");
 UPROPERTY(Config,EditAnywhere,Category="Sessions") FString MenuMap=TEXT("/Game/Maps/Starshatter");
 UFUNCTION(BlueprintPure) bool IsBusy() const;
 UFUNCTION(BlueprintPure) bool HasSession() const;
 UFUNCTION(BlueprintCallable) bool CreateSession(const FString& Name,int32 Players,bool bLAN);
 UFUNCTION(BlueprintCallable) bool FindSessions(bool bLAN);
 UFUNCTION(BlueprintCallable) bool JoinSession(int32 Index);
 UFUNCTION(BlueprintCallable) bool LeaveSession();
 UFUNCTION(BlueprintCallable) void CancelSearch();
 void AdvertiseDedicated(bool bLAN, int32 Players, const FString& Mission);
 void CloseDedicatedSessionAndExit();
 bool CreateMissionSession(const FString& Name, int32 Players, bool bLAN, int32 MissionIndex);
 static FOnlineSessionSettings MakeSettings(const FString& Name,int32 Players,bool bLAN);
private:
 void NetworkFailure(UWorld* World,class UNetDriver* Driver,ENetworkFailure::Type Type,const FString& Message);
 void TravelFailure(UWorld* World,ETravelFailure::Type Type,const FString& Message);
 FDelegateHandle NetworkFailureHandle,TravelFailureHandle,MapLoadedHandle;
 bool bReturningToMenu=false;
 void MapLoaded(UWorld* World);
 bool Acquire(bool bLAN);
 void Change(ESSWSessionState NewState,const FString& Message);
 void ClearDelegates();
 void Created(FName Name,bool bOK);
 void Found(bool bOK);
 void Joined(FName Name,EOnJoinSessionCompleteResult::Type Result);
 void Left(FName Name,bool bOK);
 void DedicatedSessionClosed(FName Name, bool bOK);
 void ExitDedicatedServer();
 FTimerHandle DedicatedExitTimeout;
 IOnlineSessionPtr Sessions;
 TSharedPtr<FOnlineSessionSearch> Search;
 TArray<FOnlineSessionSearchResult> SearchResults;
 FDelegateHandle CreateHandle,FindHandle,JoinHandle,DestroyHandle;
 bool bSearchCancelled=false;
 bool bUsingEOS=false;
 int32 HostingMissionIndex = INDEX_NONE;
 int32 HostingCapacity = 4;
 bool StartStarServer(const FString& Name, int32 Players, int32 MissionIndex);
 void PollStarServer();
 void ReleaseStarServer(bool bTerminate);
 FProcHandle StarServerProcess;
 FTimerHandle StarServerPoll;
 FString StarServerReadyFile;
 double StarServerDeadline = 0;
};
