#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "ShipActor.h"
#include "ShipDesign.h"
#include "Drive.h"
#include "Engine/World.h"
#include "NiagaraComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCapitalExhaustScaleTest,"Starshatter.Ships.CapitalExhaustPresentationScale",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCapitalExhaustScaleTest::RunTest(const FString&)
{
    const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false)
        .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    auto* World=UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
    if(!TestNotNull(TEXT("Exhaust fixture world"),World))return false;
    ON_SCOPE_EXIT {World->DestroyWorld(false);};
    for(float LegacyScale:{1.15f,5.f,7.f,8.f})
    {
        ShipDesign Design; FCStringAnsi::Strncpy(Design.name,"Capital exhaust fixture",UE_ARRAY_COUNT(Design.name));
        auto* MainDrive=new Drive(EDriveType::YELLOW,2500,0,true);
        MainDrive->CreatePort(FVector(-100,0,0),LegacyScale); Design.drives.append(MainDrive);
        auto* Actor=World->SpawnActor<AShipActor>();
        Actor->InitializeSnapshotPropulsion(&Design);
        Actor->UpdateSnapshotPropulsion(1.f,1.f,100.f,{},0xffffffff);
        TArray<UNiagaraComponent*> Components; Actor->GetComponents(Components);
        int32 Engines=0;
        for(auto* Component:Components)
            if(Component->GetName().StartsWith(TEXT("RuntimeMainEngineEmitter_")))
            {
                ++Engines;
                const FVector Scale=Component->GetRelativeScale3D();
                TestTrue(TEXT("Legacy flare does not inflate canonical nozzle length"),FMath::IsNearlyEqual(Scale.X,.5,.001));
                TestTrue(TEXT("Legacy flare does not inflate canonical nozzle width"),FMath::IsNearlyEqual(Scale.Y,.5,.001) && FMath::IsNearlyEqual(Scale.Z,.5,.001));
                TestTrue(TEXT("Exhaust remains visible at full burn"),Component->IsVisible() && Scale.GetMin()>0);
            }
        TestEqual(TEXT("Main-engine emitter is retained"),Engines,1);
        TestEqual(TEXT("Legacy port definition unchanged"),MainDrive->GetPortScale(0),LegacyScale);
        TestEqual(TEXT("Engine performance unchanged"),MainDrive->MaxThrust(),2500.f);
        Actor->Destroy();
    }
    return true;
}
#endif
