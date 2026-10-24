#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "MissionEditorDlg.h"
#include "Mission.h"
#include "MissionElement.h"
#include "ShipDesignRegistry.h"
#include "GameStructs.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionRollbackReferencesTest,"Starshatter.Audit.F21.RollbackReferences",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMissionRollbackReferencesTest::RunTest(const FString& Parameters)
{
    const auto Original=ShipDesignRegistry::GetAll();ON_SCOPE_EXIT {ShipDesignRegistry::Clear();for(const auto& Pair:Original)ShipDesignRegistry::RegisterDesign(Pair.Key,Pair.Value);};
    FShipDesign Design;Design.ShipName=TEXT("P2_Rollback");Design.ShipClass=TEXT("Fighter");ShipDesignRegistry::RegisterDesign(FName(TEXT("P2_Rollback")),Design);
    Mission Model;auto* A=new MissionElement;auto* B=new MissionElement;
    A->SetName("Target");B->SetName("Ward");A->SetShipDesign(ShipDesignRegistry::Find(FName(TEXT("P2_Rollback"))));B->SetShipDesign(ShipDesignRegistry::Find(FName(TEXT("P2_Rollback"))));A->SetPlayer(1);
    Model.GetElements().append(A);Model.GetElements().append(B);Model.SetTarget(A);Model.SetWard(B);
    auto* Editor=NewObject<UMissionEditorDlg>();Editor->SetMission(&Model);A->SetName("Changed");Model.SetTarget(nullptr);Model.SetWard(nullptr);Editor->CancelEditing();
    TestTrue(TEXT("Cancel restores mission target identity"),Model.GetTarget()&&FString(ANSI_TO_TCHAR(Model.GetTarget()->GetName()))==TEXT("Target"));
    TestTrue(TEXT("Cancel restores mission ward identity"),Model.GetWard()&&FString(ANSI_TO_TCHAR(Model.GetWard()->GetName()))==TEXT("Ward"));
    TestTrue(TEXT("Restored references point into restored element list"),Model.GetTarget()==Model.GetElements()[0]&&Model.GetWard()==Model.GetElements()[1]);
    Editor->SetMission(nullptr);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissingShipDesignTest,"Starshatter.Audit.F27.MissingDesign",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMissingShipDesignTest::RunTest(const FString& Parameters)
{
    FS_CampaignMission Source;Source.StartTime=TEXT("1/00:00:00");FS_MissionElement E;E.Name=TEXT("Unknown ship");E.Design=TEXT("P2_Missing_Authored_Design");E.Player=1;Source.Element.Add(E);
    AddExpectedError(TEXT("Missing FShipDesign"),EAutomationExpectedErrorFlags::Contains,1);
    Mission Model;TestFalse(TEXT("Missing authored design prevents mission launch"),Model.LoadFromCampaignMissionData(Source));
    TestFalse(TEXT("Missing design never registers an invented fighter"),ShipDesignRegistry::Has(FName(*E.Design)));return true;
}
#endif
