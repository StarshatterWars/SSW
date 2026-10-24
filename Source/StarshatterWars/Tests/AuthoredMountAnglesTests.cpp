#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "ShipWeaponMountComponent.h"
#include "Engine/World.h"
#include "BerentsShipActor.h"
#include "HulkTigerShipActor.h"
#include "HulkVolnarisShipActor.h"
#include "ThunderboltShipActor.h"
#include "TigerShipActor.h"
#include "VolnarisShipActor.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuthoredMountAnglesTest,"Starshatter.Audit.F04.AuthoredAngles",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAuthoredMountAnglesTest::RunTest(const FString& Parameters)
{
    const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World=UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
    { auto* Actor=World->SpawnActor<ABerentsShipActor>();
      TestTrue(TEXT("Berents station 2 uses def radians as degrees"), Actor->AuthoredWeaponPointComponents[1]->GetRelativeRotation().Equals(FRotator(0.000000000,-8.594366927,0),0.001));
      TestTrue(TEXT("Berents station 3 uses def radians as degrees"), Actor->AuthoredWeaponPointComponents[2]->GetRelativeRotation().Equals(FRotator(0.000000000,8.594366927,0),0.001));
      TestTrue(TEXT("Berents station 6 uses def radians as degrees"), Actor->AuthoredWeaponPointComponents[5]->GetRelativeRotation().Equals(FRotator(0.000000000,180.000002659,0),0.001));
    }
    { auto* Actor=World->SpawnActor<AHulkTigerShipActor>();
      TestTrue(TEXT("HulkTiger station 5 uses def radians as degrees"), Actor->AuthoredWeaponPointComponents[4]->GetRelativeRotation().Equals(FRotator(0.000000000,179.999847961,0),0.001));
    }
    { auto* Actor=World->SpawnActor<AHulkVolnarisShipActor>();
      TestTrue(TEXT("HulkVolnaris station 6 uses def radians as degrees"), Actor->AuthoredWeaponPointComponents[5]->GetRelativeRotation().Equals(FRotator(0.000000000,179.999847961,0),0.001));
    }
    { auto* Actor=World->SpawnActor<AThunderboltShipActor>();
      TestTrue(TEXT("Thunderbolt station 2 uses def radians as degrees"), Actor->AuthoredWeaponPointComponents[1]->GetRelativeRotation().Equals(FRotator(0.000000000,180.000420918,0),0.001));
    }
    { auto* Actor=World->SpawnActor<ATigerShipActor>();
      TestTrue(TEXT("Tiger station 5 uses def radians as degrees"), Actor->AuthoredWeaponPointComponents[4]->GetRelativeRotation().Equals(FRotator(0.000000000,179.999847961,0),0.001));
    }
    { auto* Actor=World->SpawnActor<AVolnarisShipActor>();
      TestTrue(TEXT("Volnaris station 6 uses def radians as degrees"), Actor->AuthoredWeaponPointComponents[5]->GetRelativeRotation().Equals(FRotator(0.000000000,179.999847961,0),0.001));
    }
    TArray<FNativeWeaponMountDef> Mounts;ShipWeaponMountCatalog::Get(TEXT("Berents"),Mounts);
    TestTrue(TEXT("Berents interceptor catalog retains radians"),FMath::IsNearlyEqual(Mounts[1].Azimuth,-0.15,1.e-9));
    TestTrue(TEXT("Berents aft turret points aft"),FMath::IsNearlyEqual(Mounts[5].Azimuth,UE_PI,1.e-6));
    ShipWeaponMountCatalog::Get(TEXT("Orion"),Mounts);
    TestEqual(TEXT("Orion weapon count preserved"),Mounts.Num(),10);
    World->DestroyWorld(false);return true;
}
#endif

