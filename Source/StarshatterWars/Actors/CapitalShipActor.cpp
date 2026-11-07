#include "CapitalShipActor.h"
#include "Components/SceneComponent.h"
#include "ShieldComponent.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "Components/StaticMeshComponent.h"

ACapitalShipActor::ACapitalShipActor()
{
    CapitalSubsystemRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CapitalSubsystemRoot"));
    CapitalSubsystemRoot->SetupAttachment(PointRoot);

    CapitalDefenseRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CapitalDefenseRoot"));
    CapitalDefenseRoot->SetupAttachment(PointRoot);
    ShieldComponent = CreateDefaultSubobject<UShieldComponent>(TEXT("ShieldComponent"));

    CapitalCommandRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CapitalCommandRoot"));
    CapitalCommandRoot->SetupAttachment(PointRoot);
}

void ACapitalShipActor::ShowShieldImpact(const Ship& ShieldedShip, const FVector& Impact)
{
    const Camera& Frame = ShieldedShip.GetCam();
    const FVector Delta = Impact - ShieldedShip.GetLocation();
    const FVector Local(FVector::DotProduct(Delta, Frame.vpn()), FVector::DotProduct(Delta, Frame.vrt()),
        FVector::DotProduct(Delta, Frame.vup()));
    const ShipDesign* Design = ShieldedShip.Design();
    ShieldComponent->ShowImpact(Local, Local.GetSafeNormal(), FMath::Clamp(ShieldedShip.GetRadius() * 0.35, 40.0, 500.0),
        Design ? Design->shield_visual_model : FString(), HullMesh);
}
