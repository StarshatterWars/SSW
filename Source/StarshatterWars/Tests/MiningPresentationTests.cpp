#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "MiningCrystalCatalog.h"
#include "MiningCrystalEquipment.h"
#include "GameStructs_economy.h"
#include "JsonObjectConverter.h"
#include "MiningBeamActor.h"
#include "SimShot.h"
#include "WeaponDesign.h"
#if WITH_EDITOR
#include "Engine/World.h"
#include "Components/PointLightComponent.h"
#include "NiagaraComponent.h"
#endif

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMiningCrystalRulesTest,"Starshatter.Mining.CrystalCatalogAndNeutralBaseline",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FMiningCrystalRulesTest::RunTest(const FString&)
{
    FString Json;FText Error;FMiningCrystalCatalog Catalog;
    if(!TestTrue(TEXT("Crystal JSON available"),FFileHelper::LoadFileToString(Json,*(FPaths::ProjectContentDir()/TEXT("GameData/Cargo/mining_crystals.json")))))return false;
    if(!TestTrue(TEXT("Crystal definitions validate"),Catalog.LoadJson(Json,Error)))return false;
    TestEqual(TEXT("Seven original SSW focusing lattices"),Catalog.GetDefinitions().Num(),7);
    TestEqual(TEXT("No crystal leaves Wep.def rate unchanged"),Catalog.ExtractionMultiplier(NAME_None,ESSWMineral::Magnetite),1.0);
    TestEqual(TEXT("Standard optics leave rate unchanged"),Catalog.ExtractionMultiplier(TEXT("StandardFocus"),ESSWMineral::WaterIce),1.0);
    TestEqual(TEXT("Explicit compatible crystal applies its authored multiplier"),Catalog.ExtractionMultiplier(TEXT("FerriteFocus"),ESSWMineral::Magnetite),1.08);
    TestEqual(TEXT("Incompatible crystal cannot grant its bonus"),Catalog.ExtractionMultiplier(TEXT("FerriteFocus"),ESSWMineral::WaterIce),1.0);
    TestEqual(TEXT("Unknown crystal fails neutral"),Catalog.ExtractionMultiplier(TEXT("Missing"),ESSWMineral::Magnetite),1.0);
    TestNotNull(TEXT("Ore has a deterministic visual profile without equipment"),Catalog.Appearance(NAME_None,ESSWMineral::WaterIce));
    const auto* Ice=Catalog.Appearance(NAME_None,ESSWMineral::WaterIce);
    if(Ice)TestEqual(TEXT("Ice appearance resolves cold optics"),Ice->Id,FName(TEXT("CryogenicFocus")));
    for(const int32 FPS:{30,60,120})
        for(int32 Frame=0;Frame<FPS*2;++Frame)
        {
            const float Pulse=FMiningCrystalCatalog::Pulse(static_cast<float>(Frame)/FPS,1.4f);
            TestTrue(TEXT("Pulse remains bounded at every tested rate"),Pulse>=.879f && Pulse<=1.121f);
        }
    FString Bad=Json;Bad.ReplaceInline(TEXT("1.08"),TEXT("9.0"));
    TestFalse(TEXT("Excessive multipliers rejected"),Catalog.LoadJson(Bad,Error));
    TestEqual(TEXT("Invalid update does not replace catalog"),Catalog.GetDefinitions().Num(),7);
    FEconomyState Economy;const FName Hull(TEXT("Ship/MiningOpticsTest"));
    TestEqual(TEXT("Ordinary fighters receive no automatic crystal"),FMiningCrystalService::FitConfigured(Economy,Hull,TEXT("Falcon"),ESSWMineral::Magnetite),NAME_None);
    const FName Fitted=FMiningCrystalService::FitConfigured(Economy,Hull,TEXT("Prospector"),ESSWMineral::Magnetite);
    TestEqual(TEXT("Explicit Prospector magazine fits a compatible lattice"),Fitted,FName(TEXT("FerriteFocus")));
    TestTrue(TEXT("Fresh fitted crystal applies rated coupling"),FMath::IsNearlyEqual(FMiningCrystalService::Multiplier(Economy,Hull,Fitted,ESSWMineral::Magnetite),1.08));
    FMiningCrystalService::RecordExtraction(Economy,Hull,Fitted,1500);
    TestTrue(TEXT("Condition reduces the bonus deterministically"),FMath::IsNearlyEqual(FMiningCrystalService::Multiplier(Economy,Hull,Fitted,ESSWMineral::Magnetite),1.04));
    FMiningCrystalService::FitConfigured(Economy,Hull,TEXT("Prospector"),ESSWMineral::Magnetite);
    TestEqual(TEXT("A new beam does not replenish crystal condition"),Economy.MiningOptics.FindChecked(FMiningCrystalService::EquipmentKey(Hull,Fitted)).ExtractedTonnes,1500.0);
    FString Saved;FEconomyState Restored;
    TestTrue(TEXT("Crystal equipment serializes with the economy"),FJsonObjectConverter::UStructToJsonObjectString(Economy,Saved));
    TestTrue(TEXT("Crystal equipment restores with the economy"),FJsonObjectConverter::JsonObjectStringToUStruct(Saved,&Restored));
    TestTrue(TEXT("Condition survives save and reload"),FMath::IsNearlyEqual(FMiningCrystalService::Multiplier(Restored,Hull,Fitted,ESSWMineral::Magnetite),1.04));
    FMiningCrystalService::RecordExtraction(Restored,Hull,Fitted,1.e8);
    TestEqual(TEXT("Exhausted crystal falls back to baseline without disabling mining"),FMiningCrystalService::Multiplier(Restored,Hull,Fitted,ESSWMineral::Magnetite),1.0);
    TestFalse(TEXT("Equipped crystal condition has a HUD summary"),FMiningCrystalService::Summary(Restored,Hull).IsEmpty());
    WeaponDesign Mining;Mining.name="Mining Laser";Mining.primary=true;Mining.beam=true;Mining.life=5;Mining.damage=999999;
    SimShot MiningShot(FVector::ZeroVector,Camera(),&Mining);
    TestEqual(TEXT("Focusing does not turn industrial beam into combat damage"),MiningShot.Damage(),0.0);
    MiningShot.SetMiningContact(true,TEXT("Magnetite"));
    TestTrue(TEXT("Actual contact metadata retains ore type"),MiningShot.HasMiningContact() && MiningShot.GetMiningContactMineral()==TEXT("Magnetite"));
    MiningShot.SetMiningContact(false);
    TestFalse(TEXT("Contact clears when ray misses"),MiningShot.HasMiningContact());
    TestEqual(TEXT("Miss clears stale mineral"),MiningShot.GetMiningContactMineral(),NAME_None);
    return true;
}

#if WITH_EDITOR
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMiningContactPresentationTest,"Starshatter.Mining.RealContactPresentationLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMiningContactPresentationTest::RunTest(const FString&)
{
    const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    auto* World=UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
    if(!TestNotNull(TEXT("Preview world"),World))return false;
    auto* Beam=World->SpawnActor<AMiningBeamActor>();
    if(!TestNotNull(TEXT("Native mining actor"),Beam)){World->DestroyWorld(false);return false;}
    Beam->BeginSimulationBeam(TEXT("Mining Laser"));
    Beam->SetBeamPoints(FVector::ZeroVector,FVector(1000,0,0));
    auto* Glow=Beam->FindComponentByClass<UPointLightComponent>();
    TestNotNull(TEXT("Contact light exists"),Glow);
    TestFalse(TEXT("Empty-space endpoint never shows ore contact"),Beam->HasVisibleOreContact());
    Beam->SetMiningContact(true,TEXT("Magnetite"),NAME_None);
    TestTrue(TEXT("Authoritative ore intersection enables contact"),Beam->HasVisibleOreContact());
    if(Glow)TestTrue(TEXT("Surface contact glows"),Glow->IsVisible() && Glow->Intensity>0);
    Beam->SetPresentationClock(true,5);
    TArray<UNiagaraComponent*> Systems;Beam->GetComponents(Systems);
    for(auto* FX:Systems)TestTrue(TEXT("Pause freezes beam and contact particles"),FX->IsPaused());
    Beam->SetMiningContact(false,NAME_None,NAME_None);
    if(Glow)TestFalse(TEXT("Miss hides previous impact immediately"),Glow->IsVisible());
    Beam->SetMiningContact(true,TEXT("Magnetite"),NAME_None);
    Beam->SetBeamActive(false);
    if(Glow)TestFalse(TEXT("Stop cleans ore glow"),Glow->IsVisible());
    TestTrue(TEXT("Simulation deactivation destroys presentation"),Beam->IsActorBeingDestroyed());
    World->DestroyWorld(false);return true;
}
#endif
#endif
