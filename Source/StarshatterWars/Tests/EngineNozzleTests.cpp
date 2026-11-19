#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "EnginePlumeTuning.h"
#include "NativeShipGeometry.h"
#include "ShipActor.h"
#include "ShipDesignRegistry.h"
#include "Drive.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEngineNozzleGeometryTest,"Starshatter.Ships.Nozzle.GeometryAndScale",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FEngineNozzleGeometryTest::RunTest(const FString&)
{
    const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false)
        .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    auto* World=UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
    if(!TestNotNull(TEXT("Nozzle world"),World))return false;
    ON_SCOPE_EXIT {World->DestroyWorld(false);};
    auto* Actor=World->SpawnActor<AShipActor>();
    auto* Cylinder=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if(!TestNotNull(TEXT("Known circular engine rim mesh"),Cylinder))return false;
    Actor->HullMesh->SetStaticMesh(Cylinder);
    const FBox Bounds=Cylinder->GetBoundingBox();
    const FVector Rim(Bounds.GetCenter().X,Bounds.GetCenter().Y,Bounds.Max.Z);
    const double Nominal=FMath::Min(Bounds.GetExtent().X,Bounds.GetExtent().Y);
    FNativeShipGeometry Geometry;
    const auto Measure=[&](double Range)
    {
        const auto Pose=Actor->HullMesh->GetComponentTransform();
        return Geometry.NozzleRadius(*Actor,Pose.TransformPosition(Rim),Pose.TransformVectorNoScale(FVector::UpVector),Range);
    };
    const double First=Measure(Nominal*2);
    AddInfo(FString::Printf(TEXT("Cylinder rim: nominal=%g measured=%g bounds=%s"),Nominal,First,*Bounds.ToString()));
    TestTrue(TEXT("Rim measurement is positive and inside known cylinder radius"),First>Nominal*.70 && First<=Nominal);
    Actor->SetActorScale3D(FVector(3));
    const double Second=Measure(Nominal*6);
    TestTrue(TEXT("World-space nozzle radius includes actor scale exactly once"),FMath::IsNearlyEqual(Second,First*3,.01));
    TestEqual(TEXT("Neighboring nozzles cannot have overlapping plume bases"),EnginePlumeTuning::Radius(100,10,20,1000),8.);
    TestEqual(TEXT("Invalid geometry uses a bounded fallback"),EnginePlumeTuning::Radius(-1,5,0,100),5.);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEngineNozzleFactionTest,"Starshatter.Ships.Nozzle.FactionAndNativeAssets",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FEngineNozzleFactionTest::RunTest(const FString&)
{
    const auto Previous=ShipDesignRegistry::GetAll();
    ON_SCOPE_EXIT {ShipDesignRegistry::Clear();for(const auto& Pair:Previous)ShipDesignRegistry::RegisterDesign(Pair.Key,Pair.Value);};
    const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false)
        .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    auto* World=UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
    if(!TestNotNull(TEXT("Faction engine world"),World))return false;
    ON_SCOPE_EXIT {World->DestroyWorld(false);};
    for(EShipEmpire Empire:{EShipEmpire::Terellian,EShipEmpire::Dantari,EShipEmpire::Marakan})
    {
        FShipDesign Row;Row.ShipName=TEXT("NozzleColorFixture");Row.ShipEmpire=Empire;
        ShipDesignRegistry::RegisterDesign(FName(TEXT("NozzleColorFixture")),Row);
        ShipDesign Design;FCStringAnsi::Strncpy(Design.name,"NozzleColorFixture",UE_ARRAY_COUNT(Design.name));
        auto* Drive=new class Drive(EDriveType::RED,2500,0,true);Drive->CreatePort(FVector::ZeroVector,800);Design.drives.append(Drive);
        auto* Actor=World->SpawnActor<AShipActor>();Actor->MainEngineEmitterRelativeScale=FVector(999);
        Actor->InitializeSnapshotPropulsion(&Design);Actor->UpdateSnapshotPropulsion(1,1,100,{},0xffffffff);
        TArray<UNiagaraComponent*> Emitters;Actor->GetComponents(Emitters);int32 Found=0;
        for(auto* Emitter:Emitters)if(Emitter->GetName().StartsWith(TEXT("RuntimeMainEngineEmitter_")))
        {
            ++Found;const auto* System=Emitter->GetAsset();
            if(TestNotNull(TEXT("Native nozzle Niagara system exists"),System))
            {
                TestEqual(TEXT("Design faction overrides mismatched legacy drive color"),System->GetPathName(),EnginePlumeTuning::SystemPath(Empire,EDriveType::RED));
                const FNiagaraVariable Radius(FNiagaraTypeDefinition::GetFloatDef(),TEXT("User.Thrusters_Radius"));
                const auto* Data=System->GetExposedParameters().GetParameterData(Radius);
                if(TestNotNull(TEXT("Plume really binds authored nozzle radius"),Data))TestEqual(TEXT("Canonical Niagara nozzle radius"),*reinterpret_cast<const float*>(Data),.65f);
            }
            TestTrue(TEXT("No duplicate parent scale"),Emitter->IsUsingAbsoluteScale());
            TestTrue(TEXT("Legacy 800x flare and 999x BP multipliers cannot inflate the flame"),Emitter->GetRelativeScale3D().Equals(FVector(.5),.001));
            bool Valid=false;
            const float Brightness=Emitter->GetVariableFloat(TEXT("User.Emissive_Boost"),Valid);
            TestTrue(TEXT("Full-power visibility is bound to Niagara and brighter than the previous plume"),Valid && Brightness>=3.f);
            const float Density=Emitter->GetVariableFloat(TEXT("User.Thrusters_SpawnRate"),Valid);
            TestTrue(TEXT("Denser plume remains bounded for fleet rendering"),Valid && Density>450.f && Density<=750.f);
        }
        TestEqual(TEXT("One plume for one nozzle"),Found,1);
        TestEqual(TEXT("Drive thrust remains unchanged"),Drive->MaxThrust(),2500.f);
        TestEqual(TEXT("Legacy port remains unchanged"),Drive->GetPortScale(0),800.f);
        TArray<UPointLightComponent*> Lights;Actor->GetComponents(Lights);
        for(auto* Light:Lights)TestTrue(TEXT("Idle glow uses same faction palette"),Light->GetLightColor().Equals(EnginePlumeTuning::Color(Empire,EDriveType::RED),.01));
        Actor->UpdateSnapshotPropulsion(1,.1f,10,{},0xffffffff);
        const float LowPower=Actor->MainEngineVisualPower;
        Actor->UpdateSnapshotPropulsion(1.f/120.f,1,100,{},0xffffffff);
        TestTrue(TEXT("Visibility ramps rather than jumping to full power"),Actor->MainEngineVisualPower>LowPower && Actor->MainEngineVisualPower<1.f);
        for(auto* Emitter:Emitters)if(Emitter->GetName().StartsWith(TEXT("RuntimeMainEngineEmitter_")))
        {
            TestTrue(TEXT("Brightness does not expand the fitted nozzle width"),FMath::IsNearlyEqual(Emitter->GetRelativeScale3D().Y,.5,.001));
            bool Valid=false;const float Brightness=Emitter->GetVariableFloat(TEXT("User.Emissive_Boost"),Valid);
            TestTrue(TEXT("Low/ramping throttle stays visible without a full-power flash"),Valid && Brightness>1.f && Brightness<3.2f);
        }
        Actor->UpdateSnapshotPropulsion(1,0,0,{},0xffffffff);
        for(auto* Emitter:Emitters)if(Emitter->GetName().StartsWith(TEXT("RuntimeMainEngineEmitter_")))
        {
            bool Valid=false;const float Spawn=Emitter->GetVariableFloat(TEXT("User.Thrusters_SpawnRate"),Valid);
            TestTrue(TEXT("Engine-off plume emits no new particles while the tail retires"),Valid && Spawn==0.f);
            TestFalse(TEXT("Engine-off plume is hidden"),Emitter->IsVisible());
        }
        Actor->Destroy();
    }
    return true;
}
#endif
