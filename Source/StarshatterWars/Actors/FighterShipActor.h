#pragma once

#include "CoreMinimal.h"
#include "ShipActor.h"
#include "FighterShipActor.generated.h"

/** Common fighter presentation attachments. Flight and gear simulation stays in Ship. */
UCLASS(Blueprintable)
class STARSHATTERWARS_API AFighterShipActor : public AShipActor
{
    GENERATED_BODY()
public:
    AFighterShipActor();    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fighter|Components")
    TObjectPtr<USceneComponent> FighterCockpitRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fighter|Components")
    TObjectPtr<USceneComponent> FighterLandingGearRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fighter|Components")
    TObjectPtr<USceneComponent> FighterLaunchRoot;
};
