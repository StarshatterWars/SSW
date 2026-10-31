#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Engine/DataTable.h"
#include "GameStructs_System.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAllianceThrusterLayoutTest,"Starshatter.Fighters.AllianceThrusterCoverage",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAllianceThrusterLayoutTest::RunTest(const FString& Parameters)
{
    auto* Table=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_ShipDesign.DT_ShipDesign"));
    if(!TestNotNull(TEXT("Ship design data"),Table))return false;
    for(const TCHAR* Name:{TEXT("Falcon"),TEXT("Eagle"),TEXT("Thunderbolt"),TEXT("Stormhawk"),TEXT("Viper"),TEXT("Razor"),TEXT("Cobra")})
    {
        const auto* Row=Table->FindRow<FShipDesign>(FName(Name),TEXT("Alliance RCS audit"));
        if(!TestNotNull(Name,Row))continue;
        TestTrue(TEXT("Fighter has an authored main drive"),!Row->Drive.IsEmpty());
        TestTrue(TEXT("Fighter has its RCS system"),!Row->Thruster.IsEmpty());
        int32 Mask=0;TSet<EThrusterPortDir> Directions;
        for(const auto& System:Row->Thruster)
            for(const auto& Port:System.Ports)
            {
                Mask|=Port.Fire;Directions.Add(Port.Direction);
                TestFalse(TEXT("RCS location is finite"),Port.Location.ContainsNaN());
                FVector Expected=FVector::ZeroVector;
                switch(Port.Direction)
                {
                case EThrusterPortDir::FORE:Expected=FVector(1,0,0);break;
                case EThrusterPortDir::AFT:Expected=FVector(-1,0,0);break;
                case EThrusterPortDir::TOP:Expected=FVector(0,0,1);break;
                case EThrusterPortDir::BOTTOM:Expected=FVector(0,0,-1);break;
                case EThrusterPortDir::LEFT:Expected=FVector(0,-1,0);break;
                case EThrusterPortDir::RIGHT:Expected=FVector(0,1,0);break;
                default:break;
                }
                if(!Expected.IsNearlyZero())TestTrue(TEXT("Nozzle points outward in the authored direction"),FVector::DotProduct(Port.Rotation.Vector(),Expected)>0.99);
            }
        TestEqual(TEXT("Jets cover translation, pitch, yaw and roll controls"),Mask&0xfff,0xfff);
        TestEqual(TEXT("Jets cover all six hull-facing directions"),Directions.Num(),6);
    }
    return true;
}
#endif
