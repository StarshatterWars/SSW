#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "DataLoader.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Ship.h"
#include "ShipActor.h"
#include "ShipDesignRegistry.h"
#include "ShipVisualCatalog.h"
#include "SimRegion.h"
#include "SimShot.h"
#include "StarshatterWeaponDesignSubsystem.h"
#include "StarSystem.h"
#include "Weapon.h"
#include "WeaponGroup.h"
#include "WeaponBlueprintResolver.h"
#include "WeaponDesign.h"
#include "WeaponDesignRegistry.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJanekGroundDefenseTest, "Starshatter.Weapons.JanekGroundDefense",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FJanekGroundDefenseTest::RunTest(const FString&)
{
    const auto SavedWeapons = WeaponDesignRegistry::GetAll();
    const auto SavedShips = ShipDesignRegistry::GetAll();
    ON_SCOPE_EXIT {
        WeaponDesignRegistry::Clear();
        for (const auto& Pair : SavedWeapons) WeaponDesignRegistry::RegisterDesign(Pair.Key, Pair.Value);
        ShipDesignRegistry::Clear();
        for (const auto& Pair : SavedShips) ShipDesignRegistry::RegisterDesign(Pair.Key, Pair.Value);
    };

    const FString Data = FPaths::ProjectContentDir() / TEXT("GameData");
    auto* Instance = NewObject<UGameInstance>();
    auto* Weapons = NewObject<UStarshatterWeaponDesignSubsystem>(Instance);
    const auto WeaponPath = StringCast<ANSICHAR>(*(Data / TEXT("Weapons/Wep.def")));
    Weapons->LoadWeaponDesign(WeaponPath.Get());
    auto* Table = LoadObject<UDataTable>(nullptr, TEXT("/Game/Game/DT_ShipDesign.DT_ShipDesign"));
    if (!TestNotNull(TEXT("Ship data table"), Table)) return false;
    const auto* Falcon = Table->FindRow<FShipDesign>(TEXT("Falcon"), TEXT("Janek target"), false);
    if (!TestNotNull(TEXT("Falcon target design"), Falcon)) return false;
    ShipDesignRegistry::RegisterDesign(FName(TEXT("JanekDefenseTarget")), *Falcon);
    Ship Target("Janek target", "", ShipDesignRegistry::FindLegacy(FName(TEXT("JanekDefenseTarget"))), 1, 0, nullptr, false);
    const bool bOwnLoader = !DataLoader::GetLoader();
    if (bOwnLoader) DataLoader::Initialize();
    DataLoader* Loader = DataLoader::GetLoader();
    const Text PreviousPath = Loader->GetDataPath();
    const bool bPreviousFileSystem = Loader->IsFileSystemEnabled();
    Loader->UseFileSystem(true);
    ON_SCOPE_EXIT {
        if (bOwnLoader) DataLoader::Close();
        else { Loader->SetDataPath(PreviousPath.data()); Loader->UseFileSystem(bPreviousFileSystem); }
    };
    StarSystem Janus("Janus System", FVector::ZeroVector);
    const FString Galaxy = FPaths::ConvertRelativePathToFull(Data / TEXT("Galaxy")) + TEXT("/");
    Janus.SetDataPath(TCHAR_TO_ANSI(*Galaxy));
    Janus.Load();
    OrbitalRegion* Janek = Janus.FindRegion("Janek Station");
    if (!TestNotNull(TEXT("Janek Station region from the Janus definition"), Janek)) return false;

    const auto Options = UWorld::InitializationValues().AllowAudioPlayback(false)
        .CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    auto* World = UWorld::CreateWorld(EWorldType::EditorPreview, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Options);
    if (!TestNotNull(TEXT("Ground defense fixture world"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };

    struct FDefense
    {
        const TCHAR* Key;
        const TCHAR* Name;
        const TCHAR* Hud;
        const TCHAR* Map;
        int32 Barrels;
    };
    const FDefense Defenses[] = {
        {TEXT("Gnd_aaa1"), TEXT("AAA 1"), TEXT("hud_aaa2"), TEXT("map_aaa1"), 2},
        {TEXT("Gnd_aaa2"), TEXT("AAA 2"), TEXT("hud_aaa4"), TEXT("map_aaa2"), 4},
        {TEXT("Gnd_sam1"), TEXT("SAM 1"), TEXT("hud_sam"), TEXT("map_sam1"), 8},
    };
    for (const FDefense& Defense : Defenses)
    {
        const FString Key(Defense.Key);
        const auto* Row = Table->FindRow<FShipDesign>(FName(Defense.Name), TEXT("Janek ground defense"), false);
        if (!TestNotNull(Key + TEXT(" design"), Row)) continue;
        ShipDesignRegistry::RegisterDesign(FName(Key), *Row);
        auto* Design = ShipDesignRegistry::FindLegacy(FName(Key));
        if (!TestNotNull(Key + TEXT(" runtime design"), Design)) continue;

        SimRegion Region(nullptr, Janek);
        Ship Unit(TCHAR_TO_ANSI(Defense.Name), "", Design, 2, 0, nullptr, false);
        Unit.SetRegion(&Region);
        Target.MoveTo(Unit.GetLocation() + FVector(1500, 1200, 15000));

        auto* Actor = World->SpawnActor<AShipActor>(ShipVisualCatalog::Resolve(Defense.Name));
        if (!TestNotNull(Key + TEXT(" native visual"), Actor)) continue;
        Unit.BindNativePresentation(Actor);
        Actor->BindRuntimeShip(&Unit);
        ON_SCOPE_EXIT { Unit.BindNativePresentation(nullptr); Actor->BindRuntimeShip(nullptr); Actor->Destroy(); };

        Weapon* Gun = Unit.GetWeapons().size() ? Unit.GetWeapons()[0]->GetWeapon(0) : nullptr;
        if (TestNotNull(Key + TEXT(" weapon"), Gun))
        {
            TestEqual(Key + TEXT(" barrel count"), Gun->GetTurretBarrelCount(), Defense.Barrels);
            Gun->Distribute(100, 1);
            Gun->Orient(&Unit);
            Gun->SetTarget(&Target, nullptr);
            TestTrue(Key + TEXT(" tracks the airborne target"), Gun->Track(&Target, nullptr) != 0);
            TArray<FVector> Muzzles;
            for (int32 Index = 0; Index < Defense.Barrels; ++Index)
            {
                FVector Position;
                Camera Frame;
                if (TestTrue(Key + TEXT(" native muzzle resolves"), Unit.GetNativeWeaponMuzzle(Gun, Index, Position, Frame)))
                {
                    TestFalse(Key + TEXT(" muzzle is distinct"), Muzzles.ContainsByPredicate([&](const FVector& Other) {
                        return Other.Equals(Position, 0.01);
                    }));
                    Muzzles.Add(Position);
                }
            }
            Gun->SetFiringOrders(WeaponsOrders::AUTO);
            const int32 InitialAmmo = Gun->Ammo();
            for (int32 Index = 0; Index < Defense.Barrels; ++Index)
            {
                Gun->Distribute(100, 1);
                Gun->ExecFrame(Gun->Design()->refire_delay + Gun->Design()->salvo_delay + 0.1);
            }
            TestEqual(Key + TEXT(" automatic fire uses every barrel"), Region.GetShots().size(), Defense.Barrels);
            TSet<int32> FiredMuzzles;
            for (int32 Index = 0; Index < Region.GetShots().size(); ++Index)
            {
                const SimShot* Shot = Region.GetShots()[Index];
                const FVector Origin = Shot->GetLocation() - Shot->GetCam().vpn() * Gun->Design()->length;
                const int32 MuzzleIndex = Muzzles.IndexOfByPredicate([&](const FVector& Muzzle) {
                    return Muzzle.Equals(Origin, 0.05);
                });
                TestTrue(Key + TEXT(" shot starts from a native muzzle"), MuzzleIndex != INDEX_NONE);
                if (MuzzleIndex != INDEX_NONE) FiredMuzzles.Add(MuzzleIndex);
            }
            TestEqual(Key + TEXT(" distinct fired muzzles"), FiredMuzzles.Num(), Defense.Barrels);
            if (InitialAmmo > 0)
            {
                TestEqual(Key + TEXT(" stores consumed"), Gun->Ammo(), InitialAmmo - Defense.Barrels);
            }
        }

        const FString Folder = ShipVisualCatalog::IconFolder(Defense.Name);
        for (const TCHAR* Icon : {Defense.Hud, Defense.Map})
        {
            const FString Path = FString::Printf(TEXT("/Game/UI/Ships/%s/%s.%s"), *Folder, Icon, Icon);
            TestNotNull(Path, LoadObject<UTexture2D>(nullptr, *Path));
        }
    }

    TestNotNull(TEXT("SAM projectile visual resolves from the legacy model"),
        WeaponBlueprintResolver::Resolve(TEXT("sam"), TEXT("SAM-1")));
    return true;
}
#endif
