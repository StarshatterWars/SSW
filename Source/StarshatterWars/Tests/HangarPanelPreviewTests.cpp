#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "InsuranceDlg.h"
#include "HangarDlg.h"
#include "ShipFitDlg.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHangarPanelPreviewTest, "Starshatter.Hangar.PanelsWithoutGameInstance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHangarPanelPreviewTest::RunTest(const FString&)
{
    const TArray<TPair<UClass*, FString>> Panels = {
        {UInsuranceDlg::StaticClass(), TEXT("/Game/Screens/inGame/WBP_Insurance.WBP_Insurance_C")},
        {UHangarDlg::StaticClass(), TEXT("/Game/Screens/inGame/WBP_Hangar.WBP_Hangar_C")},
        {UShipFitDlg::StaticClass(), TEXT("/Game/Screens/inGame/WBP_ShipFit.WBP_ShipFit_C")}};
    for (const auto& Panel : Panels)
    {
        UClass* Blueprint = LoadClass<UUserWidget>(nullptr, *Panel.Value);
        if (!TestNotNull(TEXT("Actual panel Blueprint loads"), Blueprint)) continue;
        for (UClass* Class : {Panel.Key, Blueprint})
            for (bool DesignPreview : {false, true})
            {
                TStrongObjectPtr<UUserWidget> Widget(NewObject<UUserWidget>(GetTransientPackage(), Class));
                if (!TestNotNull(TEXT("Panel created without game session"), Widget.Get())) continue;
                Widget->SetDesignerFlags(DesignPreview ? EWidgetDesignFlags::Designing : EWidgetDesignFlags::None);
                if (!TestTrue(TEXT("Initialize the UMG widget tree"), Widget->Initialize())) continue;
                TestNull(TEXT("Reproduction has no game instance"), Widget->GetGameInstance());
                const TSharedRef<SWidget> Slate = Widget->TakeWidget();
                TestTrue(TEXT("Native and Blueprint panels build safely without a game instance"), Slate->GetVisibility().IsVisible());
                Widget->ReleaseSlateResources(true);
            }
    }
    return true;
}
#endif
