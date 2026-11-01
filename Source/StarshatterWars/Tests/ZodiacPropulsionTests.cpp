#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Misc/Paths.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "StarshatterShipDesignSubsystem.h"
#include "ShipDesignRegistry.h"
#include "WeaponDesignRegistry.h"
#include "Thruster.h"
#include "Ship.h"
#include "ZodiacShipActor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZodiacPropulsionTest,"Starshatter.Fighters.ZodiacPropulsion",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FZodiacPropulsionTest::RunTest(const FString& Parameters)
{
    auto* Table=NewObject<UDataTable>();Table->RowStruct=FShipDesign::StaticStruct();
    auto* Importer=NewObject<UStarshatterShipDesignSubsystem>(NewObject<UGameInstance>());
    Importer->ShipDesignDataTable=Table;
    const FString File=FPaths::ProjectContentDir()/TEXT("GameData/Ships/Zodiac.def");
    const auto Ansi=StringCast<ANSICHAR>(*File);Importer->LoadShipDesign(Ansi.Get());
    const auto* Row=Table->FindRow<FShipDesign>(TEXT("Zodiac"),TEXT("Zodiac exhaust"));
    if(!TestNotNull(TEXT("Zodiac parses from definition"),Row))return false;
    if(!TestTrue(TEXT("Drive and maneuvering jets parsed"),!Row->Drive.IsEmpty()&&!Row->Thruster.IsEmpty()))return false;
    TestEqual(TEXT("Legacy main thrust preserved"),Row->Drive[0].Thrust,45.f);
    TestEqual(TEXT("Legacy hull scale preserved"),Row->Scale,0.45f);
    if(!TestEqual(TEXT("Two aft engine nozzles"),Row->Drive[0].Ports.Num(),2))return false;
    TestTrue(TEXT("Port engine is aft in Unreal X"),Row->Drive[0].Ports[0].Location.Equals(FVector(-143.1,-9,4.5),0.001));
    TestTrue(TEXT("Starboard engine matches mirrored nozzle"),Row->Drive[0].Ports[1].Location.Equals(FVector(-143.1,9,4.5),0.001));
    TestEqual(TEXT("All twelve legacy RCS ports retained"),Row->Thruster[0].Ports.Num(),12);
    const auto OriginalWeapons=WeaponDesignRegistry::GetAll();
    ON_SCOPE_EXIT {WeaponDesignRegistry::Clear();for(const auto& Entry:OriginalWeapons)WeaponDesignRegistry::RegisterDesign(Entry.Key,Entry.Value);};
    auto* Weapons=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_WeaponDesign.DT_WeaponDesign"));
    if(!TestNotNull(TEXT("Weapon designs"),Weapons))return false;
    for(const auto& Entry:Weapons->GetRowMap())WeaponDesignRegistry::RegisterDesign(Entry.Key,*reinterpret_cast<const FWeaponDesign*>(Entry.Value));
    const auto Originals=ShipDesignRegistry::GetAll();
    ON_SCOPE_EXIT {ShipDesignRegistry::Clear();for(const auto& Entry:Originals)ShipDesignRegistry::RegisterDesign(Entry.Key,Entry.Value);};
    ShipDesignRegistry::RegisterDesign(FName(TEXT("Zodiac")),*Row);
    auto* Design=ShipDesignRegistry::FindLegacy(FName(TEXT("Zodiac")));
    if(!TestNotNull(TEXT("Runtime design"),Design))return false;
    Ship Runtime("Zodiac exhaust test","",Design,1,0,nullptr,false);
    if(TestNotNull(TEXT("Legacy-default plasma maneuvering system is instantiated"),Runtime.GetThruster()))
        TestEqual(TEXT("All twelve RCS ports reach the runtime system"),Runtime.GetThruster()->GetNumThrusters(),12);
    const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    auto* World=UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
    ON_SCOPE_EXIT {World->DestroyWorld(false);};
    UClass* Class=LoadClass<AZodiacShipActor>(nullptr,TEXT("/Game/Models/Zodiac/BP_Zodiac.BP_Zodiac_C"));
    if(!TestNotNull(TEXT("Saved Zodiac Blueprint"),Class))return false;
    auto* Actor=World->SpawnActor<AZodiacShipActor>(Class);
    Actor->BindRuntimeShip(&Runtime);
    ON_SCOPE_EXIT {Actor->BindRuntimeShip(nullptr);};
    TArray<USceneComponent*> Components;Actor->GetComponents(Components);
    int32 Nozzles=0;
    for(auto* Component:Components)if(Component->GetName().StartsWith(TEXT("RuntimeMainEnginePoint_")))
    {
        // Check the complete imported-row -> runtime -> BP/hull transform chain.
        if(!TestTrue(TEXT("No unexpected engine point"),Nozzles<2))return false;
        // The native mesh is Y-longitudinal; BP VisualRoot turns it into the
        // ship's X-forward frame. These are the two mesh-space aft nozzles.
        const FVector Expected(Nozzles==0?20.0:-20.0,318.0,10.0);
        const FVector HullLocal=Actor->HullMesh->GetComponentTransform().InverseTransformPosition(Component->GetComponentLocation());
        TestTrue(TEXT("Exhaust point lands at the mesh nozzle without double scale"),HullLocal.Equals(Expected,0.001));
        AddInfo(FString::Printf(TEXT("Zodiac nozzle %d hull local %s"),Nozzles,*HullLocal.ToString()));
        ++Nozzles;
    }
    TestEqual(TEXT("Runtime creates both exhaust points"),Nozzles,2);
    return true;
}
#endif

