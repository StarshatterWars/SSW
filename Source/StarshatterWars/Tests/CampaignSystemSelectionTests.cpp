#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "CampaignSystemSelectionRules.h"
#include "GalaxyMapPanel.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"

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
    TArray<StarSystem*> Galaxy = { &Solus, &Loris };
    Panel->LoadFromRuntimeSystems(Galaxy);
    TestEqual(TEXT("All galaxy systems retained for rendering"), Panel->SystemLookup.Num(), 2);
    TestTrue(TEXT("Non-campaign system has a rendered marker position"), Panel->CachedSystemPositions.Contains(TEXT("Loris")));
    const FVector2D PanelSize(1024.f, 768.f);
    const FGeometry Geometry = FGeometry::MakeRoot(PanelSize, FSlateLayoutTransform());
    auto Click = [&Panel, &Geometry, PanelSize](const FString& Name, bool bDoubleClick)
    {
        const FVector2D Position = Panel->ApplyViewTransformToPoint(Panel->CachedSystemPositions[Name], PanelSize);
        const FPointerEvent Mouse(0, Position, Position, TSet<FKey>(), EKeys::LeftMouseButton, 0.f, FModifierKeysState());
        if (bDoubleClick) Panel->NativeOnMouseButtonDoubleClick(Geometry, Mouse);
        else Panel->NativeOnMouseButtonDown(Geometry, Mouse);
    };
    int32 SelectedCount = 0, ActivatedCount = 0;
    Panel->OnSystemSelected.BindLambda([&SelectedCount](const FString&) { ++SelectedCount; });
    Panel->OnSystemActivated.BindLambda([&ActivatedCount](const FString&) { ++ActivatedCount; });
    Click(TEXT("Solus"), false);
    TestEqual(TEXT("Campaign marker click selects"), Panel->GetSelectedSystem(), FString(TEXT("Solus")));
    TestEqual(TEXT("Allowed click dispatches selection"), SelectedCount, 1);
    Click(TEXT("Loris"), false);
    Click(TEXT("Loris"), true);
    TestEqual(TEXT("Blocked clicks retain allowed selection"), Panel->GetSelectedSystem(), FString(TEXT("Solus")));
    TestEqual(TEXT("Blocked clicks do not dispatch selection"), SelectedCount, 1);
    TestEqual(TEXT("Blocked double click cannot open system view"), ActivatedCount, 0);
    Click(TEXT("Solus"), true);
    TestEqual(TEXT("Allowed double click opens system view"), ActivatedCount, 1);
    Panel->SetSelectedSystem(TEXT("Solus"));
    Panel->SetSelectedSystem(TEXT("Loris"));
    TestEqual(TEXT("Programmatic blocked selection retains allowed selection"), Panel->GetSelectedSystem(), FString(TEXT("Solus")));
    Panel->SetSelectedSystem(TEXT("Loris"), true);
    TestEqual(TEXT("Autofocus overload cannot bypass restriction"), Panel->GetSelectedSystem(), FString(TEXT("Solus")));
    Active = &Second;
    Panel->NativeTick(Geometry, 0.f);
    TestTrue(TEXT("Campaign switch clears unavailable selection highlight"), Panel->GetSelectedSystem().IsEmpty());
    TestEqual(TEXT("Switch keeps every galaxy marker"), Panel->CachedSystemPositions.Num(), 2);
    TestFalse(TEXT("Switch revokes previous system immediately"), Panel->CanSelectSystem(TEXT("Solus")));
    TestTrue(TEXT("Switch permits new campaign system"), Panel->CanSelectSystem(TEXT("Loris")));
    Panel->SetSelectedSystem(TEXT(""));
    TestTrue(TEXT("Stale selection can always be cleared"), Panel->GetSelectedSystem().IsEmpty());
    Click(TEXT("Loris"), false);
    TestEqual(TEXT("Switched campaign can select its system"), Panel->GetSelectedSystem(), FString(TEXT("Loris")));
    Panel->OnSystemSelected.Unbind();
    Panel->OnSystemActivated.Unbind();
    Panel->OnCanSelectSystem.Unbind();
    Panel->SetSelectedSystem(TEXT("NonCampaignSystem"));
    TestEqual(TEXT("Unrestricted shared map retains mission navigation behavior"), Panel->GetSelectedSystem(), FString(TEXT("NonCampaignSystem")));
    TestEqual(TEXT("First campaign's underlying systems preserved"), First.GetSystemList().size(), 1);
    TestEqual(TEXT("Second campaign's underlying systems preserved"), Second.GetSystemList().size(), 1);
    return true;
}
#endif
