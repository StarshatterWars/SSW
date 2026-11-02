#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "FighterHUDDetails.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHUDStatusReadoutTest,"Starshatter.HUD.StatusDescriptors",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHUDStatusReadoutTest::RunTest(const FString&)
{
    auto Read=[](double P,bool Aug=false){return SFighterHUDDetails::FormatStatusReadout(TEXT("THRUST"),P,Aug);};
    TestEqual(TEXT("Idle thrust"),Read(0),FString(TEXT("OFF")));
    TestEqual(TEXT("Low thrust"),Read(20),FString(TEXT("LOW")));
    TestEqual(TEXT("Medium thrust"),Read(50),FString(TEXT("MED")));
    TestEqual(TEXT("High thrust"),Read(80),FString(TEXT("HIGH")));
    TestEqual(TEXT("Full thrust"),Read(100),FString(TEXT("FULL")));
    TestEqual(TEXT("Augmenter overrides thrust band"),Read(80,true),FString(TEXT("AUG")));
    TestEqual(TEXT("Near-full gun charge has no misleading 99 percent readout"),SFighterHUDDetails::FormatStatusReadout(TEXT("GUNS"),99),FString(TEXT("FULL")));
    TestEqual(TEXT("Partially charged guns remain high"),SFighterHUDDetails::FormatStatusReadout(TEXT("GUNS"),98),FString(TEXT("HIGH")));
    TestEqual(TEXT("Unknown charge"),Read(-1),FString(TEXT("--")));
    const double Hulls[]={100,99,75,60,30,10,0};
    const TCHAR* States[]={TEXT("INTACT"),TEXT("LIGHT DMG"),TEXT("LIGHT DMG"),TEXT("MOD DMG"),TEXT("HEAVY DMG"),TEXT("CRITICAL"),TEXT("DESTROYED")};
    for(int I=0;I<UE_ARRAY_COUNT(Hulls);++I)
        TestEqual(TEXT("Hull damage state"),SFighterHUDDetails::FormatStatusReadout(TEXT("HULL"),Hulls[I]),FString(States[I]));
    return true;
}
#endif
