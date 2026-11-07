#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "GearIndicator.h"
#include "FactionInsignia.h"
#include "ShipDesign.h"
#include "ShipVisualCatalog.h"
#include "MasterBuildingActor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Materials/Material.h"
#include "Materials/MaterialFunction.h"
#include "MaterialShared.h"
#include "ShaderCompiler.h"
#include "AssetCompilingManager.h"
#include "RHI.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGearInterlockTest,"Starshatter.VisualPass.GearInterlock",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FGearInterlockTest::RunTest(const FString&)
{
    class FShipWithGear : public Ship
    {
    public: FShipWithGear():Ship("Gear interlock","",static_cast<ShipDesign*>(nullptr),1,0,nullptr,false){gear=new LandingGear;}
    } Player;
    for(auto Phase:{EOPSMode::DOCKED,EOPSMode::ALERT,EOPSMode::LOCKED,EOPSMode::LAUNCH,EOPSMode::TAKEOFF,EOPSMode::RECOVERY,EOPSMode::DOCKING})
    {
        Player.SetFlightPhase(Phase);Player.GetGear()->SetState(LandingGear::GEAR_DOWN);Player.ToggleGear();
        TestEqual(TEXT("Deck/ground phases cannot retract gear using the shared toggle"),Player.GetGear()->GetState(),LandingGear::GEAR_DOWN);
    }
    Player.SetFlightPhase(EOPSMode::ACTIVE);Player.ToggleGear();
    TestEqual(TEXT("Flight toggle begins retraction"),Player.GetGear()->GetState(),LandingGear::GEAR_RAISE);
    Player.ToggleGear();TestEqual(TEXT("Flight toggle can reverse to lowering"),Player.GetGear()->GetState(),LandingGear::GEAR_LOWER);
    TestTrue(TEXT("Triangle center accepts a click"),SSWGearIndicator::Contains(FVector2D(50,75),FVector2D::ZeroVector,FVector2D(100)));
    TestFalse(TEXT("Outside triangle does not consume flight clicks"),SSWGearIndicator::Contains(FVector2D(0,0),FVector2D::ZeroVector,FVector2D(100)));
    TestTrue(TEXT("Marakan uses its own insignia"),FactionInsignia::Path(TEXT("Marakan Hegemony")).Contains(TEXT("MarakanLogo")));
    TestTrue(TEXT("Dantari insignia remains available for campaign/reference"),FactionInsignia::Path(TEXT("Dantari Separatists")).Contains(TEXT("DantariLogo")));
    TestTrue(TEXT("Unknown faction never borrows an unrelated insignia"),FactionInsignia::Path(TEXT("Zolon")).IsEmpty());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundCatalogueTest,"Starshatter.VisualPass.GroundCatalogue",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FGroundCatalogueTest::RunTest(const FString&)
{
    int32 Count=0;
    for(const auto& Entry:ShipVisualCatalog::Entries())if(FString(Entry.Kind)==TEXT("building")||FString(Entry.Kind)==TEXT("sam"))
    {
        ++Count;UClass* Class=ShipVisualCatalog::Resolve(Entry.Definition,Entry.Model);
        if(TestNotNull(*FString::Printf(TEXT("DEF ground Blueprint: %s"),Entry.Definition),Class))
            TestTrue(TEXT("Ground actors use native building parent with legacy ship combat"),Class->IsChildOf(AMasterBuildingActor::StaticClass()));
        TestTrue(TEXT("Ground assets have a dedicated Content path"),ShipVisualCatalog::CanonicalClassPath(Entry.Definition).StartsWith(TEXT("/Game/Assets/Ground/")));
    }
    TestEqual(TEXT("All building and armed ground DEFs provisioned"),Count,33);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShipPBRShaderTest,"Starshatter.VisualPass.PBRShaders",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FShipPBRShaderTest::RunTest(const FString&)
{
    TestNotNull(TEXT("Shared wear/damage surface function"),LoadObject<UMaterialFunction>(nullptr,TEXT("/Game/Materials/Ships/MF_SSW_ShipSurface.MF_SSW_ShipSurface")));
    if(GUsingNullRHI){AddWarning(TEXT("NullRHI: shader/render verification deferred to RHI test"));return true;}
    auto& Registry=FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    FARFilter Filter;Filter.PackagePaths.Add(TEXT("/Game/Models"));Filter.bRecursivePaths=true;Filter.ClassPaths.Add(UMaterial::StaticClass()->GetClassPathName());
    TArray<FAssetData> Assets;Registry.GetAssets(Filter,Assets);TArray<UMaterial*> Materials;
    for(const auto& Asset:Assets){const FString Path=Asset.PackageName.ToString();if(Path.Contains(TEXT("/Bolt/"))||Path.Contains(TEXT("/Beams/")))continue;if(auto* Mat=Cast<UMaterial>(Asset.GetAsset()))Materials.Add(Mat);}
    FAssetCompilingManager::Get().FinishAllCompilation();if(GShaderCompilingManager)GShaderCompilingManager->FinishAllCompilation();
    int32 Checked=0;
    for(auto* Mat:Materials)
        if(auto* Resource=Mat->GetMaterialResource(GMaxRHIShaderPlatform))
        {
            ++Checked;for(const auto& Error:Resource->GetCompileErrors())AddError(Mat->GetPathName()+TEXT(": ")+Error);
        }
    TestTrue(TEXT("Compiled fleet material coverage"),Checked>=100);
    AddInfo(FString::Printf(TEXT("Checked %d compiled ship/model materials"),Checked));return true;
}
#endif
