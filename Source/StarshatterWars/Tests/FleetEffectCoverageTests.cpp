#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Misc/Paths.h"
#include "Engine/World.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
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
#include "TacticalReferenceActor.h"
#include "ShipWeaponMountComponent.h"
#include "TurretActor.h"
#include "TurretBaseComponent.h"
#include "TurretBaseActor.h"
#include "Weapon.h"
#include "WeaponDesign.h"
#include "CapitalShipActor.h"
#include "ShieldComponent.h"
#include "Shield.h"
#include "SimShot.h"
#include "Game.h"
#include "SimComponent.h"
#include "StarshatterSystemDesignSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFleetEffectCoverageTest,"Starshatter.VisualPass.FleetEffects",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FFleetEffectCoverageTest::RunTest(const FString&)
{
    const bool bWasPaused = Game::Paused();
    Game::SetPausedState(false);
    ON_SCOPE_EXIT { Game::SetPausedState(bWasPaused); };
    const auto SavedWeapons=WeaponDesignRegistry::GetAll();
    const auto SavedShips=ShipDesignRegistry::GetAll();
    ON_SCOPE_EXIT {WeaponDesignRegistry::Clear();for(const auto& E:SavedWeapons)WeaponDesignRegistry::RegisterDesign(E.Key,E.Value);
        ShipDesignRegistry::Clear();for(const auto& E:SavedShips)ShipDesignRegistry::RegisterDesign(E.Key,E.Value);};
    auto* Weapons=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_WeaponDesign.DT_WeaponDesign"));
    if(!TestNotNull(TEXT("Weapon design table"),Weapons))return false;
    for(const auto& E:Weapons->GetRowMap())WeaponDesignRegistry::RegisterDesign(E.Key,*reinterpret_cast<const FWeaponDesign*>(E.Value));
    auto* GameInstance = NewObject<UGameInstance>();
    auto* SystemImporter = NewObject<UStarshatterSystemDesignSubsystem>(GameInstance);
    SystemImporter->LoadSystemDesigns();
    auto* Importer=NewObject<UStarshatterShipDesignSubsystem>(GameInstance);
    auto* Table=NewObject<UDataTable>();Table->RowStruct=FShipDesign::StaticStruct();Importer->ShipDesignDataTable=Table;
    const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    auto* World=UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
    ON_SCOPE_EXIT {World->DestroyWorld(false);};
    const bool bUseBootDesigns = IsRunningGame();
    if (bUseBootDesigns)
        TestEqual(TEXT("Boot version shown by main menu"), Game::VersionInfo, FString(TEXT("6.0 Alpha 2")));
    int32 Checked=0;
    int32 MountShips=0;
    int32 MountedTurrets=0;
    for(const auto& Entry:ShipVisualCatalog::Entries())
    {
        const FString Kind(Entry.Kind);
        if (Kind == TEXT("building")) continue;
        auto* Class=ShipVisualCatalog::Resolve(Entry.Definition,Entry.Model);
        auto* Defaults=Class?Cast<AShipActor>(Class->GetDefaultObject()):nullptr;
        if(!Defaults || !Defaults->HullMesh || !Defaults->HullMesh->GetStaticMesh())continue;
        const FString Filename=FPaths::ProjectContentDir()/TEXT("GameData/Ships")/(FString(Entry.Definition)+TEXT(".def"));
        if(!FPaths::FileExists(Filename))continue;
        const auto Ansi=StringCast<ANSICHAR>(*Filename);Importer->LoadShipDesign(Ansi.Get());
        const bool bZolonFleet = FString(Entry.Definition).StartsWith(TEXT("ZZ"));
        const FName DesignKey(Entry.Design);
        const auto* Row = bUseBootDesigns && bZolonFleet
            ? SavedShips.Find(DesignKey)
            : Table->FindRow<FShipDesign>(DesignKey, TEXT("Fleet effects"), false);
        if (bZolonFleet)
            TestNotNull(FString(Entry.Definition) + TEXT(" design available for runtime validation"), Row);
        if (!Row) continue;
        if (FString(Entry.Definition) == TEXT("Falcon") && TestTrue(TEXT("Falcon death spiral exists"), !Row->DeathSpiral.IsEmpty()))
        {
            const auto& Debris = Row->DeathSpiral[0].Debris;
            if (TestEqual(TEXT("Both Falcon debris entries survive parsing"), Debris.Num(), 2))
            {
                TestEqual(TEXT("Falcon fragment model"), Debris[0].ModelName, FString(TEXT("DebrisX")));
                TestEqual(TEXT("Falcon fragment count"), Debris[0].Count, 4);
                TestEqual(TEXT("Falcon fragment mass"), Debris[0].Mass, 20.0f);
                TestEqual(TEXT("Falcon fragment lifetime"), Debris[0].Life, 1.8f);
            }
        }
        const FName Key(Entry.Definition);ShipDesignRegistry::RegisterDesign(Key,*Row);
        auto* Design=ShipDesignRegistry::FindLegacy(Key);if(!TestNotNull(FString(Entry.Definition)+TEXT(" runtime design"),Design))continue;
        Ship Runtime(TCHAR_TO_ANSI(Entry.Definition),"",Design,1,0,nullptr,false);
        if (Key == TEXT("Orion"))
        {
            SimComponent* Core = nullptr;
            SimComponent* Regulator = nullptr;
            for (int Index = 0; Index < Runtime.GetSystems().size(); ++Index)
            {
                auto& Components = Runtime.GetSystems()[Index]->GetComponents();
                if (Components.size() != 7 ||
                    FCStringAnsi::Strcmp(Components[0]->Name(), "Reactor Core") != 0)
                    continue;
                Core = Components[0];
                Regulator = Components[4];
                break;
            }
            if (TestNotNull(TEXT("Fusion reactor copies seven DEF components onto the ship"), Core) &&
                TestNotNull(TEXT("Fusion reactor regulator"), Regulator))
            {
                TestEqual(TEXT("Core abbreviation"), FString(ANSI_TO_TCHAR(Core->Abbreviation())), FString(TEXT("Reac Core")));
                TestEqual(TEXT("Core repair time"), Core->RepairTime(), 120.0f);
                TestEqual(TEXT("Core replacement time"), Core->ReplaceTime(), 24000.0f);
                TestTrue(TEXT("Core affects efficiency and safety"), Core->DamageEfficiency() && Core->DamageSafety());
                TestFalse(TEXT("Core does not affect stability"), Core->DamageStability());
                Core->ApplyDamage(30);
                TestEqual(TEXT("Component damage reduces availability"), Core->Availability(), 70.0f);
                Core->Repair();
                TestEqual(TEXT("Repair uses DEF timer"), Core->TimeRemaining(), 120.0f);
                Core->ExecMaintFrame(121);
                TestTrue(TEXT("Repair finishes"), Core->TimeRemaining() <= 0);
                TestEqual(TEXT("Regulator DEF spares"), Regulator->SpareCount(), 2);
                Regulator->ApplyDamage(55);
                Regulator->Replace();
                TestEqual(TEXT("Replacement consumes a spare"), Regulator->SpareCount(), 1);
                TestEqual(TEXT("Replacement uses DEF timer"), Regulator->TimeRemaining(), 60.0f);
                Regulator->ExecMaintFrame(61);
                TestEqual(TEXT("Replacement restores full availability"), Regulator->Availability(), 100.0f);
            }
        }
        auto* Actor=World->SpawnActor<AShipActor>(Class);if(!TestNotNull(FString(Entry.Definition)+TEXT(" actor"),Actor))continue;
        Actor->BindRuntimeShip(&Runtime);
        ON_SCOPE_EXIT {Actor->BindRuntimeShip(nullptr);Actor->Destroy();};
        ++MountShips;
        for (int32 Index = 0; Index < Actor->NativeWeaponHardpoints.Num(); ++Index)
        {
            const auto* Mount = Cast<UShipWeaponMountComponent>(Actor->NativeWeaponHardpoints[Index].Get());
            if (!Mount || !Mount->RuntimeWeapon || !Mount->RuntimeWeapon->Design()) continue;
            const auto* WeaponDef = Mount->RuntimeWeapon->Design();
            if (!WeaponDef->turret.length()) continue;

            const FString Label = FString(Entry.Definition) + TEXT(" ") + ANSI_TO_TCHAR(WeaponDef->name.data());
            const auto* Gun = Mount->GetTurretVisualActor();
            if (!TestNotNull(Label + TEXT(" turret actor"), Gun)) continue;
            TestNotNull(Label + TEXT(" gun mesh"), Gun->GunMesh->GetStaticMesh().Get());
            ++MountedTurrets;
            if (!WeaponDef->turret_base.length()) continue;

            const UTurretBaseComponent* Base = nullptr;
            for (const auto& Point : Actor->AuthoredTurretPointComponents)
            {
                const auto* Candidate = Cast<UTurretBaseComponent>(Point.Get());
                if (Candidate && Candidate->SourceWeaponIndex == Index)
                {
                    Base = Candidate;
                    break;
                }
            }
            if (!TestNotNull(Label + TEXT(" matching base mount"), Base)) continue;
            const auto* BaseActor = Cast<ATurretBaseActor>(Base->GetChildActor());
            if (TestNotNull(Label + TEXT(" base actor"), BaseActor))
                TestNotNull(Label + TEXT(" base mesh"), BaseActor->BaseMesh->GetStaticMesh().Get());
            TestTrue(Label + TEXT(" gun and base coincide"),
                Base->GetComponentLocation().Equals(Mount->GetComponentLocation(), 0.01));
        }

        if (Row->ShipEmpire == EShipEmpire::Zolon && bZolonFleet)
        {
            const FString Label(Entry.Definition);
            int32 ArmedStations = 0;
            for (const auto& Point : Actor->NativeWeaponHardpoints)
            {
                const auto* Mount = Cast<UShipWeaponMountComponent>(Point.Get());
                if (!Mount || !Mount->RuntimeWeapon) continue;
                ++ArmedStations;
                for (int32 Barrel = 0; Barrel < Mount->GetMuzzleCount(); ++Barrel)
                {
                    const FTransform Muzzle = Mount->GetMuzzleWorldTransform(Barrel);
                    TestFalse(Label + TEXT(" muzzle has a finite transform"), Muzzle.ContainsNaN());
                    TestTrue(Label + TEXT(" muzzle is aligned with its station"),
                        FVector::DotProduct(Muzzle.GetUnitAxis(EAxis::X), Mount->GetForwardVector()) > 0.99);
                }
            }
            TestTrue(Label + TEXT(" DEF weapons are bound to native stations"), ArmedStations > 0);
            TestEqual(Label + TEXT(" authored running lights"), Actor->NavLights.Num(), 3);
            auto* Thruster = Runtime.GetThruster();
            if (TestNotNull(Label + TEXT(" runtime thruster"), Thruster))
                TestEqual(Label + TEXT(" runtime nozzle count"), Thruster->GetNumThrusters(), 6);

            auto* Shield = Runtime.GetShield();
            if (Row->Shield.IsEmpty())
            {
                TestNull(Label + TEXT(" unshielded by DEF"), Shield);
            }
            else if (TestNotNull(Label + TEXT(" runtime shield"), Shield))
            {
                auto* Capital = Cast<ACapitalShipActor>(Actor);
                if (!TestNotNull(Label + TEXT(" capital presentation"), Capital)) continue;
                Shield->SetPowerLevel(100);
                Shield->ExecFrame(10);
                Shield->Distribute(Shield->GetConsumption(), 1);
                WeaponDesign Weapon;
                Weapon.penetration = 1;
                Weapon.life = 1;
                SimShot Shot(FVector::ZeroVector, Camera(), &Weapon);
                const FVector Impact = Runtime.GetLocation() + Runtime.GetCam().vpn() * 100.0;
                Runtime.InflictDamage(100, &Shot, 1, Impact);
                TestFalse(Label + TEXT(" hull hit leaves shell hidden"), Capital->ShieldComponent->IsComponentTickEnabled());
                Runtime.InflictDamage(100, &Shot, 3, Impact);
                TestTrue(Label + TEXT(" deflected hit starts shell effect"), Capital->ShieldComponent->IsComponentTickEnabled());
                TArray<UStaticMeshComponent*> Meshes;
                Capital->GetComponents(Meshes);
                const FString ExpectedMesh = TEXT("SM_") + Row->Shield[0].ModelName;
                const auto* Found = Meshes.FindByPredicate([&](const UStaticMeshComponent* Mesh) {
                    return Mesh->GetStaticMesh() && Mesh->GetStaticMesh()->GetName() == ExpectedMesh;
                });
                if (TestNotNull(Label + TEXT(" DEF-selected shield shell"), Found))
                {
                    TestTrue(Label + TEXT(" shell follows corrected hull transform"),
                        (*Found)->GetAttachParent() == Capital->HullMesh &&
                        (*Found)->GetComponentTransform().Equals(Capital->HullMesh->GetComponentTransform()));
                    TestEqual(Label + TEXT(" shield visual has no collision"), (*Found)->GetCollisionEnabled(),
                        ECollisionEnabled::NoCollision);
                    UActorComponent* Effect = Capital->ShieldComponent;
                    Effect->TickComponent(0.5f, LEVELTICK_All, &Effect->PrimaryComponentTick);
                    TestFalse(Label + TEXT(" shield visual fades out"), (*Found)->IsVisible());
                }
            }
        }

        if (Kind == TEXT("sam") || Kind == TEXT("hulk")) continue;
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
        int32 VisibleBulbs = 0;
        for (const auto& Bulb : Actor->NavLightBulbMeshes)
        {
            if (Bulb && Bulb->GetStaticMesh() && Bulb->GetMaterial(0)) ++VisibleBulbs;
        }
        TestEqual(FString(Entry.Definition) + TEXT(" emissive nav-light bulbs"), VisibleBulbs, ExpectedLights);
        if(RCS && Actor->bEnableThrusterEmitters)TestTrue(FString(Entry.Definition)+TEXT(" Niagara thruster ports provisioned"),Thrusters>=RCS->GetNumThrusters());
        if (RCS)
        {
            int32 Nozzles = 0;
            for (const USceneComponent* Component : Components)
                if (Component->GetName().StartsWith(TEXT("ThrusterNozzle"))) ++Nozzles;
            TestEqual(FString(Entry.Definition) + TEXT(" thruster nozzle count"), Nozzles, RCS->GetNumThrusters());
            if (FString(Entry.Definition) == TEXT("Orion") || FString(Entry.Definition) == TEXT("Falcon"))
            {
                for (const USceneComponent* Component : Components)
                {
                    if (!Component->GetName().StartsWith(TEXT("ThrusterNozzle"))) continue;
                    const auto* Nozzle = Cast<UStaticMeshComponent>(Component);
                    if (!Nozzle || !Nozzle->GetStaticMesh()) continue;
                    const FTransform Pose = Nozzle->GetComponentTransform();
                    const double HalfLength = Nozzle->GetStaticMesh()->GetBounds().BoxExtent.Z;
                    const FVector EndA = Pose.TransformPosition(FVector(0, 0, HalfLength));
                    const FVector EndB = Pose.TransformPosition(FVector(0, 0, -HalfLength));
                    const FVector Axis = Pose.GetUnitAxis(EAxis::Z);
                    const bool bAligned = Components.ContainsByPredicate([&](const USceneComponent* FX) {
                        return FX->IsA<UNiagaraComponent>() && FX->GetAttachParent() == Actor->VFXRoot &&
                            (FX->GetComponentLocation().Equals(EndA, 0.05) || FX->GetComponentLocation().Equals(EndB, 0.05)) &&
                            FMath::Abs(FVector::DotProduct(Axis, FX->GetForwardVector())) > 0.999;
                    });
                    TestTrue(FString(Entry.Definition) + TEXT(" exhaust starts at the aligned nozzle end"), bAligned);
                }
            }
        }

        const FString ShipKey(Entry.Definition);
        if (ShipKey == TEXT("Orion") || ShipKey == TEXT("Falcon") || ShipKey == TEXT("Dragon"))
        {
            auto* Viewer = World->SpawnActor<ATacticalReferenceActor>();
            Viewer->ShowShipDesign(*Row);
            TArray<AActor*> Attached;
            Viewer->GetAttachedActors(Attached);
            auto* Preview = Attached.Num() == 1 ? Cast<AShipActor>(Attached[0]) : nullptr;
            if (TestNotNull(ShipKey + TEXT(" assembled tactical preview"), Preview))
            {
                TestEqual(ShipKey + TEXT(" tactical preview nav lights"), Preview->NavLights.Num(), ExpectedLights);
                TestEqual(ShipKey + TEXT(" tactical preview weapon stations"),
                    Preview->NativeWeaponHardpoints.Num(), Actor->NativeWeaponHardpoints.Num());
                if (ShipKey == TEXT("Orion"))
                {
                    int32 Turrets = 0;
                    for (const auto& Point : Preview->NativeWeaponHardpoints)
                        if (const auto* Mount = Cast<UShipWeaponMountComponent>(Point.Get()))
                            if (const auto* Gun = Mount->GetTurretVisualActor())
                                if (Gun->GunMesh && Gun->GunMesh->GetStaticMesh()) ++Turrets;
                    TestEqual(TEXT("Orion tactical viewer displays six Vanguard and four Phalanx turrets"), Turrets, 10);
                }
            }
            Viewer->SetDisplayMesh(nullptr);
            Viewer->Destroy();
        }
        AddInfo(FString::Printf(TEXT("%s: main=%d rcsFX=%d nav=%d"),Entry.Definition,Engines,Thrusters,Lights));
    }
    AddInfo(FString::Printf(TEXT("Fleet mounts checked on %d hulls, %d mounted turrets"), MountShips, MountedTurrets));
    AddInfo(FString::Printf(TEXT("Fleet effects verified on %d imported hulls"),Checked));
    TestTrue(TEXT("All three fleet groups represented with imported hulls"),Checked>=25);
    return true;
}
#endif
