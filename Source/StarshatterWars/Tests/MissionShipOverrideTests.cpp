#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Mission.h"
#include "MissionElement.h"
#include "MissionShip.h"
#include "ShipDesignRegistry.h"
#include "GameStructs.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionShipOverridesTest,"Starshatter.Audit.F19.MissionShipOverrides",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMissionShipOverridesTest::RunTest(const FString& Parameters)
{
    const FName Key(TEXT("F19_AuditShip"));
    const auto Original=ShipDesignRegistry::GetAll();
    ON_SCOPE_EXIT {ShipDesignRegistry::Clear();for(const auto& Pair:Original)ShipDesignRegistry::RegisterDesign(Pair.Key,Pair.Value);};
    FShipDesign Design;Design.ShipName=Key.ToString();Design.ShipClass=TEXT("Fighter");ShipDesignRegistry::RegisterDesign(Key,Design);
    FS_CampaignMission Source;Source.Degrees=true;Source.StartTime=TEXT("1/00:00:00");
    FS_MissionElement Element;Element.Name=TEXT("Audit flight");Element.Design=Key.ToString();Element.Count=2;Element.Player=1;
    FS_MissionShip A;A.ShipName=TEXT("Leader");A.RegNum=TEXT("NX-19");A.Location=FVector(11,22,33);A.Heading=90;A.Integrity=42;A.Ammo[0]=0;A.Fuel[0]=17;
    Element.Ship.Add(A);Element.Ship.Add(FS_MissionShip());Source.Element.Add(Element);
    Mission Model;Model.LoadFromCampaignMissionData(Source);
    if(!TestEqual(TEXT("Mission element survives conversion"),Model.GetElements().size(),1))return false;
    auto& Ships=Model.GetElements()[0]->GetShips();
    if(!TestEqual(TEXT("Individual ships survive conversion"),Ships.size(),2))return false;
    TestEqual(TEXT("Authored ship name"),FString(ANSI_TO_TCHAR(Ships[0]->GetName())),FString(TEXT("Leader")));
    TestTrue(TEXT("Authored position and degrees"),Ships[0]->GetLocation().Equals(A.Location)&&FMath::IsNearlyEqual(Ships[0]->GetHeading(),UE_PI/2,1.e-6));
    TestEqual(TEXT("Explicit empty ammunition is preserved"),Ships[0]->GetAmmo()[0],0);
    TestEqual(TEXT("Explicit fuel is preserved"),Ships[0]->GetFuel()[0],17);
    TestEqual(TEXT("Omitted ammunition retains inheritance sentinel"),Ships[1]->GetAmmo()[0],-10);
    TestEqual(TEXT("Omitted fuel retains inheritance sentinel"),Ships[1]->GetFuel()[0],-10);
    return true;
}
#endif
