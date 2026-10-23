#include "CapitalShipActor.h"
#include "Components/SceneComponent.h"

ACapitalShipActor::ACapitalShipActor()
{
    CapitalSubsystemRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CapitalSubsystemRoot"));
    CapitalSubsystemRoot->SetupAttachment(PointRoot);

    CapitalDefenseRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CapitalDefenseRoot"));
    CapitalDefenseRoot->SetupAttachment(PointRoot);

    CapitalCommandRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CapitalCommandRoot"));
    CapitalCommandRoot->SetupAttachment(PointRoot);
}
