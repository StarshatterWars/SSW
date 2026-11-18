#include "MiningCrystalCatalog.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

const FMiningCrystalCatalog& FMiningCrystalCatalog::Get()
{
    static const FMiningCrystalCatalog Catalog=[]()
    {
        FMiningCrystalCatalog Value; FString Json; FText Error;
        if(!FFileHelper::LoadFileToString(Json,*(FPaths::ProjectContentDir()/TEXT("GameData/Cargo/mining_crystals.json"))) || !Value.LoadJson(Json,Error))
        { UE_LOG(LogTemp,Warning,TEXT("[MiningCrystals] Catalog unavailable; neutral mining optics retained: %s"),*Error.ToString()); }
        else { UE_LOG(LogTemp,Display,TEXT("[MiningCrystals] Loaded %d SSW focusing crystal definitions."),Value.GetDefinitions().Num()); }
        return Value;
    }();
    return Catalog;
}
ESSWMineral FMiningCrystalCatalog::MineralFromName(FName Name)
{
    const int64 Value=StaticEnum<ESSWMineral>()->GetValueByNameString(Name.ToString());
    return Value==INDEX_NONE?ESSWMineral::None:static_cast<ESSWMineral>(Value);
}
bool FMiningCrystalCatalog::LoadJson(const FString& Json,FText& Error)
{
    TArray<TSharedPtr<FJsonValue>> Rows;
    if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Rows) || Rows.IsEmpty() || Rows.Num()>128)
    { Error=INVTEXT("Mining crystals require a nonempty JSON list.");return false; }
    TMap<FName,FMiningCrystalDefinition> Imported;
    for(const auto& Row:Rows)
    {
        const TSharedPtr<FJsonObject>* Object=nullptr; FMiningCrystalDefinition Def; FString Id;
        const TArray<TSharedPtr<FJsonValue>>* Minerals=nullptr;const TArray<TSharedPtr<FJsonValue>>* Color=nullptr;
        double Hz=0,Width=0;
        if(!Row.IsValid() || !Row->TryGetObject(Object) || !Object || !Object->IsValid() ||
            !(*Object)->TryGetStringField(TEXT("Id"),Id) || FName(*Id).IsNone() || Imported.Contains(FName(*Id)) ||
            !(*Object)->TryGetStringField(TEXT("DisplayName"),Def.DisplayName) || Def.DisplayName.IsEmpty() ||
            !(*Object)->TryGetStringField(TEXT("Description"),Def.Description) ||
            !(*Object)->TryGetBoolField(TEXT("Universal"),Def.bUniversal) ||
            !(*Object)->TryGetNumberField(TEXT("YieldMultiplier"),Def.YieldMultiplier) || !FMath::IsFinite(Def.YieldMultiplier) || Def.YieldMultiplier<1 || Def.YieldMultiplier>1.25 ||
            !(*Object)->TryGetNumberField(TEXT("PulseHz"),Hz) || !FMath::IsFinite(Hz) || Hz<.2 || Hz>4 ||
            !(*Object)->TryGetNumberField(TEXT("WidthScale"),Width) || !FMath::IsFinite(Width) || Width<.5 || Width>1.5 ||
            !(*Object)->TryGetArrayField(TEXT("Minerals"),Minerals) || !Minerals ||
            !(*Object)->TryGetArrayField(TEXT("Color"),Color) || !Color || Color->Num()!=3)
        { Error=INVTEXT("Invalid, duplicate or out-of-bounds focusing crystal definition.");return false; }
        Def.Id=FName(*Id);Def.PulseHz=Hz;Def.WidthScale=Width;
        TArray<float> Channels;
        for(const auto& Channel:*Color)
        {
            double Value=0;
            if(!Channel.IsValid() || !Channel->TryGetNumber(Value) || !FMath::IsFinite(Value) || Value<0 || Value>8)
            { Error=INVTEXT("Crystal color must contain three finite HDR values from zero to eight.");return false; }
            Channels.Add(Value);
        }
        Def.Color=FLinearColor(Channels[0],Channels[1],Channels[2],1);
        for(const auto& Entry:*Minerals)
        {
            FString Name;
            if(!Entry.IsValid() || !Entry->TryGetString(Name))return false;
            const auto Mineral=MineralFromName(FName(*Name));
            if(Mineral==ESSWMineral::None || Def.Minerals.Contains(Mineral))
            { Error=INVTEXT("Crystal mineral names must resolve to unique SSW resources.");return false; }
            Def.Minerals.Add(Mineral);
        }
        if(!Def.bUniversal && Def.Minerals.IsEmpty())
        { Error=INVTEXT("A specialized crystal must specify its supported minerals.");return false; }
        Imported.Add(Def.Id,MoveTemp(Def));
    }
    Crystals=MoveTemp(Imported);Error=FText::GetEmpty();return true;
}
double FMiningCrystalCatalog::ExtractionMultiplier(FName Id,ESSWMineral Mineral) const
{
    const auto* Crystal=Find(Id);
    return Crystal && Crystal->Supports(Mineral)?Crystal->YieldMultiplier:1.0;
}
const FMiningCrystalDefinition* FMiningCrystalCatalog::Appearance(FName Id,ESSWMineral Mineral) const
{
    if(const auto* Equipped=Find(Id);Equipped && Equipped->Supports(Mineral))return Equipped;
    // Stable ID ordering keeps visual selection deterministic if definitions overlap.
    const FMiningCrystalDefinition* Best=nullptr;
    for(const auto& Pair:Crystals)
        if(!Pair.Value.bUniversal && Pair.Value.Supports(Mineral) && (!Best || Pair.Key.LexicalLess(Best->Id)))Best=&Pair.Value;
    return Best?Best:Find(TEXT("StandardFocus"));
}
float FMiningCrystalCatalog::Pulse(float Elapsed,float Hz)
{
    if(!FMath::IsFinite(Elapsed) || !FMath::IsFinite(Hz))return 1;
    return 1.f+.12f*FMath::Sin(Elapsed*FMath::Clamp(Hz,.2f,4.f)*2.f*PI);
}
