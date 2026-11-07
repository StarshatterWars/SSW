#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Game.h"
#include "Materials/Material.h"
#include "OrionShipActor.h"
#include "Shield.h"
#include "ShieldComponent.h"
#include "Ship.h"
#include "ShipDesignRegistry.h"
#include "SimShot.h"
#include "WeaponDesign.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShieldPresentationTest, "Starshatter.VisualPass.ShieldPresentation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShieldPresentationTest::RunTest(const FString&)
{
    const bool bWasPaused = Game::Paused();
    Game::SetPausedState(false);
    ON_SCOPE_EXIT { Game::SetPausedState(bWasPaused); };
    const auto Before = ShipDesignRegistry::GetAll();
    ON_SCOPE_EXIT {
        ShipDesignRegistry::Clear();
        for (const auto& Pair : Before) ShipDesignRegistry::RegisterDesign(Pair.Key, Pair.Value);
    };

    const auto Options = UWorld::InitializationValues().AllowAudioPlayback(false)
        .CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    auto* World = UWorld::CreateWorld(EWorldType::EditorPreview, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Options);
    if (!TestNotNull(TEXT("Shield fixture world"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    auto* Class = LoadClass<AOrionShipActor>(nullptr, TEXT("/Game/Models/Orion/BP_Orion.BP_Orion_C"));
    if (!TestNotNull(TEXT("Orion Blueprint"), Class)) return false;
    auto* Orion = World->SpawnActor<AOrionShipActor>(Class);
    if (!TestNotNull(TEXT("Orion shield texture"), Orion->ShieldComponent->ShieldTexture.Get())) return false;

    FShipDesign Row;
    Row.ShipName = TEXT("Shield fixture");
    Row.Scale = 1;
    Row.Integrity = 10000;
    Row.Mass = 10000;
    Row.Shield.AddDefaulted_GetRef().ShieldType = EShieldType::DEFLECTOR;
    const FName Key(TEXT("ShieldPresentationFixture"));
    ShipDesignRegistry::RegisterDesign(Key, Row);
    Ship Runtime("Shield fixture", "", ShipDesignRegistry::FindLegacy(Key), 2, 0, nullptr, false);
    Runtime.BindNativePresentation(Orion);
    ON_SCOPE_EXIT { Runtime.BindNativePresentation(nullptr); };
    Shield* Shield = Runtime.GetShield();
    if (!TestNotNull(TEXT("Simulation shield"), Shield)) return false;
    Shield->SetPowerLevel(100);
    Shield->ExecFrame(10);
    Shield->Distribute(Shield->GetConsumption(), 1);
    TestTrue(TEXT("Simulation shield charged"), Runtime.GetShieldStrength() > 5);

    WeaponDesign Weapon;
    Weapon.penetration = 1;
    Weapon.life = 1;
    SimShot Shot(FVector::ZeroVector, Camera(), &Weapon);
    Runtime.InflictDamage(100, &Shot, 1, FVector(0, 0, 100));
    TestFalse(TEXT("Hull-only hit leaves shield presentation idle"), Orion->ShieldComponent->IsComponentTickEnabled());
    Runtime.InflictDamage(100, &Shot, 3, FVector(0, 0, 100));
    TestTrue(TEXT("Legacy shield deflection starts the effect"), Orion->ShieldComponent->IsComponentTickEnabled());
    TArray<UStaticMeshComponent*> Meshes;
    Orion->GetComponents(Meshes);
    bool bFoundImpact = false;
    for (const UStaticMeshComponent* Mesh : Meshes)
    {
        if (!Mesh->HasAnyFlags(RF_Transient) || Mesh->GetAttachParent() != Orion->GetRootComponent()) continue;
        if (!Mesh->GetMaterial(0) || !Mesh->GetMaterial(0)->GetMaterial()->GetName().Equals(TEXT("M_ShieldImpact"))) continue;
        bFoundImpact = true;
        TestEqual(TEXT("Impact effect has no UE collision"), Mesh->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
    }
    TestTrue(TEXT("Impact material is instantiated on the ship"), bFoundImpact);
    return true;
}

#endif
