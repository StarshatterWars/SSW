#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ShipStorePresentation.generated.h"
UINTERFACE(BlueprintType)
class STARSHATTERWARS_API UShipStorePresentation : public UInterface
{
    GENERATED_BODY()
};
/** Optional interface on the BP actor assigned to a loadout hardpoint. */
class STARSHATTERWARS_API IShipStorePresentation
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Weapon Mount|Stores")
    void UpdateStorePresentation(int32 RemainingRounds, int32 Capacity, bool bAvailable);
};
