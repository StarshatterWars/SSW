#pragma once
#include "Subsystems/GameInstanceSubsystem.h"
#include "EconomyServiceEndpoint.h"
#include "Interfaces/IHttpRequest.h"
#include "Containers/Ticker.h"
#include "EconomyServiceSubsystem.generated.h"
class FJsonObject;
struct FNativeMarketCommand;
struct FNpcTradeReceipt;
struct FEconomyServiceStatus
{
    bool bConnected=false,bCheckpointReady=false,bNpcAuthorityActive=false;
    FString WorldId,Error;
    int64 Revision=0;
    double EconomySeconds=0,UptimeSeconds=0,PingMs=-1;
    int32 Inventories=0,Jobs=0,Freight=0;
    FDateTime LastUpdateUtc;
};
UCLASS()
class STARSHATTERWARS_API UEconomyServiceSubsystem:public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    bool ShouldCreateSubsystem(UObject* Outer)const override;
    void Initialize(FSubsystemCollectionBase&)override;
    void Deinitialize()override;
    const FEconomyServiceEndpoint& Endpoint()const{return Address;}
    const FEconomyServiceStatus& Status()const{return Cached;}
    bool RequiresExternal()const{return bExternalRequired;}
    bool IsReady()const{return Cached.bConnected&&Cached.bCheckpointReady&&Cached.bNpcAuthorityActive&&bProjectionReady;}
    FString Summary()const;
    using FReply=TFunction<void(bool,TSharedPtr<FJsonObject>,const FString&)>;
    bool Command(TSharedRef<FJsonObject>,FReply,FString& Reason);
    bool Trade(const FNativeMarketCommand&,TFunction<bool(FText&)>&&,TFunction<void(bool,const FText&,FGuid)>&&,FText&);
    FSimpleMulticastDelegate OnStatusChanged;
private:
    FEconomyServiceEndpoint Address;
    FEconomyServiceStatus Cached;
    bool bStopping=false,bPolling=false,bExternalRequired=false,bTradePending=false,bSyncPending=false,bProjectionReady=false;
    int64 ProjectedRevision=-1;
    FString ProjectionProfile,SiteSignature;
    TMap<FGuid,FString> JourneyIds;
    TSet<FName> ProjectedOffline;
    void SynchronizeNpc();
    void ObserveTraffic();
    FTSTicker::FDelegateHandle PollHandle;
    TArray<FHttpRequestPtr> Requests;
    void Poll();
    void Disconnected(const FString&);
    void RecoverPendingTrades();
    void SettleTrade(FNpcTradeReceipt,const FString& Profile,TFunction<void(bool,const FText&,FGuid)>&&);
    void ApplyTradeReply(const FNpcTradeReceipt&,const FString& Profile,bool,TSharedPtr<FJsonObject>,const FString&,TFunction<void(bool,const FText&,FGuid)>&&);
};
