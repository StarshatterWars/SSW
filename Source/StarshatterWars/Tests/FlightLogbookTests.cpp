#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "FlightLogbookDlg.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "PlayerCharacter.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
    void CollectLogbookText(const TSharedRef<SWidget>& Widget, TArray<FString>& Text)
    {
        if (Widget->GetType() == FName(TEXT("STextBlock")))
            Text.Add(StaticCastSharedRef<STextBlock>(Widget)->GetText().ToString());
        if (auto* Children = Widget->GetChildren())
            for (int32 Index = 0; Index < Children->Num(); ++Index)
                CollectLogbookText(Children->GetChildAt(Index), Text);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlightLogbookContentTest, "Starshatter.Mission.FlightLogbookContent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlightLogbookContentTest::RunTest(const FString&)
{
    auto* Registered = LoadClass<UFlightLogbookDlg>(nullptr, TEXT("/Game/Screens/inGame/WBP_Player.WBP_Player_C"));
    if (!TestNotNull(TEXT("Registered player popup Blueprint derives from FlightLogbookDlg"), Registered))
        return false;

    PlayerCharacter Pilot;
    Pilot.SetName(TEXT("Test Pilot"));
    Pilot.SetCallsign(TEXT("vanguard"));
    auto* Prior = PlayerCharacter::GetCurrentPlayer();
    PlayerCharacter::SetCurrentPlayer(&Pilot);
    ON_SCOPE_EXIT { PlayerCharacter::SetCurrentPlayer(Prior); };

    const auto* Popup = NewObject<UFlightLogbookDlg>(GetTransientPackage(), Registered);
    TArray<FString> Text;
    CollectLogbookText(Popup->BuildPanelContent(nullptr), Text);
    TestTrue(TEXT("Panel has the pilot's callsign"), Text.Contains(TEXT("Vanguard")));
    TestTrue(TEXT("Panel displays service history"), Text.Contains(TEXT("Service record")));
    TestTrue(TEXT("Panel displays decorations"), Text.Contains(TEXT("Decorations")));
    TestTrue(TEXT("Player panel exposes the GMT universe clock"),Text.Contains(TEXT("Universe time (GMT)")));
    TestTrue(TEXT("Player panel exposes current command group"),Text.Contains(TEXT("Current group")));
    TestTrue(TEXT("Player panel exposes team score"),Text.Contains(TEXT("Team score")));
    TestTrue(TEXT("Player panel exposes campaign elapsed time"),Text.Contains(TEXT("Campaign elapsed")));
    return true;
}
#endif
