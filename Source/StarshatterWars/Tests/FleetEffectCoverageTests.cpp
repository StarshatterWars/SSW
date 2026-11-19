#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/FileManager.h"
#include "StaticMeshResources.h"
#include "Engine/World.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "StarshatterShipDesignSubsystem.h"
#include "ShipVisualCatalog.h"
#include "ShipDesignRegistry.h"
#include "WeaponDesignRegistry.h"
#include "StarshatterWeaponDesignSubsystem.h"
#include "ShipActor.h"
#include "Ship.h"
#include "Drive.h"
#include "EnginePlumeTuning.h"
#include "Thruster.h"
#include "NavLight.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
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
    auto* WeaponImporter = NewObject<UStarshatterWeaponDesignSubsystem>(GameInstance);
    WeaponImporter->LoadWeaponDesigns();
    const auto* MiningLaser = WeaponDesignRegistry::Find(FName(TEXT("Mining Laser")));
    if (TestNotNull(TEXT("Mining Laser definition is loaded"), MiningLaser))
        TestEqual(TEXT("Mining Laser rate survives DEF ingestion"), MiningLaser->MiningTonnesPerSecond, 0.25f);
    auto* SystemImporter = NewObject<UStarshatterSystemDesignSubsystem>(GameInstance);
    SystemImporter->LoadSystemDesigns();
    auto* Importer=NewObject<UStarshatterShipDesignSubsystem>(GameInstance);
    auto* Table=NewObject<UDataTable>();Table->RowStruct=FShipDesign::StaticStruct();Importer->ShipDesignDataTable=Table;
    const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    auto* World=UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
    ON_SCOPE_EXIT {World->DestroyWorld(false);};
    const bool bUseBootDesigns = IsRunningGame();
    if (bUseBootDesigns)
        TestEqual(TEXT("Boot version shown by main menu"), Game::VersionInfo, FString(TEXT("6.0 Alpha 3")));
    auto* LiveCatalog=NewObject<UDataTable>();
    if(!UStarshatterShipDesignSubsystem::ImportJsonCatalog(*LiveCatalog,FPaths::ProjectContentDir()/TEXT("GameData/Ships/ships.json")))
        return false;
    int32 Checked=0;
    int32 MountShips=0;
    int32 MountedTurrets=0;
    TArray<FString> UnverifiedHulls;
    for(const auto& Entry:ShipVisualCatalog::Entries())
    {
        const FString Kind(Entry.Kind);
        if (Kind == TEXT("building")) continue;
        auto* Class=ShipVisualCatalog::Resolve(Entry.Definition,Entry.Model);
        auto* Defaults=Class?Cast<AShipActor>(Class->GetDefaultObject()):nullptr;
        if(!Defaults || !Defaults->HullMesh || !Defaults->HullMesh->GetStaticMesh())
        {
            UnverifiedHulls.Add(FString(Entry.Definition) + TEXT(" (native hull mesh unavailable)"));
            continue;
        }
        const FString Filename=FPaths::ProjectContentDir()/TEXT("GameData/Ships")/(FString(Entry.Definition)+TEXT(".def"));
        if(!FPaths::FileExists(Filename))
        {
            UnverifiedHulls.Add(FString(Entry.Definition) + TEXT(" (definition missing)"));
            continue;
        }
        const auto Ansi=StringCast<ANSICHAR>(*Filename);Importer->LoadShipDesign(Ansi.Get());
        const bool bZolonFleet = FString(Entry.Definition).StartsWith(TEXT("ZZ"));
        const FName DesignKey(Entry.Design);
        const auto* Row = bUseBootDesigns && bZolonFleet
            ? SavedShips.Find(DesignKey)
            : Table->FindRow<FShipDesign>(DesignKey, TEXT("Fleet effects"), false);
        if (bZolonFleet)
            TestNotNull(FString(Entry.Definition) + TEXT(" design available for runtime validation"), Row);
        if (!Row)
        {
            UnverifiedHulls.Add(FString(Entry.Definition) + TEXT(" (design row unavailable)"));
            continue;
        }
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
        const auto* LiveRow=LiveCatalog->FindRow<FShipDesign>(FName(Entry.Definition),TEXT("Live propulsion"),false);
        if(!LiveRow || LiveRow->Model!=Row->Model)LiveRow=LiveCatalog->FindRow<FShipDesign>(DesignKey,TEXT("Live propulsion"),false);
        if(!TestNotNull(FString(Entry.Definition)+TEXT(" live JSON row"),LiveRow))continue;
        auto CheckPorts=[&](const auto& Authored,const auto& Live)
        {
            TestEqual(FString(Entry.Definition)+TEXT(" live system count"),Live.Num(),Authored.Num());
            for(int32 I=0;I<FMath::Min(Authored.Num(),Live.Num());++I)
            {
                TestEqual(FString(Entry.Definition)+TEXT(" live port count"),Live[I].Ports.Num(),Authored[I].Ports.Num());
                for(int32 J=0;J<FMath::Min(Authored[I].Ports.Num(),Live[I].Ports.Num());++J)
                    TestTrue(FString(Entry.Definition)+TEXT(" JSON port matches scaled DEF"),Live[I].Ports[J].Location.Equals(Authored[I].Ports[J].Location,.005));
            }
        };
        CheckPorts(Row->Drive,LiveRow->Drive);CheckPorts(Row->Thruster,LiveRow->Thruster);
        Row=LiveRow;
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

        // Export actual rendered hull vertices and propulsion components for geometry review.
        if (FParse::Param(FCommandLine::Get(),TEXT("AuditPropulsionGeometry")) && Actor->HullMesh && Actor->HullMesh->GetStaticMesh())
        {
            const auto* Render=Actor->HullMesh->GetStaticMesh()->GetRenderData();
            if(Render && Render->LODResources.Num())
            {
                const FString Dir=FPaths::ProjectSavedDir()/TEXT("Analysis/FleetPropulsion");
                IFileManager::Get().MakeDirectory(*Dir,true);
                const auto& LOD=Render->LODResources[0];
                FString MeshText,Ports=TEXT("component,x,y,z,dx,dy,dz,sx,sy,sz,distance\n");
                TArray<FVector> Vertices;
                for(uint32 I=0;I<LOD.VertexBuffers.PositionVertexBuffer.GetNumVertices();++I)
                {
                    const FVector V=Actor->HullMesh->GetComponentTransform().TransformPosition(FVector(LOD.VertexBuffers.PositionVertexBuffer.VertexPosition(I)));
                    Vertices.Add(V);MeshText+=FString::Printf(TEXT("v %.6f %.6f %.6f\n"),V.X,V.Y,V.Z);
                }
                for(int32 I=0;I+2<LOD.IndexBuffer.GetNumIndices();I+=3)
                    MeshText+=FString::Printf(TEXT("f %u %u %u\n"),LOD.IndexBuffer.GetIndex(I)+1,LOD.IndexBuffer.GetIndex(I+1)+1,LOD.IndexBuffer.GetIndex(I+2)+1);
                TArray<USceneComponent*> Parts;Actor->GetComponents(Parts);
                for(auto* Part:Parts)
                {
                    const FString Name=Part->GetName();
                    if(!Name.StartsWith(TEXT("RuntimeMainEnginePoint_")) && !Name.StartsWith(TEXT("ThrusterNozzle")))continue;
                    const FVector Pos=Part->GetComponentLocation(),DirV=Part->GetForwardVector(),Scale=Part->GetComponentScale();
                    double Distance=MAX_dbl;
                    for(int32 I=0;I+2<LOD.IndexBuffer.GetNumIndices();I+=3)
                    {
                        const FVector Closest=FMath::ClosestPointOnTriangleToPoint(Pos,Vertices[LOD.IndexBuffer.GetIndex(I)],Vertices[LOD.IndexBuffer.GetIndex(I+1)],Vertices[LOD.IndexBuffer.GetIndex(I+2)]);
                        Distance=FMath::Min(Distance,FVector::Distance(Pos,Closest));
                    }
                    if(Name.StartsWith(TEXT("RuntimeMainEnginePoint_")) && Kind!=TEXT("hulk"))
                        TestTrue(FString(Entry.Definition)+TEXT(" engine outlet is attached to actual hull triangles"),Distance<FMath::Max(.1,Actor->HullMesh->Bounds.SphereRadius*.02));
                    Ports+=FString::Printf(TEXT("%s,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n"),*Name,Pos.X,Pos.Y,Pos.Z,DirV.X,DirV.Y,DirV.Z,Scale.X,Scale.Y,Scale.Z,Distance);
                }
                FFileHelper::SaveStringToFile(MeshText,*(Dir/(FString(Entry.Definition)+TEXT(".obj"))));
                FFileHelper::SaveStringToFile(Ports,*(Dir/(FString(Entry.Definition)+TEXT(".csv"))));
            }
        }
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
        TestEqual(FString(Entry.Definition)+TEXT(" engine count"),Engines,Drive && Actor->bEnableMainEngineEmitters?Drive->NumPorts():0);
        TestEqual(FString(Entry.Definition)+TEXT(" light count"),Lights,ExpectedLights);
        if(Drive && Actor->bEnableMainEngineEmitters)
        {
            const FString Expected=EnginePlumeTuning::SystemPath(Row->ShipEmpire,Drive->GetDriveType());
            for(const auto& Engine:Actor->RuntimeMainEngineEmitters)if(Engine)
            {
                if(TestNotNull(FString(Entry.Definition) + TEXT(" native nozzle asset"),Engine->GetAsset()))
                    TestEqual(FString(Entry.Definition)+TEXT(" faction plume"),Engine->GetAsset()->GetPathName(),Expected);
                const double HullRadius=Actor->HullMesh->Bounds.SphereRadius;
                if(HullRadius>0)TestTrue(FString(Entry.Definition)+TEXT(" nozzle width bounded to actual imported hull"),
                    Engine->GetComponentScale().GetAbsMax()<=HullRadius*.0801);
            }
        }
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
        if (ShipKey == TEXT("Stormhawk") && RCS)
        {
            auto* Proxy = World->SpawnActor<AShipActor>(Class);
            if (TestNotNull(TEXT("Stormhawk client visual"), Proxy))
            {
                Proxy->InitializeSnapshotPropulsion(Design);
                TArray<uint8> Burns;
                Burns.Init(0, RCS->GetNumThrusters());
                int32 LateralPort = INDEX_NONE;
                for (int32 Port = 0; Port < Burns.Num(); ++Port)
                    if (const auto* Definition = RCS->GetPort(Port); Definition && Definition->Direction == EThrusterPortDir::RIGHT)
                    {
                        LateralPort = Port;
                        break;
                    }
                if (!TestTrue(TEXT("Stormhawk has a right-facing thruster"), LateralPort != INDEX_NONE))
                    return false;
                Runtime.GetThruster()->ExecTrans(-Design->trans_x, 0.0, 0.0);
                for (int32 Port = 0; Port < Burns.Num(); ++Port)
                    Burns[Port] = static_cast<uint8>(FMath::RoundToInt(255.0f * RCS->GetThrusterBurn(Port)));
                TestTrue(TEXT("Authoritative strafe lights the right-facing jet"), Burns[LateralPort] > 0);
                TestTrue(TEXT("Authoritative strafe applies lateral force"), Runtime.GetTransX() < 0.0);
                Runtime.Physical::ExecFrame(0.05);
                TestTrue(TEXT("Lateral force changes flight velocity"), !Runtime.GetVelocity().IsNearlyZero());
                Proxy->UpdateSnapshotPropulsion(0.1f, 0.017f, 1.7f, Burns);
                const auto& Effects = Proxy->RuntimeThrusterFX;
                if (TestTrue(TEXT("Client builds every authoritative Stormhawk RCS port"), Effects.Num() == Burns.Num()))
                {
                    const auto* Trail = Effects[LateralPort].Trail;
                    if (TestNotNull(TEXT("Client lateral jet has a trail"), Trail))
                    {
                        TestTrue(TEXT("Replicated burn displays the lateral exhaust"), Trail->IsVisible());
                        TestTrue(TEXT("Lateral exhaust retains width after port scaling"),
                            Effects[LateralPort].Nozzle && FMath::IsNearlyEqual(Trail->GetComponentScale().Y,Effects[LateralPort].Nozzle->GetComponentScale().X*50.,1.e-4));
                    }
                    TestEqual(TEXT("An unfired fore jet stays idle"), Effects[0].PresentedBurn, 0.0f);
                    for(int32 I=0;I<Effects.Num();++I)
                        if(Burns[I]==0)TestEqual(TEXT("Main throttle cannot light an unfired maneuvering jet"),Effects[I].PresentedBurn,0.f);
                }
                TestTrue(TEXT("Client displays exhaust at low main throttle"),
                    Proxy->RuntimeMainEngineEmitters.ContainsByPredicate([](const UNiagaraComponent* Engine)
                    {
                        return Engine && Engine->IsVisible();
                    }));
                Proxy->UpdateSnapshotPropulsion(.1f,0,0,Burns);
                if(Effects.IsValidIndex(LateralPort) && Effects[LateralPort].Trail)
                {
                    const auto* Jet=Effects[LateralPort].Trail;
                    TestTrue(TEXT("Native maneuvering jet fires with main throttle at zero"),Jet->IsVisible() && Effects[LateralPort].PresentedBurn>0);
                    TestTrue(TEXT("Maneuvering port uses the native faction nozzle system"),Jet->GetAsset() && Jet->GetAsset()->GetPathName().StartsWith(TEXT("/Game/Effects/Engines/NS_SSW_Nozzle_")));
                    bool Valid=false;const float Life=Jet->GetVariableFloat(TEXT("User.Thrusters_Life"),Valid);
                    TestTrue(TEXT("RCS has a shorter native pulse than main-engine exhaust"),Valid && Life<.10f);
                }
                Burns[LateralPort] = 0;
                Proxy->UpdateSnapshotPropulsion(0.1f, 0.0f, 0.0f, Burns);
                if (Effects.IsValidIndex(LateralPort) && Effects[LateralPort].Trail)
                    TestFalse(TEXT("Client stops exhaust when the burn ends"), Effects[LateralPort].Trail->IsVisible());
                Proxy->Destroy();
            }
        }
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
        if(ShipKey==TEXT("Adventure") || ShipKey==TEXT("Marauder") || ShipKey==TEXT("Cutlass"))
            if(TestNotNull(ShipKey+TEXT(" maneuvering system"),RCS))TestEqual(ShipKey+TEXT(" complete maneuvering port set"),RCS->GetNumThrusters(),16);
        if(ShipKey==TEXT("Orion") && RCS)
        {
            TArray<uint8> Burns;Burns.Init(255,RCS->GetNumThrusters());Actor->UpdateSnapshotPropulsion(.1f,1,1,Burns);
            for(int32 I=0;I<Actor->RuntimeThrusterFX.Num();++I)
                if(const auto* Trail=Actor->RuntimeThrusterFX[I].Trail)
                    TestTrue(TEXT("Carrier jets use individual plume size instead of system size"),Actor->RuntimeThrusterFX[I].Nozzle && FMath::IsNearlyEqual(Trail->GetComponentScale().Y,Actor->RuntimeThrusterFX[I].Nozzle->GetComponentScale().X*50.,1.e-4));
        }
        AddInfo(FString::Printf(TEXT("%s: main=%d rcsFX=%d nav=%d"),Entry.Definition,Engines,Thrusters,Lights));
    }
    AddInfo(FString::Printf(TEXT("Fleet mounts checked on %d hulls, %d mounted turrets"), MountShips, MountedTurrets));
    AddInfo(FString::Printf(TEXT("Fleet effects verified on %d imported hulls"),Checked));
    if (!UnverifiedHulls.IsEmpty())
        AddInfo(TEXT("Fleet entries outside fixture coverage: ") + FString::Join(UnverifiedHulls, TEXT(", ")));
    TestTrue(TEXT("All three fleet groups represented with imported hulls"),Checked>=25);
    return true;
}
#endif
