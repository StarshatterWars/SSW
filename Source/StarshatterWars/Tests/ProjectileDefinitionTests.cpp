#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "StarshatterShipDesignSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/DataTable.h"
#include "Misc/Paths.h"
#include "ShipWeaponMountComponent.h"
#include "BoltWeaponActor.h"
#include "GameStructs_System.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProjectileDefinitionTest,"Starshatter.Weapons.ProjectileDefinitions",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FProjectileDefinitionTest::RunTest(const FString&)
{
    auto* Table=NewObject<UDataTable>();Table->RowStruct=FShipDesign::StaticStruct();
    auto* Importer=NewObject<UStarshatterShipDesignSubsystem>(NewObject<UGameInstance>());Importer->ShipDesignDataTable=Table;
    const FString File=FPaths::ProjectContentDir()/TEXT("GameData/Ships/Falcon.def");
    const auto Ansi=StringCast<ANSICHAR>(*File);Importer->LoadShipDesign(Ansi.Get());
    const auto* Row=Table->FindRow<FShipDesign>(TEXT("Falcon"),TEXT("Projectile DEF"));
    if(!TestNotNull(TEXT("Real Falcon DEF imports"),Row))return false;
    const FShipWeapon* Cannon=Row->Weapon.FindByPredicate([](const FShipWeapon& W){return W.WeaponType==TEXT("Delta Cannon");});
    if(!TestNotNull(TEXT("DEF contains the Delta Cannon launcher"),Cannon))return false;
    const FString Delta=ABoltWeaponActor::BlueprintPathForWeapon(TEXT("Delta Cannon"));
    const FString Alpha=ABoltWeaponActor::BlueprintPathForWeapon(TEXT("Alpha Cannon"));
    TestEqual(TEXT("DEF projectile field survives ingestion"),Cannon->ProjectileBlueprint,Delta);
    TestEqual(TEXT("Twin muzzles remain authored"),Cannon->Muzzles.Num(),2);
    TestEqual(TEXT("Four missile stations remain"),Row->Hardpoint.Num(),4);
    auto* Mount=NewObject<UShipWeaponMountComponent>();
    Mount->ConfigureProjectileBlueprint(Cannon->ProjectileBlueprint);
    TestEqual(TEXT("DEF supplies the fired Blueprint"),Mount->ResolveProjectileClass(),ABoltWeaponActor::BlueprintClassForWeapon(TEXT("Delta Cannon")).Get());
    Mount->ProjectileBlueprint=TSoftClassPtr<AActor>(FSoftObjectPath(Alpha));
    Mount->ConfigureProjectileBlueprint(Delta);
    TestEqual(TEXT("Manual picker override survives DEF refresh"),Mount->ResolveProjectileClass(),ABoltWeaponActor::BlueprintClassForWeapon(TEXT("Alpha Cannon")).Get());
    Mount->ProjectileBlueprint.Reset();Mount->ConfigureProjectileBlueprint(Delta);
    TestEqual(TEXT("Clearing override restores DEF default"),Mount->ProjectileBlueprint.ToSoftObjectPath().ToString(),Delta);
    Mount->ConfigureProjectileBlueprint(Alpha);
    TestEqual(TEXT("Automatic defaults follow updated DEF"),Mount->ProjectileBlueprint.ToSoftObjectPath().ToString(),Alpha);
    TestFalse(TEXT("Configuring a projectile never creates an attached projectile"),Mount->GetChildActor()!=nullptr);
    return true;
}
#endif
