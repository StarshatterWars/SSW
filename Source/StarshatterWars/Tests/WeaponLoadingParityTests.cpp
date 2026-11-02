#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/DataTable.h"
#include "ShipDesignRegistry.h"
#include "WeaponDesignRegistry.h"
#include "ShipDesign.h"
#include "Ship.h"
#include "Weapon.h"
#include "WeaponGroup.h"
#include "WeaponDesign.h"
#include "ShipLoad.h"
#include "HardPoint.h"
#include "Power.h"
#include "FighterHUDDetails.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponLoadingParityTest,"Starshatter.Weapons.LoadingParity",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWeaponLoadingParityTest::RunTest(const FString&)
{
    const auto ShipsBefore=ShipDesignRegistry::GetAll();
    const auto WeaponsBefore=WeaponDesignRegistry::GetAll();
    ON_SCOPE_EXIT {
        ShipDesignRegistry::Clear(); WeaponDesignRegistry::Clear();
        for (const auto& P:WeaponsBefore) WeaponDesignRegistry::RegisterDesign(P.Key,P.Value);
        for (const auto& P:ShipsBefore) ShipDesignRegistry::RegisterDesign(P.Key,P.Value);
    };
    auto* Weapons=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_WeaponDesign.DT_WeaponDesign"));
    auto* Ships=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_ShipDesign.DT_ShipDesign"));
    if (!TestNotNull(TEXT("Weapon definitions"),Weapons) || !TestNotNull(TEXT("Ship definitions"),Ships)) return false;
    for (const auto& P:Weapons->GetRowMap()) WeaponDesignRegistry::RegisterDesign(P.Key,*reinterpret_cast<const FWeaponDesign*>(P.Value));
    const FShipDesign* Falcon=Ships->FindRow<FShipDesign>(TEXT("Falcon"),TEXT("Weapon parity"));
    if (!TestNotNull(TEXT("Falcon data"),Falcon)) return false;
    ShipDesignRegistry::RegisterDesign(FName(TEXT("Falcon")),*Falcon);
    auto* Design=const_cast<ShipDesign*>(ShipDesignRegistry::FindLegacy(FName(TEXT("Falcon"))));
    if (!TestNotNull(TEXT("Runtime Falcon"),Design)) return false;
    ShipLoad* Load=nullptr; ListIter<ShipLoad> Loads=Design->loadouts;
    while (++Loads) if (!FCStringAnsi::Stricmp(Loads->GetName(),"ACM Medium Range")) Load=Loads.value();
    if (!TestNotNull(TEXT("T1 named default loadout resolves"),Load)) return false;
    TestEqual(TEXT("Four external hardpoints"),Design->hardpoints.size(),4);
    const int Expected[]={0,1,1,0};
    for (int I=0;I<4;++I) TestEqual(TEXT("Named station choice retains legacy order"),Load->GetStation(I),Expected[I]);
    {
        Ship Fighter("Weapon parity Falcon","",Design,1,0,Load->GetLoad(),false);
        Weapon* Gun=Fighter.GetPrimary();
        if (!TestNotNull(TEXT("Fixed primary gun created"),Gun)) return false;
        TestTrue(TEXT("Delta Cannon is primary"),Gun->IsPrimary());
        TestFalse(TEXT("Delta Cannon is not a missile"),Gun->IsMissile());
        TestEqual(TEXT("Primary weapon definition"),FString(ANSI_TO_TCHAR(Gun->Design()->name)),FString(TEXT("Delta Cannon")));
        TestEqual(TEXT("Legacy one-based primary index"),Gun->GetIndex(),1);
        TestEqual(TEXT("DEF abbreviation preserved by registry and runtime group"),FString(ANSI_TO_TCHAR(Fighter.GetPrimaryGroup()->Abbreviation())),FString(TEXT("Cannon")));
        TestEqual(TEXT("HUD names Delta instead of numeric gun ammo"),SFighterHUDDetails::FormatWeaponReadout(Fighter.GetPrimaryGroup()),FString(TEXT("GUNS: DELTA")));
        TestTrue(TEXT("Secondary group is ordnance"),Fighter.GetSecondaryGroup() && Fighter.GetSecondaryGroup()->IsMissile());
        int Mounted=0;
        for (int G=0;G<Fighter.GetWeapons().size();++G)
        {
            WeaponGroup* Group=Fighter.GetWeapons()[G];
            for (int W=0;W<Group->NumWeapons();++W)
            {
                Weapon* MountedWeapon=Group->GetWeapon(W);
                if (MountedWeapon->IsPrimary()) continue;
                const int Station=MountedWeapon->GetIndex()-2;
                if (!TestTrue(TEXT("Missile index identifies legacy hardpoint"),Station>=0 && Station<4)) continue;
                Weapon* ExpectedWeapon=Design->hardpoints[Station]->CreateWeapon(Expected[Station]);
                if (TestNotNull(TEXT("Selected station definition exists"),ExpectedWeapon))
                {
                    TestTrue(TEXT("Mounted missile matches station-specific legacy type index"),MountedWeapon->Design()==ExpectedWeapon->Design());
                    TestEqual(TEXT("Mounted missile ammo matches definition"),MountedWeapon->Ammo(),ExpectedWeapon->Ammo());
                    delete ExpectedWeapon;
                }
                ++Mounted;
            }
        }
        TestEqual(TEXT("All four selected hardpoints loaded"),Mounted,4);
    }
    {
        int EmptyStations[4]={-1,-1,-1,-1};
        Ship Clean("Clean parity Falcon","",Design,1,0,EmptyStations,false);
        TestTrue(TEXT("Clean loadout keeps Delta primary"),Clean.GetPrimary() && Clean.GetPrimary()->IsPrimary());
        TestEqual(TEXT("Explicit empty stations are not refilled from defaults"),Clean.GetWeapons().size(),1);
        TestNull(TEXT("Clean loadout has no secondary missile group"),Clean.GetSecondaryGroup());
    }
    // Two-reactor fixture proves fixed guns use their source index, not reactor zero.
    {
        WeaponDesign Delta; Delta.primary=true; Delta.name="Delta Cannon"; Delta.group="GUNS";
        ShipDesign Fixture;
        Fixture.reactors.append(new PowerSource(static_cast<EPowerSource>(0),65,1));
        Fixture.reactors.append(new PowerSource(static_cast<EPowerSource>(0),65,1));
        FVector Muzzle=FVector::ZeroVector;
        auto* Gun=new Weapon(&Delta,1,&Muzzle); Gun->SetSourceIndex(1); Fixture.weapons.append(Gun);
        Ship Fighter("Reactor parity","",&Fixture,1,0,nullptr,false);
        auto* RuntimeGun=Fighter.GetPrimary();
        TestTrue(TEXT("Gun uses configured reactor"),Fighter.GetReactors()[1]->Clients().contains(RuntimeGun));
        TestFalse(TEXT("Gun is not also wired to first reactor"),Fighter.GetReactors()[0]->Clients().contains(RuntimeGun));
    }
    return true;
}
#endif
