#include "OrdnanceMagazine.h"
#include "ShipStoreAI.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "Weapon.h"
#include "WeaponDesign.h"
#include "WeaponGroup.h"
#include "HardPoint.h"
#include "Sim.h"
#include "SimRegion.h"
#include "StarSystem.h"
#include "EconomyRuntime.h"
#include "SimDataEvents.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

namespace
{
struct FMagazinePolicy
{
    int32 Carrier=500,Station=200;
    double UnitTonnes=.01,CarrierCapacity=500,StationCapacity=200;
    TMap<FName,int32> Overrides;
    TArray<FName> Items;
    bool Valid=false;
};
const FMagazinePolicy& Policy()
{
    static const FMagazinePolicy Result=[]()
    {
        FMagazinePolicy Out;FString Text;TSharedPtr<FJsonObject> Root;double Version=0,Carrier=0,Station=0;
        if(!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectContentDir()/TEXT("GameData/Ships/missile_stores.json"))) ||
            !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root) || !Root ||
            !Root->TryGetNumberField(TEXT("schemaVersion"),Version) || Version!=1 ||
            !Root->TryGetNumberField(TEXT("CarrierRoundsPerType"),Carrier) || !Root->TryGetNumberField(TEXT("StationRoundsPerType"),Station) ||
            !FMath::IsFinite(Carrier) || !FMath::IsFinite(Station) || Carrier<0 || Station<0 || Carrier>1000000 || Station>1000000)return Out;
        if(Carrier!=FMath::FloorToDouble(Carrier) || Station!=FMath::FloorToDouble(Station) ||
            !Root->TryGetNumberField(TEXT("UnitTonnes"),Out.UnitTonnes) || !FMath::IsFinite(Out.UnitTonnes) || Out.UnitTonnes<=0 || Out.UnitTonnes>100 ||
            !Root->TryGetNumberField(TEXT("CarrierCapacityTonnes"),Out.CarrierCapacity) || !FMath::IsFinite(Out.CarrierCapacity) || Out.CarrierCapacity<=0 || Out.CarrierCapacity>1000000 ||
            !Root->TryGetNumberField(TEXT("StationCapacityTonnes"),Out.StationCapacity) || !FMath::IsFinite(Out.StationCapacity) || Out.StationCapacity<=0 || Out.StationCapacity>1000000)return Out;
        Out.Carrier=int32(Carrier);Out.Station=int32(Station);
        const TSharedPtr<FJsonObject>* Overrides=nullptr;
        if(Root->TryGetObjectField(TEXT("DesignOverrides"),Overrides))for(const auto& Pair:(*Overrides)->Values)
        {double Value=0;if(Pair.Value->TryGetNumber(Value) && FMath::IsFinite(Value) && Value>=0 && Value<=1000000 && Value==FMath::FloorToDouble(Value))Out.Overrides.Add(FName(*Pair.Key),int32(Value));}
        if(!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectContentDir()/TEXT("GameData/Ships/store_equipment.json"))) ||
            !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root) || !Root)return Out;
        const TArray<TSharedPtr<FJsonValue>>* Items=nullptr;if(!Root->TryGetArrayField(TEXT("Items"),Items))return Out;
        for(const auto& Value:*Items)
        {
            const auto Item=Value->AsObject();FString Id,Weapon;
            if(Item && Item->TryGetStringField(TEXT("Id"),Id) && Item->TryGetStringField(TEXT("Weapon"),Weapon) && !Weapon.IsEmpty())Out.Items.AddUnique(FName(*Id));
        }
        Out.Valid=true;return Out;
    }();return Result;
}
bool Add(TMap<FName,int32>& Counts,FName Id,int32 Amount)
{
    if(Id.IsNone() || Amount<0 || Amount>1000000 || Counts.FindRef(Id)>1000000-Amount)return false;
    Counts.FindOrAdd(Id)+=Amount;return true;
}
TArray<int32> Stations(Ship& Craft)
{
    TArray<int32> Out;
    if(Craft.Design())for(int32 Index=0;Index<Craft.Design()->hardpoints.size();++Index)Out.Add(Craft.GetLoadout()?Craft.GetLoadout()[Index]:-1);
    return Out;
}
int32 Capacity(Ship& Craft,const Weapon& Runtime)
{
    const auto* Design=Craft.Design();if(!Design)return 0;
    const int32 Id=Runtime.GetIndex();
    if(Id>0 && Id<=Design->weapons.size() && Design->weapons[Id-1])return FMath::Max(0,Design->weapons[Id-1]->Ammo());
    int32 Index=Design->weapons.size();
    for(int32 Station=0;Station<Design->hardpoints.size();++Station)
        if(Craft.GetLoadout() && Craft.GetLoadout()[Station]>=0)
        {
            ++Index;if(Index!=Id)continue;
            TUniquePtr<Weapon> Prototype(Design->hardpoints[Station]?Design->hardpoints[Station]->CreateWeapon(Craft.GetLoadout()[Station]):nullptr);
            return Prototype?FMath::Max(0,Prototype->Ammo()):0;
        }
    return FMath::Max(0,Runtime.Ammo());
}
void SetFromPool(Ship& Craft,TMap<FName,int32> Pool)
{
    for(ListIter<WeaponGroup> Groups=Craft.GetWeapons();++Groups;)
        for(ListIter<Weapon> Weapons=Groups->GetWeapons();++Weapons;)
            if(Weapons->Design() && !Weapons->IsPrimary() && Weapons->Ammo()>=0)
            {
                const auto Id=OrdnanceMagazine::ItemId(*Weapons->Design());
                const int32 Count=FMath::Min(Capacity(Craft,*Weapons.value()),Pool.FindRef(Id));Weapons->SetAmmo(Count);Pool.FindOrAdd(Id)-=Count;
            }
}
}

FName OrdnanceMagazine::ItemId(const WeaponDesign& Design)
{
    const bool Missile=!Design.drone || FCStringAnsi::Stricmp(Design.group.data(),"TORP")==0;
    return FName(*(FString(Missile?TEXT("Missile/"):TEXT("Drone/"))+UTF8_TO_TCHAR(Design.name.data())));
}

FName OrdnanceMagazine::Key(const Ship& Base)
{
    auto* Region=Base.GetRegion();const auto* System=Region?Region->GetSystem():nullptr;
    if(!Base.Design() || !System)return NAME_None;
    const bool Carrier=(uint32(Base.GetClassification())&uint32(CLASSIFICATION::CARRIER))!=0;
    const FString Identity=Carrier?FString(TEXT("Carrier/"))+UTF8_TO_TCHAR(Base.GetName()):
        FString(TEXT("Station/"))+UTF8_TO_TCHAR(System->GetName())+TEXT("/")+UTF8_TO_TCHAR(Base.GetName());
    return FName(*(TEXT("Ordnance/")+Identity));
}
FName OrdnanceMagazine::Ensure(FEconomyState& State,const Ship& Base,FText& Reason)
{
    const auto& Config=Policy();const FName MagazineKey=Key(Base);auto* Region=Base.GetRegion();const auto* System=Region?Region->GetSystem():nullptr;
    if(!Config.Valid || MagazineKey.IsNone() || !System){Reason=INVTEXT("The local magazine definition is unavailable.");return NAME_None;}
    const bool Carrier=(uint32(Base.GetClassification())&uint32(CLASSIFICATION::CARRIER))!=0;
    if(State.Inventories.Contains(MagazineKey))return MagazineKey;
    if(State.InitializedMarkets.Contains(MagazineKey)){Reason=INVTEXT("The persistent magazine record is missing; automatic refilling is blocked.");return NAME_None;}
    const auto Before=State;FScopedSimDataChangeBatch Batch(State);
    const int32 Count=Config.Overrides.Contains(FName(UTF8_TO_TCHAR(Base.Design()->name)))?
        Config.Overrides.FindChecked(FName(UTF8_TO_TCHAR(Base.Design()->name))):(Carrier?Config.Carrier:Config.Station);
    if(!FEconomyLedger::CreateInventory(State,MagazineKey,FEconomyRuntime::IndustryOwner(),FName(UTF8_TO_TCHAR(System->GetName())),Carrier?Config.CarrierCapacity:Config.StationCapacity))return NAME_None;
    if(Count>0)for(FName Item:Config.Items)if(!FEconomyLedger::Deposit(State,MagazineKey,Item,Count,Policy().UnitTonnes))
    {State=Before;Reason=INVTEXT("Magazine creation failed; stock was restored.");return NAME_None;}
    State.InitializedMarkets.Add(MagazineKey);Reason=FText::GetEmpty();return MagazineKey;
}

TMap<FName,int32> OrdnanceMagazine::Loaded(Ship& Craft)
{
    TMap<FName,int32> Counts;
    for(ListIter<WeaponGroup> Groups=Craft.GetWeapons();++Groups;)
        for(ListIter<Weapon> Weapons=Groups->GetWeapons();++Weapons;)
            if(Weapons->Design() && !Weapons->IsPrimary() && Weapons->Ammo()>=0)Add(Counts,ItemId(*Weapons->Design()),Weapons->Ammo());
    return Counts;
}

bool OrdnanceMagazine::Required(Ship& Craft,const TArray<int32>& Choices,TMap<FName,int32>& Out,FText& Reason)
{
    Out.Reset();const auto* Design=Craft.Design();
    if(!Design || Choices.Num()!=Design->hardpoints.size()){Reason=INVTEXT("The requested hardpoints do not match this hull.");return false;}
    for(int32 Index=0;Index<Design->weapons.size();++Index)
        if(const auto* Weapon=Design->weapons[Index];Weapon && Weapon->Design() && !Weapon->IsPrimary() && Weapon->Ammo()>=0)
            if(!Add(Out,ItemId(*Weapon->Design()),Weapon->Ammo()))return false;
    for(int32 Index=0;Index<Choices.Num();++Index)
    {
        if(Choices[Index]==-1)continue;
        if(!Design->hardpoints[Index])return false;
        TUniquePtr<Weapon> Weapon(Design->hardpoints[Index]->CreateWeapon(Choices[Index]));
        if(!Weapon || !Weapon->Design() || Weapon->IsPrimary() || Weapon->Ammo()<0 || !Add(Out,ItemId(*Weapon->Design()),Weapon->Ammo()))
        {Reason=INVTEXT("A selected store is incompatible with this hardpoint.");return false;}
    }
    return true;
}

bool OrdnanceMagazine::Exchange(FS_PlayerGameInfo& Pilot,const FString& Dock,FName Magazine,
    const TMap<FName,int32>& Current,const TMap<FName,int32>& Wanted,FText& Reason)
{
    if(Dock.IsEmpty()){Reason=INVTEXT("Dock before changing missile stores.");return false;}
    const auto* Source=Magazine.IsNone()?nullptr:Pilot.Economy.Inventories.Find(Magazine);
    if(!Magazine.IsNone() && (!Source || Source->Owner!=FEconomyRuntime::IndustryOwner()))return false;
    for(const auto& Pair:Current)if(Pair.Value<0 || Pair.Value>1000000 || Pair.Key.IsNone())return false;
    for(const auto& Pair:Wanted)
    {
        const double Available=Source?FEconomyLedger::Available(Pilot.Economy,Magazine,Pair.Key):ShipStoreAI::Stored(Pilot,Pair.Key,Dock);
        if(Pair.Key.IsNone() || Pair.Value<0 || Pair.Value>1000000 || double(Pair.Value)>Available+Current.FindRef(Pair.Key))
        {Reason=FText::Format(INVTEXT("INSUFFICIENT STORES: {0}. Required {1}; loaded + local stock {2}."),FText::FromName(Pair.Key),FText::AsNumber(Pair.Value),FText::AsNumber(Available+Current.FindRef(Pair.Key)));return false;}
    }
    const auto Before=Pilot;FScopedSimDataChangeBatch Batch(Pilot.Economy);
    for(const auto& Pair:Current)
    {
        const int32 Return=FMath::Max(0,Pair.Value-Wanted.FindRef(Pair.Key));if(!Return)continue;
        if(Source)
        {if(!FEconomyLedger::Deposit(Pilot.Economy,Magazine,Pair.Key,Return,Policy().UnitTonnes)){Pilot=Before;return false;}}
        else
        {
            auto& Row=Pilot.StoredShipEquipment.AddDefaulted_GetRef();Row.CatalogId=Pair.Key;Row.Location=Dock;Row.Quantity=Return;
            Row.Category=Pair.Key.ToString().StartsWith(TEXT("Drone/"))?EShipStoreCategory::Drones:EShipStoreCategory::Missiles;
        }
    }
    for(const auto& Pair:Wanted)
    {
        int32 Count=FMath::Max(0,Pair.Value-Current.FindRef(Pair.Key));if(!Count)continue;
        if(Source)
        {if(!FEconomyLedger::Withdraw(Pilot.Economy,FEconomyRuntime::IndustryOwner(),Magazine,Pair.Key,Count)){Pilot=Before;return false;}}
        else for(auto& Row:Pilot.StoredShipEquipment)
            if(Row.CatalogId==Pair.Key && Row.Location.Equals(Dock,ESearchCase::IgnoreCase))
            {const int32 Take=FMath::Min(FMath::Max(0,Row.Quantity),Count);Row.Quantity-=Take;Count-=Take;}
    }
    Reason=Magazine.IsNone()?INVTEXT("Loadout supplied from player missile stores at this dock."):INVTEXT("Loadout supplied from the base magazine; unused stores returned there.");return true;
}

int32 OrdnanceMagazine::ReplenishCount(FEconomyState& State,FName Magazine,FName Item,int32 Current,int32 Capacity)
{
    if(Current<0 || Capacity<0 || Capacity>1000000 || Current>Capacity)return Current;
    const auto* Store=State.Inventories.Find(Magazine);
    if(!Store || Store->Owner!=FEconomyRuntime::IndustryOwner())return Current;
    const int32 Take=int32(FMath::Min(double(Capacity-Current),FEconomyLedger::Available(State,Magazine,Item)));
    return Take>0 && FEconomyLedger::Withdraw(State,Store->Owner,Magazine,Item,Take)?Current+Take:Current;
}

void OrdnanceMagazine::ReplenishAI(Ship& Base,Ship& Craft)
{
    auto* Simulation=Sim::GetSim();if(!Simulation || !Simulation->HasSimulationAuthority() || &Craft==Simulation->GetPlayerShip())return;
    auto& State=Simulation->GetEconomy();FScopedSimDataChangeBatch Batch(State);FText Reason;
    const FName Magazine=Ensure(State,Base,Reason);TMap<FName,int32> Wanted;
    if(!Required(Craft,Stations(Craft),Wanted,Reason)){SetFromPool(Craft,{});Craft.SetMagazineIssue(true);return;}
    const auto Existing=Craft.HasMagazineIssue()?Loaded(Craft):TMap<FName,int32>();
    auto Supply=Existing;
    for(const auto& Pair:Wanted)
    {
        Supply.FindOrAdd(Pair.Key)=ReplenishCount(State,Magazine,Pair.Key,Existing.FindRef(Pair.Key),Pair.Value);
    }
    SetFromPool(Craft,Supply);Craft.SetMagazineIssue(true);
}

void OrdnanceMagazine::ReturnAI(Ship& Base,Ship& Craft)
{
    auto* Simulation=Sim::GetSim();if(!Simulation || !Simulation->HasSimulationAuthority() || &Craft==Simulation->GetPlayerShip() || !Craft.HasMagazineIssue() || Craft.IsDead() || Craft.IsDying())return;
    auto& State=Simulation->GetEconomy();FScopedSimDataChangeBatch Batch(State);FText Reason;
    const FName Magazine=Ensure(State,Base,Reason);if(Magazine.IsNone())return;
    const auto Before=State;
    for(const auto& Pair:Loaded(Craft))if(Pair.Value>0 && !FEconomyLedger::Deposit(State,Magazine,Pair.Key,Pair.Value,Policy().UnitTonnes)){State=Before;return;}
    SetFromPool(Craft,{});Craft.SetMagazineIssue(false);
}
