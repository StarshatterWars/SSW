#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "CmdIntelDlg.h"
#include "IntelLVElement.h"
#include "IntelListObject.h"
#include "CombatEvent.h"
#include "Campaign.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIntelNativeReportsTest, "Starshatter.Intel.NativeReports",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FIntelNativeReportsTest::RunTest(const FString& Parameters)
{
    const auto Options = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false)
        .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Options);
    if (!TestNotNull(TEXT("Fixture world"), World)) return false;
    World->SetGameInstance(NewObject<UGameInstance>());
    TStrongObjectPtr<UCmdIntelDlg> Dialog(NewObject<UCmdIntelDlg>(World));
    Dialog->UUserWidget::Initialize();
    auto Slate = Dialog->TakeWidget();
    TestNotNull(TEXT("First display has native root"), Dialog->WidgetTree->RootWidget.Get());
    TestTrue(TEXT("Report list uses native class"), Dialog->IntelList->IsA<UIntelReportList>());
    TestTrue(TEXT("Report entry uses the native presentation implementation"), Dialog->IntelList->GetEntryWidgetClass() && Dialog->IntelList->GetEntryWidgetClass()->IsChildOf(UIntelLVElement::StaticClass()));
    auto ListSlate = Dialog->IntelList->TakeWidget();
    Dialog->SetVisibility(ESlateVisibility::Hidden); // background refresh must not consume unread events
    Campaign Story(2, "Intel fixture", true);
    auto* First = new CombatEvent(&Story, ECombatEventType::NONE, 0, 0, ECombatEventSource::NONE, "");
    First->SetTitle("Report one"); First->SetInformation("Preserved report body");
    Story.GetEvents().append(First);
    Dialog->CampaignPtr = &Story;
    Dialog->AppendNewEventsIfAny();
    TestEqual(TEXT("Events populate native list"), Dialog->IntelList->GetNumItems(), 1);
    TestFalse(TEXT("Background refresh does not read report"), First->Visited());
    auto* Item = Cast<UIntelListObject>(Dialog->IntelList->GetItemAt(0));
    Dialog->IntelList->SetSelectedItem(Item);
    TestTrue(TEXT("Fixture list accepts initial selection"), Dialog->IntelList->GetSelectedItem() == Item);
    Dialog->OnNewsSelectionChanged(Item);
    TestTrue(TEXT("Selecting report marks event read"), First->Visited());
    TestEqual(TEXT("Report body preserved"), Dialog->IntelMessageText->GetText().ToString(), FString(TEXT("Preserved report body")));
    auto* Second = new CombatEvent(&Story, ECombatEventType::NONE, 0, 0, ECombatEventSource::NONE, "");
    Second->SetTitle("Report two"); Story.GetEvents().append(Second);
    Dialog->AppendNewEventsIfAny();
    TestEqual(TEXT("New event appears"), Dialog->IntelList->GetNumItems(), 2);
    auto* Selected = Cast<UIntelListObject>(Dialog->IntelList->GetSelectedItem());
    TestTrue(TEXT("Refresh preserves selected event"), Selected && Selected->EventPtr == First);
    Dialog->AppendNewEventsIfAny();
    TestEqual(TEXT("Refresh does not duplicate reports"), Dialog->IntelList->GetNumItems(), 2);
    TStrongObjectPtr<UIntelLVElement> Row(NewObject<UIntelLVElement>(World, Dialog->IntelList->GetEntryWidgetClass()));
    Row->UUserWidget::Initialize(); auto RowSlate = Row->TakeWidget();
    auto* Box = Cast<USizeBox>(Row->WidgetTree->RootWidget);
    TestTrue(TEXT("Report rows provide readable minimum height"), Box && Box->GetMinDesiredHeight() >= 112.f);
    Campaign Other(3, "Other intel fixture", true);
    Dialog->CampaignPtr = &Other;
    Dialog->AppendNewEventsIfAny();
    TestEqual(TEXT("Campaign change replaces reports with current campaign events"), Dialog->IntelList->GetNumItems(), 0);
    TestNull(TEXT("Old campaign report is no longer selected"), Dialog->IntelList->GetSelectedItem());
    TestTrue(TEXT("Old campaign report body is cleared"), Dialog->IntelMessageText->GetText().IsEmpty());
    Dialog->CampaignPtr = nullptr;
    World->DestroyWorld(false);
    return true;
}
#endif
