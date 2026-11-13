#include "FreightManifest.h"

#include "Game.h"
#include "Instruction.h"
#include "Reader.h"
#include "SimRegion.h"
#include "StarSystem.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Parser.h"
#include "ParseUtil.h"
#include "Term.h"

void SSWFreightManifest::ReadCapacity(FSSWCargoCapacity& Capacity, TermDef& Definition, const char* Filename)
{
    TermStruct* Fields = Definition.term()->isStruct();
    if (!Fields) return;

    for (ListIter<Term> It = *Fields->elements(); ++It;)
    {
        TermDef* Field = It->isDef();
        if (!Field) continue;

        const Text& Key = Field->name()->value();
        if (Key == "tonnes") GetDefNumber(Capacity.Tonnes, Field, Filename);
        else if (Key == "load_tonnes_per_second") GetDefNumber(Capacity.LoadTonnesPerSecond, Field, Filename);
        else if (Key == "unload_tonnes_per_second") GetDefNumber(Capacity.UnloadTonnesPerSecond, Field, Filename);
        else if (Key == "passengers") GetDefNumber(Capacity.PassengerBerths, Field, Filename);
    }
    Capacity.Tonnes = FMath::IsFinite(Capacity.Tonnes) ? FMath::Max(0.0, Capacity.Tonnes) : 0.0;
    Capacity.LoadTonnesPerSecond = FMath::IsFinite(Capacity.LoadTonnesPerSecond) ? FMath::Max(0.0, Capacity.LoadTonnesPerSecond) : 0.0;
    Capacity.UnloadTonnesPerSecond = FMath::IsFinite(Capacity.UnloadTonnesPerSecond) ? FMath::Max(0.0, Capacity.UnloadTonnesPerSecond) : 0.0;
    Capacity.PassengerBerths = FMath::Max(0, Capacity.PassengerBerths);
}

void SSWFreightManifest::AddOption(TArray<FSSWCargoOption>& Options, TermDef& Definition, const char* Filename)
{
    TermStruct* Fields = Definition.term()->isStruct();
    if (!Fields) return;

    Text Item;
    Text Unit;
    Text FinishedItem;
    FSSWCargoOption Option;
    for (ListIter<Term> It = *Fields->elements(); ++It;)
    {
        TermDef* Field = It->isDef();
        if (!Field) continue;

        const Text& Key = Field->name()->value();
        if (Key == "item") GetDefText(Item, Field, Filename);
        else if (Key == "unit") GetDefText(Unit, Field, Filename);
        else if (Key == "finished_item") GetDefText(FinishedItem, Field, Filename);
        else if (Key == "passengers") GetDefBool(Option.bPassengers, Field, Filename);
        else if (Key == "min") GetDefNumber(Option.Minimum, Field, Filename);
        else if (Key == "max") GetDefNumber(Option.Maximum, Field, Filename);
    }
    if (!Item.length() || Option.Minimum <= 0 || Option.Maximum < Option.Minimum) return;

    if (FinishedItem.length())
    {
        const int64 Value = StaticEnum<ESSWFinishedItem>()->GetValueByNameString(UTF8_TO_TCHAR(FinishedItem.data()));
        if (Value == INDEX_NONE)
        {
            UE_LOG(LogTemp, Warning, TEXT("Unknown finished item '%hs' in %hs"), FinishedItem.data(), Filename);
            return;
        }
        Option.FinishedItem = static_cast<ESSWFinishedItem>(Value);
    }

    Option.Item = UTF8_TO_TCHAR(Item.data());
    Option.Unit = UTF8_TO_TCHAR(Unit.data());
    Options.Add(MoveTemp(Option));
}

SSWFreightManifest::FProfile SSWFreightManifest::LoadProfile(const FString& DefinitionName)
{
    FProfile Profile;
    const FString Path = FPaths::ProjectContentDir() / TEXT("GameData/Ships") / (DefinitionName + TEXT(".def"));
    TArray<uint8> Bytes;
    if (!FFileHelper::LoadFileToArray(Bytes, *Path, FILEREAD_Silent)) return Profile;

    Bytes.Add(0);
    const FTCHARToUTF8 Filename(*Path);
    Parser Reader(new BlockReader(reinterpret_cast<const char*>(Bytes.GetData())));
    for (TUniquePtr<Term> Term(Reader.ParseTerm()); Term; Term.Reset(Reader.ParseTerm()))
    {
        TermDef* Def = Term->isDef();
        if (!Def) continue;

        const Text& Key = Def->name()->value();
        if (Key == "cargo_capacity") ReadCapacity(Profile.Capacity, *Def, Filename.Get());
        else if (Key == "cargo_manifest") AddOption(Profile.Options, *Def, Filename.Get());
    }
    return Profile;
}

TOptional<SSWFreightManifest::FLoad> SSWFreightManifest::Assign(
    const TArray<FSSWCargoOption>& Options, const FSSWCargoCapacity& Capacity,
    const char* ShipName, const char* Registry)
{
    struct FAvailableLoad
    {
        const FSSWCargoOption* Option;
        int32 Maximum;
    };
    TArray<FAvailableLoad, TInlineAllocator<8>> Available;
    for (const FSSWCargoOption& Option : Options)
    {
        int32 Maximum = Option.Maximum;
        if (Option.bPassengers)
            Maximum = FMath::Min(Maximum, Capacity.PassengerBerths);
        else if (Option.Unit == TEXT("t"))
        {
            if (!FMath::IsFinite(Capacity.Tonnes) || Maximum < 1)
                continue;
            Maximum = FMath::FloorToInt32(FMath::Clamp(Capacity.Tonnes, 0.0, static_cast<double>(Maximum)));
        }
        else if (Capacity.Tonnes <= 0)
            continue;

        if (!Option.Item.IsEmpty() && Option.Minimum > 0 && Maximum >= Option.Minimum)
            Available.Add({&Option, Maximum});
    }
    if (Available.IsEmpty()) return {};

    const uint32 NameSeed = FCrc::StrCrc32(ShipName);
    FRandomStream Random(static_cast<int32>(FCrc::StrCrc32(Registry, NameSeed)));
    const FAvailableLoad& Selected = Available[Random.RandRange(0, Available.Num() - 1)];
    const FSSWCargoOption& Option = *Selected.Option;
    FLoad Load;
    Load.Item = Option.Item;
    Load.Unit = Option.Unit;
    Load.Quantity = Random.RandRange(Option.Minimum, Selected.Maximum);
    Load.FinishedItem = Option.FinishedItem;
    Load.bPassengers = Option.bPassengers;
    return Load;
}

void SSWFreightManifest::FLoad::CaptureOrigin(SimRegion& Region, const FVector& Position)
{
    if (Origin.IsSet()) return;

    FEndpoint Endpoint;
    if (const StarSystem* System = Region.GetSystem())
        Endpoint.System = UTF8_TO_TCHAR(System->GetName());
    Endpoint.Sector = UTF8_TO_TCHAR(Region.GetName());
    Endpoint.RegionPosition = Position;
    Origin = MoveTemp(Endpoint);
}

void SSWFreightManifest::FLoad::CaptureDestination(Instruction& LastWaypoint)
{
    if (Destination.IsSet() || !LastWaypoint.GetRegionName()[0]) return;

    FEndpoint Endpoint;
    if (SimRegion* Region = LastWaypoint.GetRegion())
        if (const StarSystem* System = Region->GetSystem())
            Endpoint.System = UTF8_TO_TCHAR(System->GetName());
    Endpoint.Sector = UTF8_TO_TCHAR(LastWaypoint.GetRegionName());
    Endpoint.Site = UTF8_TO_TCHAR(LastWaypoint.GetTargetName());
    // Read the authored position without resolving or modifying the live route.
    Endpoint.RegionPosition = OtherHand(LastWaypoint.GetRLoc().BaseLocation());
    Destination = MoveTemp(Endpoint);
}

bool SSWFreightManifest::CanResolve(double DistanceMetres, double SensorRangeMetres)
{
    constexpr double MaxScanDistanceMetres = 25000.0;
    constexpr double ScanRangeFraction = 0.25;
    return FMath::IsFinite(DistanceMetres) && DistanceMetres >= 0.0 &&
        FMath::IsFinite(SensorRangeMetres) && SensorRangeMetres > 0.0 &&
        DistanceMetres <= FMath::Min(MaxScanDistanceMetres, SensorRangeMetres * ScanRangeFraction);
}
