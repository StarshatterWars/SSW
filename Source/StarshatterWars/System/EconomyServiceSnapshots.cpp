#include "EconomyServiceSubsystem.h"
#include "EconomyAuthority.h"
#include "EconomySnapshot.h"
#include "EconomyOwnership.h"
#include "EconomyRuntime.h"
#include "GalaxyTrafficSubsystem.h"
#include "StarshatterPlayerSubsystem.h"
#include "CargoCatalogSubsystem.h"
#include "SimDataEvents.h"
#include "Engine/GameInstance.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
namespace
{
FString SnapshotProfileKey(UStarshatterPlayerSubsystem* P){return P->GetSlotName()+TEXT("/")+P->GetActiveSavedPlayerSlot()+FString::Printf(TEXT("/%d/%lld"),P->GetUserIndex(),P->GetPlayerInfo().CreateTime);}
TSharedPtr<FJsonObject> Read(const FString& Path){FString Text;TSharedPtr<FJsonObject> O;if(FFileHelper::LoadFileToString(Text,*Path))FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),O);return O;}
bool Durable(const FString& File,const TSharedRef<FJsonObject>& Object)
{
    FString Text;if(!FJsonSerializer::Serialize(Object,TJsonWriterFactory<>::Create(&Text)))return false;
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(File),true);const auto Temp=File+TEXT(".pending");FTCHARToUTF8 Bytes(*Text);
    TUniquePtr<IFileHandle> Handle(FPlatformFileManager::Get().GetPlatformFile().OpenWrite(*Temp));
    if(!Handle||!Handle->Write(reinterpret_cast<const uint8*>(Bytes.Get()),Bytes.Length())||!Handle->Flush(true))return false;Handle.Reset();
    return IFileManager::Get().Move(*File,*Temp,true,true)==COPY_OK;
}
}
void UEconomyServiceSubsystem::SynchronizeNpc()
{
    if(!bExternalRequired||bSyncPending||bStopping||!Cached.bConnected||!Cached.bCheckpointReady)return;
    auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();auto* Traffic=GetGameInstance()->GetSubsystem<UGalaxyTrafficSubsystem>();
    if(!Players||!Players->HasLoaded()||!Traffic)return;
    const auto Bound=SnapshotProfileKey(Players);if(ProjectionProfile!=Bound){bProjectionReady=false;ProjectedRevision=-1;ProjectedOffline.Reset();}if(Cached.Revision<ProjectedRevision){bProjectionReady=false;ProjectedRevision=-1;}
    auto& State=Players->GetEconomy();const auto Root=SSWEconomyAuthority::StateDirectory();const auto LinkPath=Root/TEXT("client-world.json"),PendingPath=Root/TEXT("handoff-request.json");
    const auto Link=Read(LinkPath);if(!Link&&IFileManager::Get().FileExists(*LinkPath)){bProjectionReady=false;Cached.Error=TEXT("The saved economy identity is unreadable. No replacement world was accepted.");return;}FString Pinned;if(Link)Link->TryGetStringField(TEXT("worldId"),Pinned);
    if((!Pinned.IsEmpty()&&Pinned!=Cached.WorldId)||(!State.NpcAuthorityWorldId.IsEmpty()&&State.NpcAuthorityWorldId!=Cached.WorldId))
    {bProjectionReady=false;Cached.Error=TEXT("The economy world identity changed. Existing pilot assets and NPC state were retained.");return;}
    const TWeakObjectPtr<UEconomyServiceSubsystem> Weak(this);
    if(!Cached.bNpcAuthorityActive)
    {
        if(!Pinned.IsEmpty()||!State.NpcAuthorityWorldId.IsEmpty()){Cached.Error=TEXT("The pinned economy is not active. No replacement world was initialized.");return;}
        auto Request=Read(PendingPath);FString Reason;
        if(!Request)
        {
            if(IFileManager::Get().FileExists(*PendingPath)){Cached.Error=TEXT("The pending NPC handoff is unreadable; it was not overwritten.");return;}
            auto* Cargo=GetGameInstance()->GetSubsystem<UCargoCatalogSubsystem>();if(!Cargo||!Cargo->HasDefinitions())return;
            if(!Traffic->QuiesceForExternal(Reason)){Cached.Error=Reason;return;}
            FEconomyRuntime::InitializeMarkets(State,*Cargo);
            auto Snapshot=SSWEconomySnapshot::Capture(State,Players->GetPlayerInfo().Traffic,Reason);
            if(!Snapshot){Cached.Error=Reason;return;}
            Request=MakeShared<FJsonObject>();Request->SetStringField(TEXT("action"),TEXT("bootstrap_npc"));Request->SetStringField(TEXT("format"),TEXT("SSW-UE-NPC-1"));
            Request->SetStringField(TEXT("handoffId"),FGuid::NewGuid().ToString(EGuidFormats::Digits));Request->SetStringField(TEXT("sourceId"),FGuid::NewGuid().ToString(EGuidFormats::Digits));Request->SetObjectField(TEXT("snapshot"),Snapshot);
            if(!Players->SaveActivePlayerProgress()||!Durable(PendingPath,Request.ToSharedRef())){Cached.Error=TEXT("The source NPC snapshot could not be saved. No server activation was requested.");return;}
        }
        if(!SSWEconomyAuthority::BeginHandoff(Reason)){Cached.Error=Reason;return;}
        bSyncPending=true;
        if(!Command(Request.ToSharedRef(),[Weak](bool Ok,TSharedPtr<FJsonObject>,const FString& Error){if(auto* Self=Weak.Get()){Self->bSyncPending=false;if(!Ok)Self->Cached.Error=Error;Self->Poll();}},Reason)){bSyncPending=false;Cached.Error=Reason;}
        return;
    }
    if(bProjectionReady&&ProjectionProfile==Bound&&SiteSignature!=SSWEconomySnapshot::SitesSignature(State))
    {
        FString Why;auto Sites=SSWEconomySnapshot::Sites(State,Why);if(!Sites){Cached.Error=Why;return;}
        const FString Signature=SSWEconomySnapshot::SitesSignature(State);Sites->SetStringField(TEXT("action"),TEXT("register_sites"));Sites->SetStringField(TEXT("id"),TEXT("sites/")+Cached.WorldId+TEXT("/")+Signature);Sites->SetStringField(TEXT("worldId"),Cached.WorldId);bSyncPending=true;
        if(!Command(Sites.ToSharedRef(),[Weak,Signature](bool Ok,TSharedPtr<FJsonObject>,const FString& Error){if(auto* Self=Weak.Get()){Self->bSyncPending=false;if(Ok){Self->SiteSignature=Signature;Self->ProjectedRevision=-1;}else Self->Cached.Error=Error;}},Why)){bSyncPending=false;Cached.Error=Why;}return;
    }
    if(bProjectionReady&&ProjectionProfile==Bound&&ProjectedRevision==Cached.Revision)
    {State.EconomyClockSeconds=Cached.EconomySeconds;ObserveTraffic();return;}
    auto Request=MakeShared<FJsonObject>();Request->SetStringField(TEXT("action"),TEXT("npc_projection"));Request->SetStringField(TEXT("worldId"),Cached.WorldId);const FString World=Cached.WorldId;
    bSyncPending=true;FString Reason;
    if(!Command(Request,[Weak,Bound,World,LinkPath](bool Ok,TSharedPtr<FJsonObject> Reply,const FString& Error){
        auto* Self=Weak.Get();if(!Self)return;Self->bSyncPending=false;
        auto* P=Self->GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();auto* T=Self->GetGameInstance()->GetSubsystem<UGalaxyTrafficSubsystem>();
        if(!Ok||!Reply||!P||!P->HasLoaded()||!T||SnapshotProfileKey(P)!=Bound||Self->Cached.WorldId!=World){if(!Ok)Self->Cached.Error=Error;return;}
        FString Returned;double Revision=0;if(!Reply->TryGetStringField(TEXT("worldId"),Returned)||Returned!=World||!Reply->TryGetNumberField(TEXT("revision"),Revision)||Revision<0||Revision>9007199254740991.0||!FMath::IsFinite(Revision)||(Self->ProjectedRevision>=0&&Revision<Self->ProjectedRevision))
        {Self->bProjectionReady=false;Self->Cached.Error=TEXT("Rejected a stale or foreign NPC projection.");return;}
        FEconomyState Npc;FGalaxyTrafficState Trips;TMap<FGuid,FString> Ids;FString Why;
        if(!SSWEconomySnapshot::Decode(Reply,Npc,Trips,Ids,Why)){Self->bProjectionReady=false;Self->Cached.Error=Why;return;}
        auto Pin=MakeShared<FJsonObject>();Pin->SetStringField(TEXT("worldId"),World);
        if(!Durable(LinkPath,Pin)){Self->Cached.Error=TEXT("Could not persist the economy world link.");return;}
        const auto Before=P->GetEconomy();auto& Local=P->GetEconomy();const auto RemoteSites=SSWEconomySnapshot::SitesSignature(Npc);SSWEconomyOwnership::ApplyNpcProjection(Local,Npc);
        for(const auto& Pair:Before.GroundFactories)if(!Local.GroundFactories.Contains(Pair.Key))Local.GroundFactories.Add(Pair.Key,Pair.Value);
        if(!Before.NpcAuthorityWorldId.IsEmpty())for(FName Id:Before.OfflineFacilities)if(!Self->ProjectedOffline.Contains(Id))Local.OfflineFacilities.Add(Id);
        Self->ProjectedOffline=Npc.OfflineFacilities;Self->SiteSignature=RemoteSites;Local.NpcAuthorityWorldId=World;Local.NpcAuthorityRevision=int64(Revision);
        // Cached projections can be rebuilt, but the pilot/world pin must be durable before trading.
        if(Before.NpcAuthorityWorldId!=World&&!P->SaveActivePlayerProgress()){Local=Before;Self->Cached.Error=TEXT("Could not save the pilot economy link. No trades enabled.");return;}
        Self->ProjectedRevision=int64(Revision);Self->ProjectionProfile=Bound;Self->JourneyIds=MoveTemp(Ids);Self->bProjectionReady=true;Self->Cached.Error.Reset();
        T->ApplyExternalTraffic(MoveTemp(Trips));FSimDataEvents::Publish(Local);Self->OnStatusChanged.Broadcast();Self->RecoverPendingTrades();Self->ObserveTraffic();
    },Reason)){bSyncPending=false;Cached.Error=Reason;}
}
void UEconomyServiceSubsystem::ObserveTraffic()
{
    if(!IsReady())return;auto* T=GetGameInstance()->GetSubsystem<UGalaxyTrafficSubsystem>();if(!T)return;
    T->ReportExternalTraffic([this](FGuid Id,bool Lost,bool Arrived,EGalaxyTrafficPhase Phase){
        const auto* Native=JourneyIds.Find(Id);if(!Native)return;auto Body=MakeShared<FJsonObject>();Body->SetStringField(TEXT("action"),TEXT("observe_freight"));Body->SetStringField(TEXT("id"),FGuid::NewGuid().ToString(EGuidFormats::Digits));
        Body->SetStringField(TEXT("worldId"),Cached.WorldId);Body->SetStringField(TEXT("journey"),*Native);Body->SetBoolField(TEXT("lost"),Lost);Body->SetBoolField(TEXT("arrived"),Arrived);Body->SetStringField(TEXT("phase"),Phase==EGalaxyTrafficPhase::Loading||Phase==EGalaxyTrafficPhase::Waiting?TEXT("Pickup"):TEXT("Transit"));
        FString Why;Command(Body,{},Why);
    });
}
