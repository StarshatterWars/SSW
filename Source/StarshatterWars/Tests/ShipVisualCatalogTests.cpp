#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "ShipVisualCatalog.h"
#include "ShipActor.h"
#include "FighterActor.h"
#include "Components/StaticMeshComponent.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShipVisualCatalogTest,"Starshatter.Ships.DefinitionBlueprintCoverage",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FShipVisualCatalogTest::RunTest(const FString&)
{
    TSet<FString> Seen;
    for(const auto& Entry : ShipVisualCatalog::Entries())
    {
        const FString Key(Entry.Definition);
        TestFalse(*FString::Printf(TEXT("Unique DEF asset key: %s"),*Key),Seen.Contains(Key));Seen.Add(Key);
        TestEqual(*FString::Printf(TEXT("DEF filename lookup: %s"),*Key),ShipVisualCatalog::DefinitionName(Key),Key);
        TestEqual(*FString::Printf(TEXT("Internal design alias: %s"),Entry.Design),ShipVisualCatalog::DefinitionName(Entry.Design,Entry.Model),Key);
        UClass* Native=LoadClass<AShipActor>(nullptr,*FString::Printf(TEXT("/Script/StarshatterWars.%s"),Entry.NativeClass));
        TestNotNull(*FString::Printf(TEXT("Matching native class: %s"),Entry.NativeClass),Native);
        UClass* Canonical=LoadClass<AShipActor>(nullptr,*ShipVisualCatalog::CanonicalClassPath(Key));
        if(!TestNotNull(*FString::Printf(TEXT("Canonical DEF Blueprint: %s"),*Key),Canonical))continue;
        TestTrue(*FString::Printf(TEXT("Runtime resolves DEF/internal name to same Blueprint: %s"),*Key),ShipVisualCatalog::Resolve(Entry.Design,Entry.Model)==Canonical);
        auto* Defaults=Cast<AShipActor>(Canonical->GetDefaultObject());
        if(TestNotNull(TEXT("Ship actor default object"),Defaults))TestNotNull(*FString::Printf(TEXT("Editable HullMesh slot: %s"),*Key),Defaults->HullMesh);
        const FString Kind(Entry.Kind);
        if(Kind==TEXT("fighter")||Kind==TEXT("attack"))TestTrue(*FString::Printf(TEXT("Fighter inherits shared cockpit/gear behavior: %s"),*Key),Canonical->IsChildOf(AFighterActor::StaticClass()));
    }
    TestTrue(TEXT("Fleet inventory is populated"),Seen.Num()>50);
    TestEqual(TEXT("Cargo DEF remains canonical for IM3500"),ShipVisualCatalog::DefinitionName(TEXT("IM3500")),FString(TEXT("CargoA")));
    TestEqual(TEXT("Tanker DEF remains canonical for IM1906"),ShipVisualCatalog::DefinitionName(TEXT("IM1906")),FString(TEXT("TankerA")));
    TestEqual(TEXT("Shared Airbase2 identifier uses its model"),ShipVisualCatalog::DefinitionName(TEXT("Airbase2"),TEXT("TigerBay")),FString(TEXT("TigerBayBase")));
    TestEqual(TEXT("Ambiguous legacy name never picks an arbitrary base"),ShipVisualCatalog::DefinitionName(TEXT("Airbase2")),FString(TEXT("Airbase2")));
    TestNull(TEXT("Unrecognized design has no unrelated fallback"),ShipVisualCatalog::Resolve(TEXT("AutomationMissingShip")));
    return true;
}
#endif
