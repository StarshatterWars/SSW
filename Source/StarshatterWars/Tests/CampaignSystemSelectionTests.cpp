#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "CampaignSystemSelectionRules.h"
#include "GalaxyMapPanel.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignSystemSelectionTest,
    "Starshatter.Campaign.TheaterSystemSelection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCampaignSystemSelectionTest::RunTest(const FString& Parameters)
{
    StarSystem Solus("Solus", FVector::ZeroVector);
    StarSystem Loris("Loris", FVector(100, 0, 0));
    Campaign First(1, "First fixture", true);
    Campaign Second(2, "Second fixture", true);
    First.GetSystemList().append(&Solus);
    Second.GetSystemList().append(&Loris);
    const Campaign* Active = &First;
    TestTrue(TEXT("Campaign's authored system allowed, case/whitespace tolerant"),
        CampaignSystemSelectionRules::CanSelect(Active, TEXT(" solus ")));
    TestFalse(TEXT("Other existing galaxy system unavailable"),
        CampaignSystemSelectionRules::CanSelect(Active, TEXT("Loris")));
    TestFalse(TEXT("No campaign does not unlock the galaxy"),
        CampaignSystemSelectionRules::CanSelect(nullptr, TEXT("Solus")));
    TestFalse(TEXT("Empty input rejected"), CampaignSystemSelectionRules::CanSelect(Active, TEXT(" ")));

    UGalaxyMapPanel* Panel = NewObject<UGalaxyMapPanel>();
    Panel->OnCanSelectSystem.BindLambda([&Active](const FString& Name)
    { return CampaignSystemSelectionRules::CanSelect(Active, Name); });
    Panel->SetSelectedSystem(TEXT("Solus"));
    Panel->SetSelectedSystem(TEXT("Loris"));
    TestEqual(TEXT("Programmatic blocked selection retains allowed selection"), Panel->GetSelectedSystem(), FString(TEXT("Solus")));
    Panel->SetSelectedSystem(TEXT("Loris"), true);
    TestEqual(TEXT("Autofocus overload cannot bypass restriction"), Panel->GetSelectedSystem(), FString(TEXT("Solus")));
    Active = &Second;
    TestFalse(TEXT("Switch revokes previous system immediately"), Panel->CanSelectSystem(TEXT("Solus")));
    TestTrue(TEXT("Switch permits new campaign system"), Panel->CanSelectSystem(TEXT("Loris")));
    Panel->SetSelectedSystem(TEXT(""));
    TestTrue(TEXT("Stale selection can always be cleared"), Panel->GetSelectedSystem().IsEmpty());
    Panel->SetSelectedSystem(TEXT("Loris"));
    TestEqual(TEXT("Switched campaign can select its system"), Panel->GetSelectedSystem(), FString(TEXT("Loris")));
    Panel->OnCanSelectSystem.Unbind();
    Panel->SetSelectedSystem(TEXT("NonCampaignSystem"));
    TestEqual(TEXT("Unrestricted shared map retains mission navigation behavior"), Panel->GetSelectedSystem(), FString(TEXT("NonCampaignSystem")));
    TestEqual(TEXT("First campaign's underlying systems preserved"), First.GetSystemList().size(), 1);
    TestEqual(TEXT("Second campaign's underlying systems preserved"), Second.GetSystemList().size(), 1);
    return true;
}
#endif
