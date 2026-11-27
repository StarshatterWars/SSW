#include "EconomySnapshot.h"
#include "EconomyOwnership.h"
#include "EconomyRuntime.h"
#include "GameStructs_economy.h"
#include "GameStructs_traffic.h"
#include "ProductionCatalog.h"
#include "JsonObjectConverter.h"
#include "Dom/JsonObject.h"
#include "Misc/SecureHash.h"
namespace SSWEconomySnapshot
{
FString SitesSignature(const FEconomyState& State)
{
    TArray<FString> Rows;for(const auto& P:State.GroundFactories)Rows.Add(P.Key.ToString());for(FName Id:State.OfflineFacilities)Rows.Add(TEXT("OFF/")+Id.ToString());Rows.Sort();return FMD5::HashAnsiString(*FString::Join(Rows,TEXT("\n")));
}
TSharedPtr<FJsonObject> Sites(const FEconomyState& State,FString& Reason)
{
    auto Out=MakeShared<FJsonObject>(),SitesJson=MakeShared<FJsonObject>(),Definitions=MakeShared<FJsonObject>();
    for(const auto& P:State.GroundFactories){FSSWProductionFacility Site;if(!SSWProductionCatalog::ResolveGround(P.Value,Site)){Reason=TEXT("A discovered building has no production profile.");return nullptr;}
        auto Definition=MakeShared<FJsonObject>(),Row=MakeShared<FJsonObject>();
        if(!FJsonObjectConverter::UStructToJsonObject(FSSWProductionFacility::StaticStruct(),&Site,Definition,0,0)||!FJsonObjectConverter::UStructToJsonObject(FGroundFactorySite::StaticStruct(),&P.Value,Row,0,0)){Reason=TEXT("A discovered site could not be serialized.");return nullptr;}
        Definitions->SetObjectField(P.Key.ToString(),Definition);SitesJson->SetObjectField(P.Key.ToString(),Row);
    }
    TArray<TSharedPtr<FJsonValue>> Offline;for(FName Id:State.OfflineFacilities)Offline.Add(MakeShared<FJsonValueString>(Id.ToString()));
    Out->SetObjectField(TEXT("facilities"),Definitions);Out->SetObjectField(TEXT("sites"),SitesJson);Out->SetArrayField(TEXT("offline"),Offline);return Out;
}
TSharedPtr<FJsonObject> Capture(const FEconomyState& Source,const FGalaxyTrafficState& Traffic,FString& Reason)
{
    auto Npc=SSWEconomyOwnership::ExtractNpc(Source);FGalaxyTrafficState Trips=Traffic;
    Trips.Journeys.RemoveAll([](const auto& J){return J.bPatrol||!J.IsCargo()||J.IsFinished();});
    for(const auto& J:Trips.Journeys)
    {
        if(!Npc.Inventories.Contains(J.SourceInventory)||!Npc.Inventories.Contains(J.DestinationInventory)||!Npc.Inventories.Contains(J.Hold))
        {Reason=TEXT("A freight journey does not resolve to NPC-owned inventories.");return nullptr;}
        const FName Escrow(*(TEXT("NPC freight escrow/")+J.Id.ToString(EGuidFormats::Digits)));
        if(J.TradeCredits>0&&!J.bTradeSettled)
        {
            if(Source.Wallets.FindRef(Escrow)!=J.TradeCredits){Reason=TEXT("A freight journey has an unmatched payment escrow.");return nullptr;}
            Npc.Wallets.Add(Escrow,J.TradeCredits);
        }
    }
    auto Economy=MakeShared<FJsonObject>(),TrafficJson=MakeShared<FJsonObject>(),Facilities=MakeShared<FJsonObject>(),Wallets=MakeShared<FJsonObject>();
    if(!FJsonObjectConverter::UStructToJsonObject(FEconomyState::StaticStruct(),&Npc,Economy,0,0)||!FJsonObjectConverter::UStructToJsonObject(FGalaxyTrafficState::StaticStruct(),&Trips,TrafficJson,0,0))
    {Reason=TEXT("The NPC snapshot could not be serialized.");return nullptr;}
    for(const auto& P:Npc.Wallets)Wallets->SetStringField(P.Key.ToString(),FString::Printf(TEXT("%lld"),P.Value));
    Economy->SetObjectField(TEXT("wallets"),Wallets);
    // Reflection lowercases FName map keys as if they were property names. IDs
    // are data, so preserve their exact spelling at this boundary.
    auto Inventories=MakeShared<FJsonObject>();for(const auto& P:Npc.Inventories){auto Row=MakeShared<FJsonObject>();FJsonObjectConverter::UStructToJsonObject(FEconomyInventory::StaticStruct(),&P.Value,Row,0,0);Inventories->SetObjectField(P.Key.ToString(),Row);}Economy->SetObjectField(TEXT("inventories"),Inventories);
    const auto ExactNumbers=[&](const TCHAR* Field,const TMap<FName,double>& Values){auto Object=MakeShared<FJsonObject>();for(const auto& P:Values)Object->SetNumberField(P.Key.ToString(),P.Value);Economy->SetObjectField(Field,Object);};
    ExactNumbers(TEXT("backgroundOreRemaining"),Npc.BackgroundOreRemaining);ExactNumbers(TEXT("backgroundMineDeadlines"),Npc.BackgroundMineDeadlines);ExactNumbers(TEXT("backgroundMineLastCycle"),Npc.BackgroundMineLastCycle);
    auto Ground=MakeShared<FJsonObject>();for(const auto& P:Npc.GroundFactories){auto Row=MakeShared<FJsonObject>();FJsonObjectConverter::UStructToJsonObject(FGroundFactorySite::StaticStruct(),&P.Value,Row,0,0);Ground->SetObjectField(P.Key.ToString(),Row);}Economy->SetObjectField(TEXT("groundFactories"),Ground);

    // Preserve int64 freight escrow exactly across the JSON boundary too.
    const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
    if(TrafficJson->TryGetArrayField(TEXT("journeys"),Rows))for(int32 I=0;I<Rows->Num();++I)(*Rows)[I]->AsObject()->SetStringField(TEXT("tradeCredits"),FString::Printf(TEXT("%lld"),Trips.Journeys[I].TradeCredits));
    SSWProductionCatalog::RestoreGround(Npc);
    for(const auto& P:SSWProductionCatalog::Facilities())
    {auto Row=MakeShared<FJsonObject>();if(!FJsonObjectConverter::UStructToJsonObject(FSSWProductionFacility::StaticStruct(),&P.Value,Row,0,0)){Reason=TEXT("A facility could not be serialized.");return nullptr;}Facilities->SetObjectField(P.Key.ToString(),Row);}
    auto Out=MakeShared<FJsonObject>();Out->SetNumberField(TEXT("schema"),1);Out->SetNumberField(TEXT("utc"),double(FDateTime::UtcNow().ToUnixTimestamp()));
    Out->SetObjectField(TEXT("economy"),Economy);Out->SetObjectField(TEXT("traffic"),TrafficJson);Out->SetObjectField(TEXT("facilities"),Facilities);return Out;
}
bool Decode(const TSharedPtr<FJsonObject>& Reply,FEconomyState& Economy,FGalaxyTrafficState& Traffic,TMap<FGuid,FString>& NativeIds,FString& Reason)
{
    const TSharedPtr<FJsonObject>* E=nullptr;const TSharedPtr<FJsonObject>* T=nullptr;
    if(!Reply||!Reply->TryGetObjectField(TEXT("economy"),E)||!Reply->TryGetObjectField(TEXT("traffic"),T)){Reason=TEXT("Missing NPC projection records.");return false;}
    // Credits use decimal strings. Reflection's numeric conversion would otherwise lose large int64 values.
    auto Copy=MakeShared<FJsonObject>(**E);const TSharedPtr<FJsonObject>* Wallets=nullptr;
    if(!Copy->TryGetObjectField(TEXT("wallets"),Wallets)){Reason=TEXT("Missing NPC budgets.");return false;}
    TMap<FName,int64> Exact;
    for(const auto& P:(*Wallets)->Values)
    {
        FString Amount;if(!P.Value->TryGetString(Amount)||Amount.IsEmpty()||Amount.Len()>19){Reason=TEXT("Invalid exact NPC budget.");return false;}
        for(TCHAR C:Amount)if(C<'0'||C>'9'){Reason=TEXT("Invalid NPC budget digits.");return false;}
        uint64 Value=0;if(!LexTryParseString(Value,*Amount)||Value>uint64(MAX_int64)){Reason=TEXT("NPC budget overflow.");return false;}
        if(!P.Key.StartsWith(TEXT("NPC/"))&&!P.Key.StartsWith(TEXT("Residents/"))){Reason=TEXT("Projection tried to write a player budget.");return false;}Exact.Add(FName(*P.Key),int64(Value));
    }
    Copy->RemoveField(TEXT("wallets"));FEconomyState Decoded;FGalaxyTrafficState Journeys;
    FText Failure;
    if(!FJsonObjectConverter::JsonObjectToUStruct(Copy,FEconomyState::StaticStruct(),&Decoded,0,0,false,&Failure)||!FJsonObjectConverter::JsonObjectToUStruct((*T).ToSharedRef(),FGalaxyTrafficState::StaticStruct(),&Journeys,0,0,false,&Failure))
    {Reason=TEXT("Invalid NPC projection: ")+Failure.ToString();return false;}
    Decoded.Wallets=MoveTemp(Exact);
    for(auto& P:Decoded.Inventories)
    {
        auto& I=P.Value;for(auto& Item:I.Items)Item.Type=DataTableUtils::MakeValidName(Item.Type.ToString());
        const auto Normalize=[](auto& Map){auto Before=MoveTemp(Map);Map.Reset();for(const auto& V:Before)Map.Add(DataTableUtils::MakeValidName(V.Key.ToString()),V.Value);};
        Normalize(I.AskPrices);Normalize(I.BidPrices);Normalize(I.DesiredStock);
    }
    for(auto& P:Decoded.Reservations)P.Value.Item=DataTableUtils::MakeValidName(P.Value.Item.ToString());
    auto Histories=MoveTemp(Decoded.RegionalMarkets);Decoded.RegionalMarkets.Reset();for(auto& P:Histories){auto& H=P.Value;H.Item=DataTableUtils::MakeValidName(H.Item.ToString());Decoded.RegionalMarkets.Add(FName(*(H.System.ToString()+TEXT("/")+H.Item.ToString())),MoveTemp(H));}
    for(auto& J:Decoded.Jobs){for(auto& A:J.Materials)A.Item=DataTableUtils::MakeValidName(A.Item.ToString());for(auto& A:J.Products)A.Item=DataTableUtils::MakeValidName(A.Item.ToString());}
    for(auto& J:Journeys.Journeys)J.Item=DataTableUtils::MakeValidName(J.Item.ToString());
    if(!FMath::IsFinite(Decoded.EconomyClockSeconds)||Decoded.EconomyClockSeconds<0){Reason=TEXT("Invalid server economy clock.");return false;}
    for(const auto& P:Decoded.Inventories)
    {
        const auto& I=P.Value;if(I.Owner!=FEconomyRuntime::IndustryOwner()||!FMath::IsFinite(I.CapacityTonnes)||I.CapacityTonnes<0||!FMath::IsFinite(I.ReservedOutputTonnes)||I.ReservedOutputTonnes<0||FEconomyLedger::Used(I)+I.ReservedOutputTonnes>I.CapacityTonnes+1.e-5)
        {Reason=TEXT("Invalid NPC inventory ownership or capacity.");return false;}
        for(const auto& Item:I.Items)if(!Item.Id.IsValid()||Item.Type.IsNone()||!FMath::IsFinite(Item.Quantity)||Item.Quantity<0||!FMath::IsFinite(Item.UnitTonnes)||Item.UnitTonnes<0){Reason=TEXT("Invalid NPC item record.");return false;}
    }
    TMap<FGuid,FString> Ids;const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
    if((*T)->TryGetArrayField(TEXT("journeys"),Rows))for(const auto& Value:*Rows)
    {FString Id,Native;auto Row=Value->AsObject();FGuid Guid;if(!Row||!Row->TryGetStringField(TEXT("id"),Id)||!FGuid::Parse(Id,Guid)||!Row->TryGetStringField(TEXT("nativeId"),Native)||Native.IsEmpty()){Reason=TEXT("Invalid freight projection identity.");return false;}Ids.Add(Guid,Native);}
    Economy=MoveTemp(Decoded);Traffic=MoveTemp(Journeys);NativeIds=MoveTemp(Ids);return true;
}
}
