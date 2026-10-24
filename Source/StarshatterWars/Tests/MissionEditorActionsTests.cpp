#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Misc/ScopeExit.h"
#include "MissionEditorDlg.h"
#include "ShipDesignRegistry.h"
#include "Mission.h"
#include "MissionElement.h"
#include "MissionEvent.h"
#include "MissionInfo.h"
#include "MissionDebriefDlg.h"
#include "Components/ListView.h"
#include "Components/Button.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionEditorActionsTest,"Starshatter.Audit.F11.MissionEditorActions",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMissionEditorActionsTest::RunTest(const FString& Parameters)
{
    Mission Model; Model.SetName("Original mission");
    MissionInfo Info; Info.name="Original mission"; Info.description="Original description";
    auto* Dlg=NewObject<UMissionEditorDlg>();
    Dlg->LstElem=NewObject<UDebriefListView>(Dlg);
    Dlg->LstEvent=NewObject<UDebriefListView>(Dlg);
    Dlg->BtnElemDel=NewObject<UButton>(Dlg);
    Dlg->BtnElemInc=NewObject<UButton>(Dlg);
    Dlg->BtnElemDec=NewObject<UButton>(Dlg);
    Dlg->SetMission(&Model); Dlg->SetMissionInfo(&Info); Dlg->DrawPackages();
    TestFalse(TEXT("Empty selection disables deletion"),Dlg->BtnElemDel->GetIsEnabled());
    Dlg->OnElemAddClicked(); auto* First=Model.GetElements()[0];First->SetName("First");
    Dlg->OnElemAddClicked(); auto* Second=Model.GetElements()[1];Second->SetName("Second");
    Dlg->DrawPackages();
    TestEqual(TEXT("Rows reflect mission elements"),Dlg->LstElem->GetNumItems(),2);
    TestTrue(TEXT("Selection enables deletion"),Dlg->BtnElemDel->GetIsEnabled());
    Dlg->OnElemIncClicked();TestTrue(TEXT("Promote element swaps order"),Model.GetElements()[0]==Second);
    Dlg->OnElemDecClicked();TestTrue(TEXT("Demote element restores order"),Model.GetElements()[0]==First);
    Dlg->ElementSelected(Dlg->LstElem->GetItemAt(0),true);
    TestFalse(TEXT("First element cannot move up"),Dlg->BtnElemInc->GetIsEnabled());
    TestTrue(TEXT("First element can move down"),Dlg->BtnElemDec->GetIsEnabled());
    Dlg->OnElemDelClicked();TestTrue(TEXT("Deletion removes selected element only"),Model.GetElements().size()==1&&Model.GetElements()[0]==Second);
    Dlg->OnEventAddClicked(); auto* E1=Model.GetEvents()[0];
    Dlg->OnEventAddClicked(); auto* E2=Model.GetEvents()[1];
    TestTrue(TEXT("New event IDs are unique"),E1->EventID()==1&&E2->EventID()==2);
    Dlg->OnEventIncClicked();TestTrue(TEXT("Event priority keeps persistent IDs"),Model.GetEvents()[0]==E2&&E2->EventID()==2);
    Dlg->OnEventDecClicked();Dlg->OnEventDelClicked();
    TestTrue(TEXT("Event deletion preserves other event"),Model.GetEvents().size()==1&&Model.GetEvents()[0]==E1);
    Model.SetName("Changed mission");Info.description="Changed description";
    Dlg->CancelEditing();
    TestEqual(TEXT("Cancel restores mission header"),FString(ANSI_TO_TCHAR(Model.GetName())),FString("Original mission"));
    TestTrue(TEXT("Cancel removes newly added elements and events"),Model.GetElements().size()==0&&Model.GetEvents().size()==0);
    TestEqual(TEXT("Cancel restores metadata"),FString(ANSI_TO_TCHAR(Info.description.data())),FString("Original description"));
    const auto OriginalDesigns=ShipDesignRegistry::GetAll();
    ON_SCOPE_EXIT { ShipDesignRegistry::Clear();for(const auto& Pair:OriginalDesigns) ShipDesignRegistry::RegisterDesign(Pair.Key,Pair.Value); };
    FShipDesign TestDesign;TestDesign.ShipName=TEXT("F11_AuditShip");TestDesign.ShipClass=TEXT("Fighter");TestDesign.Mass=20;TestDesign.Integrity=100;
    ShipDesignRegistry::RegisterDesign(FName(TEXT("F11_AuditShip")),TestDesign);
    const FString Folder=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Automation/F11/"));
    IFileManager::Get().MakeDirectory(*Folder,true);
    const FString File=Folder+TEXT("roundtrip.def");
    ON_SCOPE_EXIT { IFileManager::Get().Delete(*File); };
    Mission Saved(911,"roundtrip.def",TCHAR_TO_ANSI(*Folder));Saved.SetName("Edited mission");
    auto* Pilot=new MissionElement;Pilot->SetShipDesign(ShipDesignRegistry::Find(FName(TEXT("F11_AuditShip"))));Pilot->SetName("Editor pilot");Pilot->SetPlayer(1);Pilot->SetRegion("");Saved.GetElements().append(Pilot);
    auto* Trigger=new MissionEvent;Trigger->id=19;Saved.GetEvents().append(Trigger);
    TestTrue(TEXT("Edited mission saves successfully"),Saved.Save());
    FString Written;TestTrue(TEXT("Mission output exists"),FFileHelper::LoadFileToString(Written,*File));
    Mission Reloaded;TestTrue(TEXT("Reloaded mission definition is valid"),Reloaded.ParseMission(TCHAR_TO_ANSI(*Written)));
    TestEqual(TEXT("Saved edit survives reload"),FString(ANSI_TO_TCHAR(Reloaded.GetName())),FString("Edited mission"));
    TestTrue(TEXT("Package and persistent event ID survive reload"),Reloaded.GetElements().size()==1&&Reloaded.GetEvents().size()==1&&Reloaded.GetEvents()[0]->EventID()==19);
    Dlg->mission=nullptr;Dlg->mission_info=nullptr;
    return true;
}
#endif
