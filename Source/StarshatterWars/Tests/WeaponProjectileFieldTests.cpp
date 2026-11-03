#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "StarshatterWeaponDesignSubsystem.h"
#include "WeaponDesignRegistry.h"
#include "WeaponDesign.h"
#include "WeaponBlueprintResolver.h"
#include "Engine/GameInstance.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponProjectileFieldTest,"Starshatter.Weapons.ProjectileField",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWeaponProjectileFieldTest::RunTest(const FString&)
{
    const auto Before=WeaponDesignRegistry::GetAll();
    ON_SCOPE_EXIT{WeaponDesignRegistry::Clear();for(const auto& P:Before)WeaponDesignRegistry::RegisterDesign(P.Key,P.Value);};
    auto* Table=NewObject<UDataTable>();Table->RowStruct=FWeaponDesign::StaticStruct();
    auto* Importer=NewObject<UStarshatterWeaponDesignSubsystem>(NewObject<UGameInstance>());Importer->WeaponDesignDataTable=Table;
    const FString File=FPaths::ProjectContentDir()/TEXT("GameData/Weapons/Wep.def");
    const auto Ansi=StringCast<ANSICHAR>(*File);Importer->LoadWeaponDesign(Ansi.Get());
    auto* Saved=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_WeaponDesign.DT_WeaponDesign"));
    if(!TestNotNull(TEXT("Persisted weapon table"),Saved))return false;
    int32 Count=0;
    for(const auto& P:Table->GetRowMap())
    {
        const auto* Row=reinterpret_cast<const FWeaponDesign*>(P.Value);
        if(!Row->bPrimary || Row->bBeam || Row->GraphicType!=3)continue;
        ++Count;
        TestFalse(TEXT("Wep.def bolt has a projectile"),Row->Projectile.IsEmpty());
        const FWeaponDesign* Persisted=nullptr;
        for(const auto& Entry:Saved->GetRowMap()) {
            const auto* Candidate=reinterpret_cast<const FWeaponDesign*>(Entry.Value);
            if(Candidate->Name==Row->Name){Persisted=Candidate;break;}
        }
        if(TestNotNull(TEXT("Saved bolt row exists"),Persisted))
            TestEqual(TEXT("Projectile persists in imported DataTable"),Persisted->Projectile,Row->Projectile);
        const auto* Legacy=WeaponDesignRegistry::FindLegacy(Row->Name);
        if(TestNotNull(TEXT("Legacy weapon exists"),Legacy))
            TestEqual(TEXT("Registry carries projectile without loss"),FString(ANSI_TO_TCHAR(Legacy->projectile.data())),Row->Projectile);
        TestNotNull(TEXT("Authored projectile Blueprint loads"),WeaponBlueprintResolver::ResolveProjectile(Row->Projectile));
    }
    TestEqual(TEXT("All 26 bolt weapons covered"),Count,26);
    FWeaponDesign Fixture;Fixture.Name=TEXT("Projectile registry fixture");Fixture.Model=TEXT("Archer");Fixture.Projectile=TEXT("DeltaBolt");
    WeaponDesignRegistry::RegisterDesign(FName(TEXT("ProjectileFixture")),Fixture);
    const auto* Legacy=WeaponDesignRegistry::FindLegacy(FName(TEXT("ProjectileFixture")));
    if(TestNotNull(TEXT("Fixture converts"),Legacy)){
        TestEqual(TEXT("Model is carried independently"),FString(ANSI_TO_TCHAR(Legacy->model.data())),Fixture.Model);
        TestEqual(TEXT("Projectile is carried independently"),FString(ANSI_TO_TCHAR(Legacy->projectile.data())),Fixture.Projectile);
    }
    TestEqual(TEXT("Short reference resolves canonical bolt path"),WeaponBlueprintResolver::ProjectileClassPath(TEXT("DeltaBolt")),FString(TEXT("/Game/Models/Weapons/Bolt/Delta/BP_DeltaBolt.BP_DeltaBolt_C")));
    return true;
}
#endif
