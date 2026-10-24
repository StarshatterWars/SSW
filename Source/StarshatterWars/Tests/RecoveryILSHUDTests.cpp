#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "FighterHUDDetails.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryILSHUDTest,"Starshatter.Audit.F01.ILSHUD", EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecoveryILSHUDTest::RunTest(const FString& Parameters)
{
    TSharedRef<SFighterHUDDetails> HUD = SNew(SFighterHUDDetails).ShowHUD(true);
    TestTrue(TEXT("Native ILS texture loads"),HUD->ILSTexture.IsValid());
    HUD->UpdateILSMode(10,false);
    TestFalse(TEXT("Initially off"),HUD->bILSMode);
    HUD->UpdateILSMode(10,true);
    TestTrue(TEXT("Inbound assignment enables ILS"),HUD->bILSMode);
    HUD->bILSMode=false;
    HUD->UpdateILSMode(10,true);
    TestFalse(TEXT("Manual hide survives HUD ticks"),HUD->bILSMode);
    HUD->UpdateILSMode(10,false);HUD->UpdateILSMode(10,true);
    TestTrue(TEXT("New inbound assignment enables ILS again"),HUD->bILSMode);
    HUD->UpdateILSMode(11,false);
    TestFalse(TEXT("Changing player resets mode"),HUD->bILSMode);
    HUD->bILSMode=true;HUD->UpdateILSMode(INDEX_NONE,false);
    TestFalse(TEXT("Mission end clears mode"),HUD->bILSMode);
    return true;
}
#endif
