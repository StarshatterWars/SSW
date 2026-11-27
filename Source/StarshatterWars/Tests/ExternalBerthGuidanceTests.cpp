#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "ExternalBerthGuidance.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "Camera.h"
#include "DataLoader.h"
#include "Sim.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExternalBerthGuidanceTest,"Starshatter.Facilities.ExternalBerthILS",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FExternalBerthGuidanceTest::RunTest(const FString&)
{
    const bool OwnLoader=!DataLoader::GetLoader();if(OwnLoader)DataLoader::Initialize();
    ON_SCOPE_EXIT{if(OwnLoader)DataLoader::Close();};
    TUniquePtr<Sim> OwnedSim;if(!Sim::GetSim())OwnedSim=MakeUnique<Sim>(nullptr);
    ShipDesign Definition;Definition.type=static_cast<int>(CLASSIFICATION::STATION);Definition.radius=500;
    Ship Station("External berth fixture","",&Definition,1,0,nullptr,false);
    Station.SetNavigationHullRadius(5000);Camera Pose;Pose.MoveTo(FVector(1300,-700,250));Pose.Yaw(.6);Pose.Pitch(.25);Pose.Roll(.35);Station.CloneCam(Pose);
    for(int32 Index=0;Index<2;++Index)
    {
        FVector Berth;FExternalBerthGuidance Guide;
        TestTrue(TEXT("Docking-only station offers external berth"),Station.GetTransferBerth(Index,80,Berth));
        TestTrue(TEXT("Native guidance for berth"),Station.GetExternalBerthGuidance(Index,80,Guide));
        TestTrue(TEXT("ILS endpoint is the exact stock-transfer endpoint"),Guide.Berth.Equals(Berth,1.e-6));
        TestTrue(TEXT("Approach points outward, away from station hull"),FVector::DotProduct(Guide.Center(3)-Berth,Berth-Station.GetLocation())>0);
        TestTrue(TEXT("Corridor frame is orthonormal"),FMath::Abs(FVector::DotProduct(Guide.Right,Guide.Up))<1.e-6 && FMath::Abs(FVector::DotProduct(Guide.Right,Guide.Outward))<1.e-6);
        for(int32 Hoop=0;Hoop<4;++Hoop)
        {
            FVector Points[4];TestTrue(TEXT("Four approach hoops"),Guide.Corners(Hoop,Points));
            FVector Center=FVector::ZeroVector;for(const auto& Point:Points)Center+=Point/4;
            TestTrue(TEXT("Hoop plane follows exact native corridor"),Center.Equals(Guide.Center(Hoop),1.e-6));
            TestTrue(TEXT("Hoops increase from near berth to outer marker"),FMath::IsNearlyEqual(FVector::Distance(Center,Berth),Guide.Length*(Hoop+1)/4,1.e-6));
        }
        FVector Points[4];TestFalse(TEXT("Invalid hoop rejected"),Guide.Corners(4,Points));
    }
    FExternalBerthGuidance Invalid;
    TestFalse(TEXT("Invalid berth index rejected"),Station.GetExternalBerthGuidance(2,20,Invalid));
    TestFalse(TEXT("Invalid craft radius rejected"),Station.GetExternalBerthGuidance(0,-1,Invalid));
    TestFalse(TEXT("Parallel frame rejected"),Invalid.Initialize(FVector::ZeroVector,FVector::UpVector,FVector::UpVector,10));
    TestFalse(TEXT("Zero approach rejected"),Invalid.Initialize(FVector::ZeroVector,FVector::ZeroVector,FVector::RightVector,10));
    return true;
}
#endif
