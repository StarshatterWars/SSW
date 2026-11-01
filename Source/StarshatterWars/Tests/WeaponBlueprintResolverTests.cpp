#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "WeaponBlueprintResolver.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponBlueprintResolverTest, "Starshatter.Weapons.BlueprintModels",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWeaponBlueprintResolverTest::RunTest(const FString& Parameters)
{
    for (const TCHAR* Name : {TEXT("Archer"), TEXT("Hammer"), TEXT("Javelin"), TEXT("Torpedo"), TEXT("Vector"), TEXT("Harpoon")})
    {
        UClass* Class = WeaponBlueprintResolver::Resolve(Name, TEXT("Unused"));
        TestNotNull(*FString::Printf(TEXT("%s model resolves to Blueprint"), Name), Class);
        TestEqual(TEXT("Weapon name fallback resolves the same asset"), WeaponBlueprintResolver::Resolve(TEXT("MissingLegacyModel"), Name), Class);
    }
    TestNull(TEXT("Unknown model retains presentation fallback"), WeaponBlueprintResolver::Resolve(TEXT("UnknownMissile"), TEXT("UnknownWeapon")));
    return true;
}
#endif
