#include "MiningCrystalEquipment.h"
#include "GameStructs_economy.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
struct FMagazine { TArray<FName> Crystals; double ServiceLifeTonnes=3000; };
const TMap<FName,FMagazine>& Magazines()
{
    static const TMap<FName,FMagazine> Values=[]()
    {
        TMap<FName,FMagazine> Imported;FString Json;TArray<TSharedPtr<FJsonValue>> Rows;
        if(!FFileHelper::LoadFileToString(Json,*(FPaths::ProjectContentDir()/TEXT("GameData/Cargo/mining_fittings.json"))) ||
            !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Rows))return Imported;
        for(const auto& Row:Rows)
        {
            const TSharedPtr<FJsonObject>* Object=nullptr;FString Hull;FMagazine Magazine;
            const TArray<TSharedPtr<FJsonValue>>* Crystals=nullptr;
            if(!Row.IsValid() || !Row->TryGetObject(Object) || !Object || !Object->IsValid() ||
                !(*Object)->TryGetStringField(TEXT("ShipDesign"),Hull) || FName(*Hull).IsNone() || Imported.Contains(FName(*Hull)) ||
                !(*Object)->TryGetNumberField(TEXT("ServiceLifeTonnes"),Magazine.ServiceLifeTonnes) ||
                !FMath::IsFinite(Magazine.ServiceLifeTonnes) || Magazine.ServiceLifeTonnes<=0 || Magazine.ServiceLifeTonnes>1.e9 ||
                !(*Object)->TryGetArrayField(TEXT("Crystals"),Crystals) || !Crystals || Crystals->IsEmpty())return TMap<FName,FMagazine>();
            for(const auto& Entry:*Crystals)
            {
                FString Id;
                if(!Entry.IsValid() || !Entry->TryGetString(Id) || !FMiningCrystalCatalog::Get().Find(FName(*Id)) || Magazine.Crystals.Contains(FName(*Id)))return TMap<FName,FMagazine>();
                Magazine.Crystals.Add(FName(*Id));
            }
            Imported.Add(FName(*Hull),MoveTemp(Magazine));
        }
        return Imported;
    }();
    return Values;
}
}
FName FMiningCrystalService::EquipmentKey(FName Hull,FName Crystal)
{ return FName(*(Hull.ToString()+TEXT("/Optics/")+Crystal.ToString())); }
FName FMiningCrystalService::FitConfigured(FEconomyState& State,FName Hull,FName ShipDesign,ESSWMineral Mineral)
{
    if(Hull.IsNone())return NAME_None;
    const auto* Magazine=Magazines().Find(ShipDesign);
    if(!Magazine)return NAME_None;
    const FMiningCrystalDefinition* Best=nullptr;
    for(const auto Id:Magazine->Crystals)
        if(const auto* Crystal=FMiningCrystalCatalog::Get().Find(Id);Crystal && Crystal->Supports(Mineral) &&
            (!Best || Crystal->YieldMultiplier>Best->YieldMultiplier ||
            (Crystal->YieldMultiplier==Best->YieldMultiplier && Crystal->Id.LexicalLess(Best->Id))))Best=Crystal;
    if(!Best)return NAME_None;
    const FName Key=EquipmentKey(Hull,Best->Id);
    auto* Existing=State.MiningOptics.Find(Key);
    if(Existing && Existing->Crystal!=Best->Id)return NAME_None;
    if(!Existing)
    {
        FMiningCrystalEquipment Equipment;Equipment.Crystal=Best->Id;Equipment.FittedFamily=ShipDesign;Equipment.ServiceLifeTonnes=Magazine->ServiceLifeTonnes;
        State.MiningOptics.Add(Key,Equipment);
    }
    const FString Prefix=Hull.ToString()+TEXT("/Optics/");
    for(auto& Pair:State.MiningOptics)
        if(Pair.Key.ToString().StartsWith(Prefix))Pair.Value.bSelected=Pair.Key==Key;
    return Best->Id;
}
double FMiningCrystalService::Multiplier(const FEconomyState& State,FName Hull,FName Crystal,ESSWMineral Mineral)
{
    const auto* Equipment=State.MiningOptics.Find(EquipmentKey(Hull,Crystal));
    if(!Equipment || Equipment->Crystal!=Crystal || !FMath::IsFinite(Equipment->ServiceLifeTonnes) || Equipment->ServiceLifeTonnes<=0 ||
        !FMath::IsFinite(Equipment->ExtractedTonnes) || Equipment->ExtractedTonnes<0)return 1;
    const double Life=1-FMath::Clamp(Equipment->ExtractedTonnes/Equipment->ServiceLifeTonnes,0.0,1.0);
    return 1+(FMiningCrystalCatalog::Get().ExtractionMultiplier(Crystal,Mineral)-1)*Life;
}
void FMiningCrystalService::RecordExtraction(FEconomyState& State,FName Hull,FName Crystal,double AcceptedTonnes)
{
    auto* Equipment=State.MiningOptics.Find(EquipmentKey(Hull,Crystal));
    if(!Equipment || !FMath::IsFinite(AcceptedTonnes) || AcceptedTonnes<=0 || !FMath::IsFinite(Equipment->ExtractedTonnes) ||
        !FMath::IsFinite(Equipment->ServiceLifeTonnes) || Equipment->ServiceLifeTonnes<=0)return;
    Equipment->ExtractedTonnes=FMath::Clamp(Equipment->ExtractedTonnes+AcceptedTonnes,0.0,Equipment->ServiceLifeTonnes);
}
FText FMiningCrystalService::Summary(const FEconomyState& State,FName Hull)
{
    const FString Prefix=Hull.ToString()+TEXT("/Optics/");const FMiningCrystalEquipment* Latest=nullptr;
    for(const auto& Pair:State.MiningOptics)
        if(Pair.Key.ToString().StartsWith(Prefix) && Pair.Value.bSelected) {Latest=&Pair.Value;break;}
    if(!Latest)return FText::GetEmpty();
    const auto* Definition=FMiningCrystalCatalog::Get().Find(Latest->Crystal);
    const double Condition=Latest->ServiceLifeTonnes>0 && FMath::IsFinite(Latest->ExtractedTonnes)?
        100*(1-FMath::Clamp(Latest->ExtractedTonnes/Latest->ServiceLifeTonnes,0.0,1.0)):0;
    const FString ShortName=(Definition?Definition->Id.ToString():Latest->Crystal.ToString()).Replace(TEXT("Focus"),TEXT(""));
    return FText::Format(INVTEXT("FOCUS {0} {1}%"),FText::FromString(ShortName),FText::AsNumber(FMath::RoundToInt(Condition)));
}
