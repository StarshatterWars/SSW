#include "CarrierFighterSalesAI.h"
#include "SerializedHullStockAI.h"
#include "Hangar.h"
#include "Ship.h"
#include "ShipDesignRegistry.h"
#include "ShipyardAI.h"
#include "EconomyRuntime.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
struct FOOBFaction {FString Name;TArray<TSharedPtr<FJsonValue>> Rows;};
const TArray<FOOBFaction>& Rosters()
{
    static const TArray<FOOBFaction> Cached=[]()
    {
        TArray<FOOBFaction> Out;TArray<FString> Files;
        const FString Dir=FPaths::ProjectContentDir()/TEXT("GameData/Campaigns/OOB");
        IFileManager::Get().FindFiles(Files,*(Dir/TEXT("*.json")),true,false);Files.Sort();
        for(const auto& File:Files)
        {
            FString Text;FOOBFaction Row;Row.Name=FPaths::GetBaseFilename(File);
            if(FFileHelper::LoadFileToString(Text,*(Dir/File)) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Row.Rows))Out.Add(MoveTemp(Row));
        }
        return Out;
    }();return Cached;
}
FString GroupKey(const FString& Type,int32 Id){return Type+TEXT(":")+FString::FromInt(Id);}
FString FactionFile(int32 Empire)
{
    if(Empire==int32(EEMPIRE_NAME::Terellian))return TEXT("Alliance");
    if(Empire==int32(EEMPIRE_NAME::Marakan))return TEXT("Hegemony");
    const FString Name=StaticEnum<EEMPIRE_NAME>()->GetNameStringByValue(Empire);
    if(Name==TEXT("Zolon"))return TEXT("Zolons");
    if(Name==TEXT("Pirate"))return TEXT("Pirates");
    if(Name==TEXT("Silessian"))return TEXT("Silessia");
    return StaticEnum<EEMPIRE_NAME>()->GetNameStringByValue(Empire);
}
int64 Price(const FS_PlayerGameInfo& P,FName Design,int64 Normal)
{
    static const TMap<FName,int64> Prices=[]()
    {
        TMap<FName,int64> Out;FString Text;TSharedPtr<FJsonObject> Root;double Version=0;
        if(!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectContentDir()/TEXT("GameData/Production/carrier_fighter_sales.json"))) ||
           !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root) || !Root || !Root->TryGetNumberField(TEXT("schemaVersion"),Version) || Version!=1)return Out;
        const TSharedPtr<FJsonObject>* Values=nullptr;
        if(Root->TryGetObjectField(TEXT("trainingPrices"),Values))for(const auto& Pair:(*Values)->Values)
        {double N=0;if(Pair.Value->TryGetNumber(N) && FMath::IsFinite(N) && N>0 && N<=1.e12 && N==FMath::FloorToDouble(N))Out.Add(FName(*Pair.Key),int64(N));}
        return Out;
    }();
    if(CarrierFighterSalesAI::IsTrainingPilot(P))if(const auto* Value=Prices.Find(Design))return FMath::Min(*Value,Normal);
    return Normal;
}
bool RosterSquadron(const FCarrierFighterOffer& O,const FString& Name)
{return O.Squadrons.ContainsByPredicate([&](const auto& S){return S.Key.Equals(Name,ESearchCase::IgnoreCase);});}
int32 ExternalAssigned(Hangar& H,const FCarrierFighterOffer& O)
{
    int32 N=0;
    for(int32 S=0;S<H.NumSquadrons();++S)
    {
        if(RosterSquadron(O,UTF8_TO_TCHAR(H.SquadronName(S).data())))continue;
        for(int32 I=0;I<H.SquadronSize(S);++I)
        {
            const auto* Row=H.GetSlot(S,I);const auto* Design=H.GetDesign(Row);const int32 State=H.GetState(Row);
            if(Design && FName(UTF8_TO_TCHAR(Design->name))==O.Fighter && State!=Hangar::STORAGE && State!=Hangar::TRANSFERRED)++N;
        }
    }
    return N;
}
}
bool CarrierFighterSalesAI::IsTrainingPilot(const FS_PlayerGameInfo& P)
{return P.Campaign==1 && !P.bTrainingGraduate;}
TArray<FCarrierFighterOffer> CarrierFighterSalesAI::Offers(const FS_PlayerGameInfo& P,const Ship& Base)
{
    TArray<FCarrierFighterOffer> Out;
    const auto* Faction=Rosters().FindByPredicate([&](const auto& F){return F.Name.Equals(FactionFile(P.Empire),ESearchCase::IgnoreCase);});
    if(!Faction)return Out;
    FString RootKey;
    for(const auto& Value:Faction->Rows)
    {
        const auto Row=Value->AsObject();FString Type;int32 Id=0;const TArray<TSharedPtr<FJsonValue>>* Units=nullptr;
        if(!Row || !Row->TryGetStringField(TEXT("Type"),Type) || Type!=TEXT("CARRIER_GROUP") || !Row->TryGetNumberField(TEXT("Id"),Id) || !Row->TryGetArrayField(TEXT("Unit"),Units))continue;
        for(const auto& Unit:*Units)
        {
            const auto U=Unit->AsObject();FString Name,Design;
            if(U && U->TryGetStringField(TEXT("UnitName"),Name) && U->TryGetStringField(TEXT("UnitDesign"),Design) &&
               Name.Equals(UTF8_TO_TCHAR(Base.GetName()),ESearchCase::IgnoreCase) && Design.Equals(UTF8_TO_TCHAR(Base.DesignName()),ESearchCase::IgnoreCase))
            {if(!RootKey.IsEmpty())return TArray<FCarrierFighterOffer>();RootKey=GroupKey(Type,Id);}
        }
    }
    if(RootKey.IsEmpty())return Out;
    TSet<FString> Descendants;Descendants.Add(RootKey);
    for(int32 Pass=0;Pass<Faction->Rows.Num();++Pass)
    {
        const int32 Before=Descendants.Num();
        for(const auto& Value:Faction->Rows)
        {
            const auto R=Value->AsObject();FString Type,Parent;int32 Id=0,ParentId=0;
            if(R && R->TryGetStringField(TEXT("Type"),Type) && R->TryGetNumberField(TEXT("Id"),Id) && R->TryGetStringField(TEXT("ParentType"),Parent) && R->TryGetNumberField(TEXT("ParentId"),ParentId) && Descendants.Contains(GroupKey(Parent,ParentId)))Descendants.Add(GroupKey(Type,Id));
        }
        if(Before==Descendants.Num())break;
    }
    for(const auto& Value:Faction->Rows)
    {
        const auto R=Value->AsObject();FString Type,Name;int32 Id=0;const TArray<TSharedPtr<FJsonValue>>* Units=nullptr;
        if(!R || !R->TryGetStringField(TEXT("Type"),Type) || !Type.EndsWith(TEXT("SQUADRON")) || !R->TryGetNumberField(TEXT("Id"),Id) ||
           !Descendants.Contains(GroupKey(Type,Id)) || !R->TryGetStringField(TEXT("Name"),Name) || !R->TryGetArrayField(TEXT("Unit"),Units))continue;
        const FString Caption=FString::Printf(TEXT("%d %s"),Id,*Name.ToUpper());
        for(const auto& V:*Units)
        {
            const auto U=V->AsObject();FString Design;int32 Count=0,Dead=0;double Damage=0;
            if(!U || !U->TryGetStringField(TEXT("UnitDesign"),Design) || !U->TryGetNumberField(TEXT("UnitCount"),Count) || Count<=0 || Count>10000)continue;
            U->TryGetNumberField(TEXT("UnitDead"),Dead);U->TryGetNumberField(TEXT("UnitDamage"),Damage);
            Count=FMath::Max(0,Count-FMath::Max(0,Dead));if(!Count || Damage>0)continue;
            const FName Key(*Design);const auto* Hull=ShipDesignRegistry::Find(Key);
            if(!Hull || (Hull->Category!=EShipCategory::Fighter && Hull->Category!=EShipCategory::Shuttle && Hull->Category!=EShipCategory::Transport))continue;
            auto* O=Out.FindByPredicate([&](const auto& A){return A.Fighter==Key;});
            if(!O){FCarrierFighterOffer New;New.Carrier=FName(UTF8_TO_TCHAR(Base.DesignName()));New.Fighter=Key;New.Squadron=Caption;New.Price=Price(P,Key,Hull->BaseCost);Out.Add(New);O=&Out.Last();}
            O->Quantity+=Count;O->Squadrons.Emplace(Caption,Count);
        }
    }
    return Out;
}
TOptional<FCarrierFighterOffer> CarrierFighterSalesAI::Offer(const FS_PlayerGameInfo& P,const Ship& Base,FName Fighter)
{
    const auto All=Offers(P,Base);if(const auto* Row=All.FindByPredicate([&](const auto& O){return O.Fighter==Fighter;}))return *Row;
    return {};
}
FName CarrierFighterSalesAI::Inventory(FName System,const FString& CarrierName,FName Fighter)
{const FString Compartment=TEXT("Squadron Sales/")+Fighter.ToString();return FEconomyRuntime::Warehouse(System.ToString(),CarrierName,*Compartment);}
bool CarrierFighterSalesAI::FindReserve(Hangar& H,const FCarrierFighterOffer& O,int32& Squadron,int32& Slot)
{
    for(int32 S=0;S<H.NumSquadrons();++S)
    {
        if(!RosterSquadron(O,UTF8_TO_TCHAR(H.SquadronName(S).data())))continue;
        for(int32 I=0;I<H.SquadronSize(S);++I)
        {
            const auto* Row=H.GetSlot(S,I);const auto* Design=H.GetDesign(Row);
            if(Row && Design && FName(UTF8_TO_TCHAR(Design->name))==O.Fighter && H.GetState(Row)==Hangar::STORAGE && !H.GetShip(Row) && !H.GetPackageElement(Row) && !H.GetFlightDeck(Row))
            {Squadron=S;Slot=I;return true;}
        }
    }
    return false;
}
int32 CarrierFighterSalesAI::Available(const FEconomyState& State,FName Key,Hangar& H,const FCarrierFighterOffer& O)
{
    int32 Blocked=ExternalAssigned(H,O),Stored=0;
    for(int32 S=0;S<H.NumSquadrons();++S)if(RosterSquadron(O,UTF8_TO_TCHAR(H.SquadronName(S).data())))
        for(int32 I=0;I<H.SquadronSize(S);++I)
        {const auto* R=H.GetSlot(S,I);const int32 Status=H.GetState(R);if(Status==Hangar::STORAGE && !H.GetShip(R) && !H.GetPackageElement(R) && !H.GetFlightDeck(R))++Stored;else if(Status!=Hangar::TRANSFERRED)++Blocked;}
    return FMath::Min(Stored,FMath::Max(0,FMath::FloorToInt(ShipyardAI::Stock(State,Key,O.Fighter))-Blocked));
}
namespace
{
bool SerializedReserve(Hangar& H,const FCarrierFighterOffer& Offer,const FSerializedHullStock& Record,int32& Squadron)
{
    for(int32 S=0;S<H.NumSquadrons();++S)
    {
        const FString Name=UTF8_TO_TCHAR(H.SquadronName(S).data());
        if(!Name.Equals(Record.Squadron,ESearchCase::IgnoreCase) || !RosterSquadron(Offer,Name) || Record.Slot<0 || Record.Slot>=H.SquadronSize(S))continue;
        const auto* Row=H.GetSlot(S,Record.Slot);
        if(H.GetState(Row)==Hangar::STORAGE && !H.GetShip(Row) && !H.GetPackageElement(Row) && !H.GetFlightDeck(Row)){Squadron=S;return true;}
    }
    return false;
}
}
bool CarrierFighterSalesAI::BindHullOffers(FEconomyState& State,FName Key,Hangar& H,const FCarrierFighterOffer& Offer)
{
    bool Changed=false;TSet<FString> Bound;
    for(const auto& Pair:State.SerializedHulls)if(Pair.Value.Inventory==Key && !Pair.Value.Squadron.IsEmpty())
        Bound.Add(Pair.Value.Squadron.ToUpper()+TEXT("/")+FString::FromInt(Pair.Value.Slot));
    const auto Ids=SerializedHullStockAI::Available(State,Key,Offer.Fighter,Offer.Quantity);
    for(const auto Id:Ids)
    {
        auto& Record=State.SerializedHulls.FindChecked(Id);if(!Record.Squadron.IsEmpty())continue;
        bool Found=false;
        for(int32 S=0;S<H.NumSquadrons() && !Found;++S)
        {
            const FString Name=UTF8_TO_TCHAR(H.SquadronName(S).data());if(!RosterSquadron(Offer,Name))continue;
            for(int32 I=0;I<H.SquadronSize(S);++I)
            {
                const FString SlotKey=Name.ToUpper()+TEXT("/")+FString::FromInt(I);if(Bound.Contains(SlotKey))continue;
                const auto* Row=H.GetSlot(S,I);
                if(H.GetState(Row)!=Hangar::STORAGE || H.GetShip(Row) || H.GetPackageElement(Row) || H.GetFlightDeck(Row))continue;
                Record.Squadron=Name;Record.Slot=I;Bound.Add(SlotKey);Found=Changed=true;break;
            }
        }
    }
    return Changed;
}
TArray<FGuid> CarrierFighterSalesAI::HullOffers(const FEconomyState& State,FName Key,Hangar& H,const FCarrierFighterOffer& Offer)
{
    auto Ids=SerializedHullStockAI::Available(State,Key,Offer.Fighter,Offer.Quantity);
    Ids.RemoveAll([&](const FGuid& Id){int32 Squadron=INDEX_NONE;return !SerializedReserve(H,Offer,State.SerializedHulls.FindChecked(Id),Squadron);});
    Ids.SetNum(FMath::Min(Ids.Num(),Available(State,Key,H,Offer)));return Ids;
}
bool CarrierFighterSalesAI::Prepare(FS_PlayerGameInfo& P,Ship& Base,FName System,TFunctionRef<bool()> Save,FText& Why)
{
    auto* H=Base.GetHangar();const auto All=Offers(P,Base);if(!H || System.IsNone() || All.IsEmpty())return false;
    const auto Before=P;bool Changed=false;
    int32 Needed=0;for(const auto& O:All)for(const auto& Squad:O.Squadrons)
    {bool Exists=false;for(int32 S=0;S<H->NumSquadrons();++S)if(Squad.Key.Equals(UTF8_TO_TCHAR(H->SquadronName(S).data()),ESearchCase::IgnoreCase))Exists=true;if(!Exists)++Needed;}
    if(Needed+H->NumSquadrons()>Hangar::MAX_SQUADRONS){Why=INVTEXT("Carrier hangar cannot accommodate the authored OOB squadrons.");return false;}
    for(const auto& O:All)
    {
        const FName Key=Inventory(System,UTF8_TO_TCHAR(Base.GetName()),O.Fighter);
        if(P.Economy.Inventories.Contains(Key))continue;
        auto* Store=FEconomyLedger::CreateInventory(P.Economy,Key,FEconomyRuntime::IndustryOwner(),System,double(O.Quantity));
        if(!Store){P=Before;return false;}Store->bHullStorage=true;Store->bOperational=true;
        const FName Item=DataTableUtils::MakeValidName(O.Fighter.ToString().ToLower()+TEXT(" assembled hull"));
        if(!FEconomyLedger::Deposit(P.Economy,Key,Item,O.Quantity,1)){P=Before;return false;}Changed=true;
    }
    if(Changed && !Save()){P=Before;Why=INVTEXT("Could not save carrier squadron stock.");return false;}
    for(const auto& O:All)
    {
        for(const auto& Squad:O.Squadrons)
        {
            bool Exists=false;for(int32 S=0;S<H->NumSquadrons();++S)if(Squad.Key.Equals(UTF8_TO_TCHAR(H->SquadronName(S).data()),ESearchCase::IgnoreCase))Exists=true;
            if(!Exists && !H->CreateSquadron(TCHAR_TO_UTF8(*Squad.Key),nullptr,ShipDesignRegistry::FindLegacy(O.Fighter),Squad.Value,Base.GetIFF()))return false;
        }
        const FName StockKey=Inventory(System,UTF8_TO_TCHAR(Base.GetName()),O.Fighter);
        TSet<FString> SoldSlots;
        for(const auto& Pair:P.Economy.SerializedHulls)
        {
            const auto& Record=Pair.Value;if(Record.Inventory!=StockKey || !Record.SoldReceipt.IsValid() || Record.Squadron.IsEmpty())continue;
            SoldSlots.Add(Record.Squadron.ToUpper()+TEXT("/")+FString::FromInt(Record.Slot));
            for(int32 S=0;S<H->NumSquadrons();++S)if(Record.Squadron.Equals(UTF8_TO_TCHAR(H->SquadronName(S).data()),ESearchCase::IgnoreCase) && Record.Slot>=0 && Record.Slot<H->SquadronSize(S))H->TransferStoredReserve(S,Record.Slot);
        }
        const int32 Target=O.Quantity-FMath::Clamp(FMath::FloorToInt(ShipyardAI::Stock(P.Economy,Inventory(System,UTF8_TO_TCHAR(Base.GetName()),O.Fighter),O.Fighter)),0,O.Quantity)+ExternalAssigned(*H,O);
        int32 Removed=0;for(int32 S=0;S<H->NumSquadrons();++S)if(RosterSquadron(O,UTF8_TO_TCHAR(H->SquadronName(S).data())))
            for(int32 I=0;I<H->SquadronSize(S);++I)if(H->GetState(H->GetSlot(S,I))==Hangar::TRANSFERRED)++Removed;
        for(int32 S=0;S<H->NumSquadrons();++S)if(RosterSquadron(O,UTF8_TO_TCHAR(H->SquadronName(S).data())))
            for(int32 I=0;I<H->SquadronSize(S);++I)
            {
                if(Removed<Target && H->TransferStoredReserve(S,I))++Removed;
                else if(Removed>Target && !SoldSlots.Contains(FString(UTF8_TO_TCHAR(H->SquadronName(S).data())).ToUpper()+TEXT("/")+FString::FromInt(I)) && H->TransferStoredReserve(S,I,true))--Removed;
            }
    }
    return true;
}
bool CarrierFighterSalesAI::Purchase(FS_PlayerGameInfo& P,const FShipDesign& Hull,Hangar& H,const FCarrierFighterOffer& O,FName Key,const FString& Location,FGuid Receipt,TFunctionRef<bool()> Save,FText& Why,FGuid OfferedHullId)
{
    if(FName(*Hull.ShipName)!=O.Fighter || O.Price<=0){Why=INVTEXT("This design is not in the carrier OOB.");return false;}
    if(P.Economy.CommittedTransactions.Contains(Receipt)){FShipDesign Sale=Hull;Sale.BaseCost=O.Price;return ShipyardAI::Purchase(P,Sale,Key,Location,Receipt,Why,OfferedHullId);}
    int32 Squadron=INDEX_NONE,Slot=INDEX_NONE;
    if(Available(P.Economy,Key,H,O)<1 || !FindReserve(H,O,Squadron,Slot)){Why=INVTEXT("No unassigned OOB squadron aircraft is available.");return false;}
    const auto Before=P;FShipDesign Sale=Hull;Sale.BaseCost=O.Price;
    if(!OfferedHullId.IsValid())
    {
        const FName Item=DataTableUtils::MakeValidName(O.Fighter.ToString().ToLower()+TEXT(" assembled hull"));
        SerializedHullStockAI::Ensure(P.Economy,Key,O.Fighter,FMath::FloorToInt(FEconomyLedger::Quantity(P.Economy,Key,Item)));
        BindHullOffers(P.Economy,Key,H,O);
        const auto AvailableIds=HullOffers(P.Economy,Key,H,O);
        if(!AvailableIds.IsEmpty())OfferedHullId=AvailableIds[0];
    }
    {
        BindHullOffers(P.Economy,Key,H,O);
        const auto* Record=P.Economy.SerializedHulls.Find(OfferedHullId);
        if(!Record || Record->Inventory!=Key || Record->Design!=O.Fighter || !SerializedReserve(H,O,*Record,Squadron))
        {P=Before;Why=INVTEXT("The selected squadron hull is no longer available.");return false;}
        Slot=Record->Slot;
    }
    if(!ShipyardAI::Purchase(P,Sale,Key,Location,Receipt,Why,OfferedHullId)){P=Before;return false;}
    if(!H.TransferStoredReserve(Squadron,Slot)){P=Before;Why=INVTEXT("The fighter was assigned before purchase.");return false;}
    if(!Save()){P=Before;H.TransferStoredReserve(Squadron,Slot,true);Why=INVTEXT("Could not save the purchase. Credits and squadron stock restored.");return false;}
    Why=FText::Format(INVTEXT("{0} purchased from {1} and delivered to your hangar."),FText::FromString(Hull.DisplayName.IsEmpty()?Hull.ShipName:Hull.DisplayName),FText::FromString(UTF8_TO_TCHAR(H.SquadronName(Squadron).data())));return true;
}
