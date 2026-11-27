#include "EconomyServiceSubsystem.h"
#include "EconomyAuthority.h"
#include "FractalVerseSubsystem.h"
#include "HttpModule.h"
#include "Containers/Ticker.h"
#include "Interfaces/IHttpResponse.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "HAL/PlatformProcess.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
namespace
{
constexpr int32 MaxReplyBytes=16*1024*1024;
TSharedPtr<FJsonObject> Decode(FHttpResponsePtr Response)
{
    TSharedPtr<FJsonObject> Root;
    if(!Response||Response->GetContent().Num()>MaxReplyBytes||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Response->GetContentAsString()),Root))return nullptr;
    return Root;
}
bool Count(const TSharedPtr<FJsonObject>& Root,const TCHAR* Name,int32& Out)
{double V=0;if(!Root->TryGetNumberField(Name,V)||!FMath::IsFinite(V)||V<0||V>1000000000||V!=FMath::FloorToDouble(V))return false;Out=int32(V);return true;}
}
bool UEconomyServiceSubsystem::ShouldCreateSubsystem(UObject* Outer)const
{
    return Super::ShouldCreateSubsystem(Outer)&&(!IsRunningCommandlet()||FParse::Param(FCommandLine::Get(),TEXT("SSWIntegrationEconomy")))&&UFractalVerseSubsystem::ProcessRole()!=EFractalVerseRole::Client&&
        (!FApp::IsUnattended()||FParse::Param(FCommandLine::Get(),TEXT("SSWIntegrationEconomy")));
}
void UEconomyServiceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);Address=FEconomyServiceEndpoint::Load();
    bExternalRequired=SSWEconomyAuthority::RequiresService();
    if(!Address.bValid){Disconnected(Address.Error);return;}
    PollHandle=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this,[this](float){Poll();return !bStopping;}),2.0f);
    Poll();
}
void UEconomyServiceSubsystem::Deinitialize()
{
    bStopping=true;FTSTicker::GetCoreTicker().RemoveTicker(PollHandle);PollHandle.Reset();
    for(auto& Request:Requests)if(Request){Request->OnProcessRequestComplete().Unbind();Request->CancelRequest();}
    Requests.Reset();Super::Deinitialize();
}
void UEconomyServiceSubsystem::Disconnected(const FString& Reason)
{
    Cached.bConnected=false;Cached.bNpcAuthorityActive=false;Cached.PingMs=-1;Cached.Error=Reason;OnStatusChanged.Broadcast();
}
void UEconomyServiceSubsystem::Poll()
{
    if(bStopping||bPolling||!Address.bValid)return;bPolling=true;
    const double Began=FPlatformTime::Seconds();const TWeakObjectPtr<UEconomyServiceSubsystem> Weak(this);
    auto Request=FHttpModule::Get().CreateRequest();Request->SetURL(Address.Url+TEXT("/status"));Request->SetVerb(TEXT("GET"));Request->SetTimeout(3);Request->SetHeader(TEXT("Cache-Control"),TEXT("no-store"));
    Request->SetDelegateThreadPolicy(EHttpRequestDelegateThreadPolicy::CompleteOnGameThread);Requests.Add(Request);
    Request->OnProcessRequestComplete().BindLambda([Weak,Began](FHttpRequestPtr Finished,FHttpResponsePtr Response,bool Success){
        auto* Self=Weak.Get();if(!Self||Self->bStopping)return;Self->Requests.Remove(Finished);Self->bPolling=false;
        const auto Root=Decode(Response);FString Server;double Protocol=0,Uptime=0,Clock=0,Revision=0,NowUtc=0;bool Online=false,Checkpoint=false;
        if(!Success||!Response||Response->GetResponseCode()!=200||!Root||!Root->TryGetStringField(TEXT("server"),Server)||Server!=TEXT("FractalEconomy")||
           !Root->TryGetNumberField(TEXT("protocol"),Protocol)||Protocol!=1||!Root->TryGetBoolField(TEXT("online"),Online)||!Online||
           !Root->TryGetNumberField(TEXT("serverUtc"),NowUtc)||!FMath::IsFinite(NowUtc)||FMath::Abs(NowUtc-double(FDateTime::UtcNow().ToUnixTimestamp()))>10||
           !Root->TryGetNumberField(TEXT("revision"),Revision)||!FMath::IsFinite(Revision)||Revision<0||Revision>9007199254740991.0||
           !Root->TryGetNumberField(TEXT("uptimeSeconds"),Uptime)||!FMath::IsFinite(Uptime)||Uptime<0||
           !Root->TryGetNumberField(TEXT("economySeconds"),Clock)||!FMath::IsFinite(Clock)||Clock<0||!Root->TryGetBoolField(TEXT("checkpointReady"),Checkpoint))
        {Self->Disconnected(TEXT("No verified economy heartbeat. Pending trades remain in the player outbox."));return;}
        FEconomyServiceStatus Next;Next.bConnected=true;Next.bCheckpointReady=Checkpoint;Next.Revision=int64(Revision);Next.EconomySeconds=Clock;Next.UptimeSeconds=Uptime;
        Next.PingMs=(FPlatformTime::Seconds()-Began)*1000;Next.LastUpdateUtc=FDateTime::UtcNow();
        if(!Count(Root,TEXT("inventories"),Next.Inventories)||!Count(Root,TEXT("activeJobs"),Next.Jobs)||!Count(Root,TEXT("deliveredFreight"),Next.Freight))
        {Self->Disconnected(TEXT("The economy heartbeat contains invalid counters."));return;}
        Root->TryGetStringField(TEXT("worldId"),Next.WorldId);Root->TryGetBoolField(TEXT("npcAuthorityActive"),Next.bNpcAuthorityActive);
        Next.bNpcAuthorityActive&=!Next.WorldId.IsEmpty();
        if(!Checkpoint)Next.Error=TEXT("Economy persistence is unavailable. Mutating requests are suspended.");
        Self->Cached=MoveTemp(Next);Self->OnStatusChanged.Broadcast();Self->RecoverPendingTrades();
    });
    if(!Request->ProcessRequest()){Requests.Remove(Request);bPolling=false;Disconnected(TEXT("The economy heartbeat could not start."));}
}
FString UEconomyServiceSubsystem::Summary()const
{
    if(!Address.bValid)return TEXT("CONFIGURATION ERROR\n")+Address.Error;
    FString Label=!Cached.bConnected?TEXT("DISCONNECTED"):!Cached.bCheckpointReady?TEXT("CHECKPOINT BLOCKED"):Cached.bNpcAuthorityActive?TEXT("ONLINE"):TEXT("CONNECTED - AWAITING NPC HANDOFF");
    if(!bExternalRequired&&Cached.bConnected)Label=TEXT("AVAILABLE - GAME STILL USES LOCAL ECONOMY");
    const FString Ping=Cached.PingMs>=0?FString::Printf(TEXT("%.0f MS"),Cached.PingMs):TEXT("--");
    return FString::Printf(TEXT("%s\n%s\nPING %s  |  REVISION %lld  |  JOBS %d  |  DELIVERIES %d\nLAST UPDATE %s\n%s"),*Label,*Address.Url,*Ping,Cached.Revision,Cached.Jobs,Cached.Freight,
        Cached.LastUpdateUtc.GetTicks()>0?*Cached.LastUpdateUtc.ToString(TEXT("%H:%M:%S GMT")):TEXT("--"),*Cached.Error);
}
bool UEconomyServiceSubsystem::Command(TSharedRef<FJsonObject> Body,FReply Completed,FString& Reason)
{
    if(bStopping||!Address.bValid||!Cached.bConnected||!Cached.bCheckpointReady){Reason=TEXT("The economy service is unavailable. No local fallback or exchange was performed.");return false;}
    if(!Address.bLoopback){Reason=TEXT("Remote economy authorization has not been configured.");return false;}
    FString Action;Body->TryGetStringField(TEXT("action"),Action);
    if(!bExternalRequired||(Action!=TEXT("bootstrap_npc")&&!IsReady())){Reason=TEXT("The live NPC economy handoff is not active. Nothing exchanged.");return false;}
    FString Key;const auto KeyPath=FPaths::Combine(FPlatformProcess::UserSettingsDir(),TEXT("Fractal Dev Games/FractalEconomy/LocalEconomy/authority.key"));
    if(!FFileHelper::LoadFileToString(Key,*KeyPath)){Reason=TEXT("Local economy authorization is unavailable.");return false;}Key.TrimStartAndEndInline();
    if(Key.Len()!=64){Reason=TEXT("Invalid local economy authorization.");return false;}for(TCHAR C:Key)if(!FChar::IsHexDigit(C)){Reason=TEXT("Invalid local economy authorization.");return false;}
    FString Payload;if(!FJsonSerializer::Serialize(Body,TJsonWriterFactory<>::Create(&Payload))){Reason=TEXT("The economy request could not be encoded.");return false;}
    const TWeakObjectPtr<UEconomyServiceSubsystem> Weak(this);auto Request=FHttpModule::Get().CreateRequest();Request->SetURL(Address.Url+TEXT("/v1/commands"));Request->SetVerb(TEXT("POST"));Request->SetTimeout(5);
    Request->SetHeader(TEXT("Authorization"),TEXT("Bearer ")+Key);Request->SetHeader(TEXT("Content-Type"),TEXT("application/json"));Request->SetContentAsString(Payload);Request->SetDelegateThreadPolicy(EHttpRequestDelegateThreadPolicy::CompleteOnGameThread);Requests.Add(Request);
    Request->OnProcessRequestComplete().BindLambda([Weak,Reply=MoveTemp(Completed)](FHttpRequestPtr Finished,FHttpResponsePtr Response,bool Success)mutable{
        auto* Self=Weak.Get();if(!Self||Self->bStopping)return;Self->Requests.Remove(Finished);auto Root=Decode(Response);bool Accepted=false;
        const bool Good=Success&&Response&&Response->GetResponseCode()==200&&Root&&Root->TryGetBoolField(TEXT("ok"),Accepted)&&Accepted;
        FString Error;if(!Good){if(Root)Root->TryGetStringField(TEXT("error"),Error);if(Error.IsEmpty())Error=TEXT("Economy response unavailable; reconcile the receipt before retrying an exchange.");}
        if(!Success)Self->Disconnected(Error);if(Reply)Reply(Good,MoveTemp(Root),Error);
    });
    if(!Request->ProcessRequest()){Requests.Remove(Request);Reason=TEXT("The economy request could not start.");return false;}return true;
}
