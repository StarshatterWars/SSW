#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "ShipWeaponMountComponent.h"
#include "BoltWeaponActor.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProjectileBlueprintTest,"Starshatter.Weapons.ProjectileBlueprints",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FProjectileBlueprintTest::RunTest(const FString&)
{
    const FString Delta=ABoltWeaponActor::BlueprintPathForWeapon(TEXT("Delta Cannon"));
    const FString Alpha=ABoltWeaponActor::BlueprintPathForWeapon(TEXT("Alpha Cannon"));
    auto* Mount=NewObject<UShipWeaponMountComponent>();
    Mount->ConfigureProjectileBlueprint(Delta);
    TestEqual(TEXT("Weapon type supplies the fired Blueprint"),Mount->ResolveProjectileClass(),ABoltWeaponActor::BlueprintClassForWeapon(TEXT("Delta Cannon")).Get());
    Mount->ProjectileBlueprint=TSoftClassPtr<AActor>(FSoftObjectPath(Alpha));
    Mount->ConfigureProjectileBlueprint(Delta);
    TestEqual(TEXT("Manual picker override survives code refresh"),Mount->ResolveProjectileClass(),ABoltWeaponActor::BlueprintClassForWeapon(TEXT("Alpha Cannon")).Get());
    Mount->ProjectileBlueprint.Reset();Mount->ConfigureProjectileBlueprint(Delta);
    TestEqual(TEXT("Clearing override restores code default"),Mount->ProjectileBlueprint.ToSoftObjectPath().ToString(),Delta);
    Mount->ConfigureProjectileBlueprint(Alpha);
    TestEqual(TEXT("Automatic default follows updated weapon"),Mount->ProjectileBlueprint.ToSoftObjectPath().ToString(),Alpha);
    TestNull(TEXT("Selecting a projectile creates no attached bolt"),Mount->GetChildActor());
    return true;
}
#endif
