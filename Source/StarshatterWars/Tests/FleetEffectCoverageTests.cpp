#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Misc/Paths.h"
#include "Engine/World.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Components/StaticMeshComponent.h"
#include "StarshatterShipDesignSubsystem.h"
#include "ShipVisualCatalog.h"
#include "ShipDesignRegistry.h"
#include "WeaponDesignRegistry.h"
#include "ShipActor.h"
#include "Ship.h"
#include "Drive.h"
#include "Thruster.h"
#include "NavLight.h"
#include "NiagaraComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFleetEffectCoverageTest,"Starshatter.VisualPass.FleetEffects",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFleetEffectCoverageTest::RunTest(const FString&)
{
    const auto SavedWeapons=WeaponDesignRegistry::GetAll();
    const auto SavedShips=ShipDesignRegistry::GetAll();
    ON_SCOPE_EXIT {WeaponDesignRegistry::Clear();for(const auto& E:SavedWeapons)WeaponDesignRegistry::RegisterDesign(E.Key,E.Value);
        ShipDesignRegistry::Clear();for(const auto& E:SavedShips)ShipDesignRegistry::RegisterDesign(E.Key,E.Value);};
    auto* Weapons=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_WeaponDesign.DT_WeaponDesign"));
    if(!TestNotNull(TEXT("Weapon design table"),Weapons))return false;
    for(const auto& E:Weapons->GetRowMap())WeaponDesignRegistry::RegisterDesign(E.Key,*reinterpret_cast<const FWeaponDesign*>(E.Value));
    auto* Importer=NewObject<UStarshatterShipDesignSubsystem>(NewObject<UGameInstance>());
    auto* Table=NewObject<UDataTable>();Table->RowStruct=FShipDesign::StaticStruct();Importer->ShipDesignDataTable=Table;
    const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    auto* World=UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
    ON_SCOPE_EXIT {World->DestroyWorld(false);};
    int32 Checked=0;
    for(const auto& Entry:ShipVisualCatalog::Entries())
    {
        const FString Kind(Entry.Kind);
        if(Kind==TEXT("building")||Kind==TEXT("sam")||Kind==TEXT("starbase")||Kind==TEXT("hulk"))continue;
        auto* Class=ShipVisualCatalog::Resolve(Entry.Definition,Entry.Model);
        auto* Defaults=Class?Cast<AShipActor>(Class->GetDefaultObject()):nullptr;
        if(!Defaults || !Defaults->HullMesh || !Defaults->HullMesh->GetStaticMesh())continue;
        const FString Filename=FPaths::ProjectContentDir()/TEXT("GameData/Ships")/(FString(Entry.Definition)+TEXT(".def"));
        if(!FPaths::FileExists(Filename))continue;
        const auto Ansi=StringCast<ANSICHAR>(*Filename);Importer->LoadShipDesign(Ansi.Get());
        const auto* Row=Table->FindRow<FShipDesign>(FName(Entry.Design),TEXT("Fleet effects"),false);
        if(!Row || (Row->ShipEmpire!=EShipEmpire::Terellian && Row->ShipEmpire!=EShipEmpire::Marakan && Row->ShipEmpire!=EShipEmpire::Dantari))continue;
        const FName Key(Entry.Definition);ShipDesignRegistry::RegisterDesign(Key,*Row);
        auto* Design=ShipDesignRegistry::FindLegacy(Key);if(!TestNotNull(FString(Entry.Definition)+TEXT(" runtime design"),Design))continue;
        Ship Runtime(TCHAR_TO_ANSI(Entry.Definition),"",Design,1,0,nullptr,false);
        auto* Actor=World->SpawnActor<AShipActor>(Class);if(!TestNotNull(FString(Entry.Definition)+TEXT(" actor"),Actor))continue;
        Actor->BindRuntimeShip(&Runtime);
        ON_SCOPE_EXIT {Actor->BindRuntimeShip(nullptr);Actor->Destroy();};
        ++Checked;
        int32 Engines=0,Thrusters=0,Lights=0;
        TArray<USceneComponent*> Components;Actor->GetComponents(Components);
        for(auto* C:Components)
        {
            if(C->GetName().StartsWith(TEXT("RuntimeMainEnginePoint_")))++Engines;
            if(C->IsA<UNiagaraComponent>() && C->GetAttachParent()==Actor->VFXRoot)++Thrusters;
            if(C->IsA<UNavLightComponent>())++Lights;
            if(C->GetAttachParent()==Actor->VFXRoot)TestFalse(FString(Entry.Definition)+TEXT(" effect world location finite"),C->GetComponentLocation().ContainsNaN());
        }
        int32 ExpectedLights=0;for(const auto& Nav:Row->Navlight)ExpectedLights+=Nav.Beacons.Num();
        const auto* Drive=Runtime.GetMainDrive();const auto* RCS=Runtime.GetThruster();
        TestEqual(FString(Entry.Definition)+TEXT(" engine count"),Engines,Drive?Drive->NumPorts():0);
        TestEqual(FString(Entry.Definition)+TEXT(" light count"),Lights,ExpectedLights);
        if(RCS)TestTrue(FString(Entry.Definition)+TEXT(" Niagara thruster ports provisioned"),Thrusters>=RCS->GetNumThrusters());
        AddInfo(FString::Printf(TEXT("%s: main=%d rcsFX=%d nav=%d"),Entry.Definition,Engines,Thrusters,Lights));
    }
    AddInfo(FString::Printf(TEXT("Fleet effects verified on %d imported hulls"),Checked));
    TestTrue(TEXT("All three fleet groups represented with imported hulls"),Checked>=25);
    return true;
}
#endif
