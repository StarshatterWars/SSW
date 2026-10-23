#include "FighterShipActor.h"
#include "Components/SceneComponent.h"

AFighterShipActor::AFighterShipActor()
{
    FighterCockpitRoot = CreateDefaultSubobject<USceneComponent>(TEXT("FighterCockpitRoot"));
    FighterCockpitRoot->SetupAttachment(PointRoot);

    FighterLandingGearRoot = CreateDefaultSubobject<USceneComponent>(TEXT("FighterLandingGearRoot"));
    FighterLandingGearRoot->SetupAttachment(PointRoot);

    FighterLaunchRoot = CreateDefaultSubobject<USceneComponent>(TEXT("FighterLaunchRoot"));
    FighterLaunchRoot->SetupAttachment(PointRoot);
}
