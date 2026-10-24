#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/DataTable.h"
#include "TacRefDlg.h"
#include "WeaponDesign.h"
#include "WeaponDesignRegistry.h"
#include "Components/ComboBoxString.h"
#include "Components/RichTextBlock.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponReferenceTest,"Starshatter.Audit.F12.WeaponReference",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWeaponReferenceTest::RunTest(const FString& Parameters)
{
    const auto Original=WeaponDesignRegistry::GetAll();
    ON_SCOPE_EXIT { WeaponDesignRegistry::Clear();for(const auto& Pair:Original) WeaponDesignRegistry::RegisterDesign(Pair.Key,Pair.Value); };
    auto* Table=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_WeaponDesign.DT_WeaponDesign"));
    if(!TestNotNull(TEXT("Project weapon DataTable exists"),Table)) return false;
    for(FName Name:Table->GetRowNames()) if(const auto* Row=Table->FindRow<FWeaponDesign>(Name,TEXT("F12"))) WeaponDesignRegistry::RegisterDesign(Name,*Row);
    auto* Dlg=NewObject<UTacRefDlg>();Dlg->ShipCombo=NewObject<UComboBoxString>(Dlg);Dlg->TxtStats=NewObject<URichTextBlock>(Dlg);
    Dlg->HandleWeaponModeClicked();
    int32 Expected=0;TSet<FName> Seen;
    for (const auto& Pair:WeaponDesignRegistry::GetAll())
        if (auto* D=WeaponDesign::Find(TCHAR_TO_ANSI(*Pair.Key.ToString()))) if (!D->secret) Seen.Add(Pair.Key);
    for (int32 I=0;I<WeaponDesign::catalog.size();++I)
        if(auto* D=WeaponDesign::catalog[I]) if(!D->secret) Seen.Add(FName(ANSI_TO_TCHAR(D->name.data())));
    Expected=Seen.Num();
    TestTrue(TEXT("Project weapon registry is populated"),Expected>0);
    TestEqual(TEXT("Every public weapon has a dropdown row"),Dlg->ShipCombo->GetOptionCount(),Expected);
    if(Expected>0) TestTrue(TEXT("Selecting first weapon populates stats"),!Dlg->TxtStats->GetText().IsEmpty());
    WeaponDesign Design;Design.name="Audit beam";Design.primary=true;Design.beam=true;Design.damage=20;Design.charge=3;Design.speed=500;Design.max_range=8000;
    FString Caption,Stats,Desc;Dlg->BuildWeaponTexts(Design,Caption,Stats,Desc);
    TestTrue(TEXT("Legacy beam damage is damage times charge per second"),Stats.Contains(TEXT("DAMAGE: 60/s")));
    TestTrue(TEXT("Range and speed use authored values"),Stats.Contains(TEXT("RANGE: 8000 m"))&&Stats.Contains(TEXT("SPEED: 500 m/s")));
    return true;
}
#endif
