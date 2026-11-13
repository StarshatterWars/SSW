#include "ShipVisualCatalog.h"
#include "ShipActor.h"
#include "Misc/PackageName.h"
namespace {
const ShipVisualCatalog::FEntry Definitions[] = {
    { TEXT("Barracks"), TEXT("Barracks"), TEXT("Barracks"), TEXT("BarracksBuildingActor"), TEXT("building") },
    { TEXT("BioDome"), TEXT("BioDome"), TEXT("A1-0"), TEXT("BioDomeBuildingActor"), TEXT("building") },
    { TEXT("BioDome2"), TEXT("BioDome2"), TEXT("A3-0"), TEXT("BioDome2BuildingActor"), TEXT("building") },
    { TEXT("BioTrail"), TEXT("BioTrail"), TEXT("A2-0"), TEXT("BioTrailBuildingActor"), TEXT("building") },
    { TEXT("Bldg00"), TEXT("Bldg00"), TEXT("Bldg00-0"), TEXT("Bldg00BuildingActor"), TEXT("building") },
    { TEXT("Bldg01"), TEXT("Bldg01"), TEXT("Bldg01-0.mag"), TEXT("Bldg01BuildingActor"), TEXT("building") },
    { TEXT("Bldg02"), TEXT("Bldg02"), TEXT("Bldg02-0"), TEXT("Bldg02BuildingActor"), TEXT("building") },
    { TEXT("Bldg03"), TEXT("Bldg03"), TEXT("Bldg03-0"), TEXT("Bldg03BuildingActor"), TEXT("building") },
    { TEXT("Bldg04"), TEXT("Bldg04"), TEXT("Bldg04-0"), TEXT("Bldg04BuildingActor"), TEXT("building") },
    { TEXT("Bldg05"), TEXT("Bldg05"), TEXT("Bldg05-0"), TEXT("Bldg05BuildingActor"), TEXT("building") },
    { TEXT("City00"), TEXT("City00"), TEXT("City00"), TEXT("City00BuildingActor"), TEXT("building") },
    { TEXT("City01"), TEXT("City01"), TEXT("City01"), TEXT("City01BuildingActor"), TEXT("building") },
    { TEXT("City02"), TEXT("City02"), TEXT("City02"), TEXT("City02BuildingActor"), TEXT("building") },
    { TEXT("City03"), TEXT("City03"), TEXT("Vesta02"), TEXT("City03BuildingActor"), TEXT("building") },
    { TEXT("Cool00"), TEXT("Cool00"), TEXT("Cool00"), TEXT("Cool00BuildingActor"), TEXT("building") },
    { TEXT("Dsrt00"), TEXT("Dsrt00"), TEXT("Dsrt00"), TEXT("Dsrt00BuildingActor"), TEXT("building") },
    { TEXT("Dsrt01"), TEXT("Dsrt01"), TEXT("Dsrt01"), TEXT("Dsrt01BuildingActor"), TEXT("building") },
    { TEXT("Dsrt02"), TEXT("Dsrt02"), TEXT("Dsrt02"), TEXT("Dsrt02BuildingActor"), TEXT("building") },
    { TEXT("Dsrt03"), TEXT("Dsrt03"), TEXT("Dsrt03"), TEXT("Dsrt03BuildingActor"), TEXT("building") },
    { TEXT("Dsrt04"), TEXT("Dsrt04"), TEXT("Dsrt04"), TEXT("Dsrt04BuildingActor"), TEXT("building") },
    { TEXT("Dsrt05"), TEXT("Dsrt05"), TEXT("Dsrt05"), TEXT("Dsrt05BuildingActor"), TEXT("building") },
    { TEXT("Fact00"), TEXT("Fact00"), TEXT("Fact00"), TEXT("Fact00BuildingActor"), TEXT("building") },
    { TEXT("Fact01"), TEXT("Fact01"), TEXT("Fact01"), TEXT("Fact01BuildingActor"), TEXT("building") },
    { TEXT("Fact02"), TEXT("Fact02"), TEXT("Fact02"), TEXT("Fact02BuildingActor"), TEXT("building") },
    { TEXT("Fact03"), TEXT("Fact03"), TEXT("Fact03"), TEXT("Fact03BuildingActor"), TEXT("building") },
    { TEXT("Fact04"), TEXT("Fact04"), TEXT("Fact04"), TEXT("Fact04BuildingActor"), TEXT("building") },
    { TEXT("Gnd_aaa1"), TEXT("AAA 1"), TEXT("Tower3"), TEXT("Gnd_aaa1ShipActor"), TEXT("sam") },
    { TEXT("Gnd_aaa2"), TEXT("AAA 2"), TEXT("Tower3"), TEXT("Gnd_aaa2ShipActor"), TEXT("sam") },
    { TEXT("Gnd_sam1"), TEXT("SAM 1"), TEXT("SAMBase"), TEXT("Gnd_sam1ShipActor"), TEXT("sam") },
    { TEXT("Platform"), TEXT("Platform"), TEXT("Platform"), TEXT("PlatformBuildingActor"), TEXT("building") },
    { TEXT("Reac01"), TEXT("Reac01"), TEXT("Reactor"), TEXT("Reac01BuildingActor"), TEXT("building") },
    { TEXT("Senate"), TEXT("Senate"), TEXT("senate2"), TEXT("SenateBuildingActor"), TEXT("building") },
    { TEXT("Tank00"), TEXT("Tank00"), TEXT("Tank00"), TEXT("Tank00BuildingActor"), TEXT("building") },
    { TEXT("Adventure"), TEXT("Adventure"), TEXT("Adventure"), TEXT("AdventureShipActor"), TEXT("carrier") },
    { TEXT("Archaelith"), TEXT("Archaelith"), TEXT("Archaelith"), TEXT("ArchaelithShipActor"), TEXT("drone") },
    { TEXT("Asher"), TEXT("Asher"), TEXT("Asher"), TEXT("AsherShipActor"), TEXT("destroyer") },
    { TEXT("Avenger"), TEXT("Avenger"), TEXT("Avenger"), TEXT("AvengerShipActor"), TEXT("attack") },
    { TEXT("Baikal"), TEXT("Baikal"), TEXT("Baikal"), TEXT("BaikalShipActor"), TEXT("frigate") },
    { TEXT("Berents"), TEXT("Berents"), TEXT("Berents"), TEXT("BerentsShipActor"), TEXT("frigate") },
    { TEXT("Broadsword"), TEXT("Broadsword"), TEXT("Broadsword"), TEXT("BroadswordShipActor"), TEXT("destroyer") },
    { TEXT("CameraPod"), TEXT("CameraPod"), TEXT("CameraPod"), TEXT("CameraPodShipActor"), TEXT("drone") },
    { TEXT("CargoA"), TEXT("IM3500"), TEXT("IM3500"), TEXT("CargoAShipActor"), TEXT("freighter") },
    { TEXT("CargoB"), TEXT("IM4300"), TEXT("IM4300"), TEXT("CargoBShipActor"), TEXT("freighter") },
    { TEXT("CargoC"), TEXT("IM2800"), TEXT("IM2800"), TEXT("CargoCShipActor"), TEXT("cargo") },
    { TEXT("Cobra"), TEXT("Cobra"), TEXT("Cobra"), TEXT("CobraShipActor"), TEXT("attack") },
    { TEXT("Colony"), TEXT("Colony"), TEXT("Colony"), TEXT("ColonyShipActor"), TEXT("station") },
    { TEXT("Courageous"), TEXT("Courageous"), TEXT("Courageous"), TEXT("CourageousShipActor"), TEXT("cruiser") },
    { TEXT("Courier"), TEXT("Courier"), TEXT("Courier"), TEXT("CourierShipActor"), TEXT("courier") },
    { TEXT("Cutlass"), TEXT("Cutlass"), TEXT("Cutlass"), TEXT("CutlassShipActor"), TEXT("fighter") },
    { TEXT("Devastator"), TEXT("Devastator"), TEXT("Devastator"), TEXT("DevastatorShipActor"), TEXT("cruiser") },
    { TEXT("Dragon"), TEXT("Dragon"), TEXT("Dragon"), TEXT("DragonShipActor"), TEXT("carrier") },
    { TEXT("Eagle"), TEXT("Eagle"), TEXT("Eagle"), TEXT("EagleShipActor"), TEXT("fighter") },
    { TEXT("Falcon"), TEXT("Falcon"), TEXT("Falcon"), TEXT("FalconShipActor"), TEXT("fighter") },
    { TEXT("Farcaster"), TEXT("Farcaster"), TEXT("Farcaster"), TEXT("FarcasterShipActor"), TEXT("farcaster") },
    { TEXT("Farcaster2"), TEXT("Marakan"), TEXT("Farcaster2"), TEXT("Farcaster2ShipActor"), TEXT("farcaster") },
    { TEXT("FoothillBase"), TEXT("Airbase2"), TEXT("Base2"), TEXT("FoothillBaseShipActor"), TEXT("starbase") },
    { TEXT("Fortress"), TEXT("Station3"), TEXT("Fortress"), TEXT("FortressShipActor"), TEXT("station") },
    { TEXT("MiningStation"), TEXT("Mining Station"), TEXT("Xfer"), TEXT("MiningStationActor"), TEXT("station") },
    { TEXT("AsteroidBase"), TEXT("Asteroid Base"), TEXT("Xfer"), TEXT("AsteroidBaseActor"), TEXT("station") },
    { TEXT("FreightXfer"), TEXT("Station1"), TEXT("Xfer"), TEXT("FreightXferShipActor"), TEXT("station") },
    { TEXT("Goliath"), TEXT("Goliath"), TEXT("Goliath"), TEXT("GoliathShipActor"), TEXT("carrier") },
    { TEXT("Harmony"), TEXT("Station2"), TEXT("Harmony"), TEXT("HarmonyShipActor"), TEXT("station") },
    { TEXT("Hulk-Broadsword"), TEXT("Hulk-Broadsword"), TEXT("Hulk-Broadsword"), TEXT("HulkBroadswordShipActor"), TEXT("hulk") },
    { TEXT("Hulk-Dragon"), TEXT("Hulk-Dragon"), TEXT("Hulk-Dragon"), TEXT("HulkDragonShipActor"), TEXT("hulk") },
    { TEXT("Hulk-Imperial"), TEXT("Hulk-Imperial"), TEXT("Hulk-Imperial"), TEXT("HulkImperialShipActor"), TEXT("hulk") },
    { TEXT("Hulk-Tiger"), TEXT("Hulk-Tiger"), TEXT("Hulk-Frigate"), TEXT("HulkTigerShipActor"), TEXT("hulk") },
    { TEXT("Hulk-Vendetta"), TEXT("Hulk-Vendetta"), TEXT("Hulk-Vendetta"), TEXT("HulkVendettaShipActor"), TEXT("hulk") },
    { TEXT("Hulk-Volnaris"), TEXT("Hulk-Volnaris"), TEXT("Hulk-Volnaris"), TEXT("HulkVolnarisShipActor"), TEXT("hulk") },
    { TEXT("Imperial"), TEXT("Imperial"), TEXT("Imperial"), TEXT("ImperialShipActor"), TEXT("cruiser") },
    { TEXT("Intruder"), TEXT("Intruder"), TEXT("Intruder"), TEXT("IntruderShipActor"), TEXT("fighter") },
    { TEXT("JanekStation"), TEXT("Airbase1"), TEXT("Base1"), TEXT("JanekStationShipActor"), TEXT("starbase") },
    { TEXT("Marauder"), TEXT("Marauder"), TEXT("Marauder"), TEXT("MarauderShipActor"), TEXT("frigate") },
    { TEXT("Mine1"), TEXT("BlockadeMine"), TEXT("Mine"), TEXT("Mine1ShipActor"), TEXT("mine") },
    { TEXT("Observatory"), TEXT("Observatory"), TEXT("Observatory"), TEXT("ObservatoryShipActor"), TEXT("station") },
    { TEXT("Orion"), TEXT("Orion"), TEXT("Orion"), TEXT("OrionShipActor"), TEXT("carrier") },
    { TEXT("QShip"), TEXT("QShip"), TEXT("QShip"), TEXT("QShipShipActor"), TEXT("freighter") },
    { TEXT("RadixBase"), TEXT("Airbase2"), TEXT("Radix"), TEXT("RadixBaseShipActor"), TEXT("starbase") },
    { TEXT("Raptor"), TEXT("Raptor"), TEXT("Raptor"), TEXT("RaptorShipActor"), TEXT("fighter") },
    { TEXT("Razor"), TEXT("Razor"), TEXT("Razor"), TEXT("RazorShipActor"), TEXT("fighter") },
    { TEXT("Relay1"), TEXT("Relay1"), TEXT("Relay"), TEXT("Relay1ShipActor"), TEXT("comsat") },
    { TEXT("S4"), TEXT("Station4"), TEXT("S4"), TEXT("StationActor"), TEXT("station") },
    { TEXT("S5"), TEXT("Station5"), TEXT("S5"), TEXT("S5ShipActor"), TEXT("station") },
    { TEXT("S6"), TEXT("Station6"), TEXT("MarakanXfer"), TEXT("S6ShipActor"), TEXT("station") },
    { TEXT("S7"), TEXT("Station7"), TEXT("MarakanLab"), TEXT("S7ShipActor"), TEXT("station") },
    { TEXT("Saber"), TEXT("Saber"), TEXT("Saber"), TEXT("SaberShipActor"), TEXT("fighter") },
    { TEXT("Spectre"), TEXT("Spectre"), TEXT("Spectre"), TEXT("SpectreShipActor"), TEXT("destroyer") },
    { TEXT("Stormhawk"), TEXT("Stormhawk"), TEXT("Stormhawk"), TEXT("StormhawkShipActor"), TEXT("fighter") },
    { TEXT("TanisBase"), TEXT("Airbase3"), TEXT("Tanis"), TEXT("TanisBaseShipActor"), TEXT("starbase") },
    { TEXT("TankerA"), TEXT("IM1906"), TEXT("TankerA"), TEXT("TankerAShipActor"), TEXT("freighter") },
    { TEXT("TankerB"), TEXT("IM1912"), TEXT("TankerB"), TEXT("TankerBShipActor"), TEXT("freighter") },
    { TEXT("TargetDrone"), TEXT("TargetDrone"), TEXT("TargetDrone"), TEXT("TargetDroneShipActor"), TEXT("drone") },
    { TEXT("TelmeraStarport"), TEXT("Airbase1"), TEXT("Telmera"), TEXT("TelmeraStarportShipActor"), TEXT("starbase") },
    { TEXT("Thunderbolt"), TEXT("Thunderbolt"), TEXT("Thunderbolt"), TEXT("ThunderboltShipActor"), TEXT("attack") },
    { TEXT("Tiger"), TEXT("Tiger"), TEXT("Tiger"), TEXT("TigerShipActor"), TEXT("frigate") },
    { TEXT("TigerBayBase"), TEXT("Airbase2"), TEXT("TigerBay"), TEXT("TigerBayBaseShipActor"), TEXT("starbase") },
    { TEXT("Transport"), TEXT("Transport"), TEXT("Transport"), TEXT("TransportShipActor"), TEXT("courier") },
    { TEXT("Vendetta"), TEXT("Vendetta"), TEXT("Vendetta"), TEXT("VendettaShipActor"), TEXT("cruiser") },
    { TEXT("Viper"), TEXT("Viper"), TEXT("Viper"), TEXT("ViperShipActor"), TEXT("fighter") },
    { TEXT("Volnaris"), TEXT("Volnaris"), TEXT("Volnaris"), TEXT("VolnarisShipActor"), TEXT("destroyer") },
    { TEXT("Wolf"), TEXT("Wolf"), TEXT("Wolf"), TEXT("WolfShipActor"), TEXT("destroyer") },
    { TEXT("Jackal"), TEXT("Jackal"), TEXT("Jackal"), TEXT("JackalShipActor"), TEXT("attack") },
    { TEXT("Zodiac"), TEXT("Zodiac"), TEXT("Zodiac"), TEXT("ZodiacShipActor"), TEXT("fighter") },
    { TEXT("Prospector"), TEXT("Prospector"), TEXT("Zodiac"), TEXT("ZodiacShipActor"), TEXT("fighter") },
    { TEXT("Rockhopper"), TEXT("Rockhopper"), TEXT("IM3500"), TEXT("CargoAShipActor"), TEXT("freighter") },
    { TEXT("ZZA"), TEXT("ZZA"), TEXT("ZAttack"), TEXT("ZZAShipActor"), TEXT("attack") },
    { TEXT("ZZCA"), TEXT("ZZCA"), TEXT("ZolonCA"), TEXT("ZZCAShipActor"), TEXT("cruiser") },
    { TEXT("ZZCS"), TEXT("ZZCS"), TEXT("ZolonCS"), TEXT("ZZCSShipActor"), TEXT("cruiser") },
    { TEXT("ZZDD"), TEXT("ZZDD"), TEXT("ZZDD"), TEXT("ZZDDShipActor"), TEXT("destroyer") },
    { TEXT("ZZF"), TEXT("ZZF"), TEXT("ZFighter"), TEXT("ZZFShipActor"), TEXT("fighter") },
    { TEXT("ZZFF"), TEXT("ZZFF"), TEXT("ZolonFF"), TEXT("ZZFFShipActor"), TEXT("frigate") },
};
FString ClassPath(const FString& Name)
{
    const bool bGround = [&]() { for (const auto& E : Definitions)
        if (Name==E.Definition) return FString(E.Kind)==TEXT("building") || FString(E.Kind)==TEXT("sam"); return false; }();
    if(bGround) return FString::Printf(TEXT("/Game/Assets/Ground/%s/BP_%s.BP_%s_C"), *Name, *Name, *Name);
    return FString::Printf(TEXT("/Game/Models/%s/BP_%s.BP_%s_C"), *Name, *Name, *Name);
}
}
TConstArrayView<ShipVisualCatalog::FEntry> ShipVisualCatalog::Entries() { return MakeArrayView(Definitions); }
FString ShipVisualCatalog::DefinitionName(const FString& DesignName, const FString& ModelName)
{
    if(DesignName.Equals(TEXT("Marakan Farcaster"),ESearchCase::IgnoreCase))return TEXT("Farcaster2");
    for (const auto& Entry : Definitions)
        if (DesignName.Equals(Entry.Definition, ESearchCase::IgnoreCase)) return Entry.Definition;
    const FEntry* Match = nullptr;
    int32 Matches = 0;
    for (const auto& Entry : Definitions)
    {
        if (!DesignName.Equals(Entry.Design, ESearchCase::IgnoreCase)) continue;
        if (!ModelName.IsEmpty() && ModelName.Equals(Entry.Model, ESearchCase::IgnoreCase)) return Entry.Definition;
        Match = &Entry; ++Matches;
    }
    // Shared legacy identifiers such as Airbase2 need their model to disambiguate.
    return Matches == 1 ? FString(Match->Definition) : DesignName;
}
FString ShipVisualCatalog::CanonicalClassPath(const FString& DesignName, const FString& ModelName)
{
    const FString Definition = DefinitionName(DesignName, ModelName);
    if (Definition == TEXT("Prospector")) return ClassPath(TEXT("Zodiac"));
    if (Definition == TEXT("Rockhopper")) return ClassPath(TEXT("CargoA"));
    return ClassPath(Definition);
}
UClass* ShipVisualCatalog::Resolve(const FString& DesignName, const FString& ModelName)
{
    TArray<FString> Names;
    Names.AddUnique(DefinitionName(DesignName, ModelName));
    Names.AddUnique(DesignName);
    if (DesignName.Equals(TEXT("Rockhopper"))) Names.AddUnique(TEXT("CargoA"));
    if (DesignName.Equals(TEXT("Prospector"))) Names.AddUnique(TEXT("Zodiac"));
    if (Names[0] == TEXT("AsteroidBase") || Names[0] == TEXT("MiningStation")) Names.AddUnique(TEXT("FreightXfer"));
    if (!ModelName.IsEmpty()) Names.AddUnique(ModelName);
    for (const FString& Name : Names)
    {
        if (Name.IsEmpty()) continue;
        FString Package; FString Object; ClassPath(Name).Split(TEXT("."), &Package, &Object);
        if (!FPackageName::IsValidLongPackageName(Package) || !FPackageName::DoesPackageExist(Package)) continue;
        if (UClass* Class = LoadClass<AShipActor>(nullptr, *ClassPath(Name))) return Class;
    }
    return nullptr;
}

FString ShipVisualCatalog::IconFolder(const FString& DesignName)
{
    if (DesignName.Equals(TEXT("Prospector"))) return TEXT("Zodiac");
    if (DesignName.Equals(TEXT("Rockhopper"))) return TEXT("CargoA");
    if (DesignName.Equals(TEXT("Asteroid Base"), ESearchCase::IgnoreCase) ||
        DesignName.Equals(TEXT("Mining Station"), ESearchCase::IgnoreCase)) return TEXT("Station1");
    for (const FEntry& Entry : Entries())
    {
        if (FCString::Stricmp(Entry.Kind, TEXT("sam")) == 0 &&
            (DesignName.Equals(Entry.Design, ESearchCase::IgnoreCase) ||
             DesignName.Equals(Entry.Definition, ESearchCase::IgnoreCase)))
        {
            return Entry.Definition;
        }
    }

    return DesignName;
}

const TCHAR* ShipVisualCatalog::DedicatedStationIcon(const FString& DesignName)
{
    if (DesignName.Equals(TEXT("Asteroid Base")) || DesignName.Equals(TEXT("AsteroidBase")))
        return TEXT("/Game/UI/Ships/AsteroidBase/AsteroidBaseHud.AsteroidBaseHud");
    if (DesignName.Equals(TEXT("Mining Station")) || DesignName.Equals(TEXT("MiningStation")))
        return TEXT("/Game/UI/Ships/MiningStation/MiningStationHud.MiningStationHud");
    return nullptr;
}
