#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "StationServicesAI.h"
#include "StarshatterAssetRegistrySubsystem.h"
#include "StationServicesDlg.h"
#include "Engine/GameInstance.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStationServicesAccessTest,"Starshatter.StationServices.DockedContextAccess",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStationServicesAccessTest::RunTest(const FString&)
{
    auto* GameInstance=NewObject<UGameInstance>();
    auto* Registry=NewObject<UStarshatterAssetRegistrySubsystem>(GameInstance);
    if(!TestTrue(TEXT("Actual panel registry initializes"),Registry->InitRegistry()))return false;
    auto* PanelClass=Registry->GetWidgetClass(TEXT("UI.StationServicesPopupClass"),true).Get();
    if(!TestNotNull(TEXT("Station Services key resolves before live play"),PanelClass))return false;
    TestTrue(TEXT("Registered panel uses the native StationServicesDlg driver"),PanelClass->IsChildOf(UStationServicesDlg::StaticClass()));
    FStationDockContext Context;Context.Design=TEXT("Orion");Context.Facility=TEXT("Archon");FText Reason;
    TestFalse(TEXT("Carrier capabilities do not grant services while flying"),StationServicesAI::CanUse(Context,ESSWStationService::Refit,Reason));
    Context.bDocked=true;Context.ActualDock=Context.Facility;
    TestFalse(TEXT("Docked carrier cannot provide shipyard refit"),StationServicesAI::CanUse(Context,ESSWStationService::Refit,Reason));
    TestTrue(TEXT("Docked carrier provides repair"),StationServicesAI::CanUse(Context,ESSWStationService::Repair,Reason));
    Context.Design=TEXT("Station4");Context.Facility=TEXT("Fortress");Context.Name=TEXT("Fortress");
    TestFalse(TEXT("Docking elsewhere cannot use the viewed fortress"),StationServicesAI::CanUse(Context,ESSWStationService::Refit,Reason));
    Context.ActualDock=Context.Facility;
    TestTrue(TEXT("Docking at the viewed shipyard enables refit"),StationServicesAI::CanUse(Context,ESSWStationService::Refit,Reason));
    Context.bDocked=false;
    TestFalse(TEXT("Undocking immediately removes access without hiding details"),StationServicesAI::CanUse(Context,ESSWStationService::Refit,Reason));
    TestEqual(TEXT("Viewed facility details retained"),Context.Name,FString(TEXT("Fortress")));
    Context.bDocked=true;
    Context.Design=TEXT("Mining Station");
    TestFalse(TEXT("Docking does not bypass mining station capability"),StationServicesAI::CanUse(Context,ESSWStationService::Refit,Reason));
    Context.Design=TEXT("Unknown");
    TestFalse(TEXT("Unknown dock cannot acquire market capability"),StationServicesAI::CanUse(Context,ESSWStationService::Market,Reason));
    TSet<ESSWStationService> Seen;
    for(const auto& Entry:StationServicesAI::Descriptors())
    {TestFalse(TEXT("Each service descriptor unique"),Seen.Contains(Entry.Service));Seen.Add(Entry.Service);TestTrue(TEXT("Each destination is registered popup key"),FString(Entry.RegistryKey).StartsWith(TEXT("UI.")));}
    TestEqual(TEXT("All authored capability types represented"),Seen.Num(),int32(ESSWStationService::HullSales)+1);return true;
}
#endif
