#pragma once
#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "MissionRouteMap.generated.h"
class Mission;
UCLASS()
class STARSHATTERWARS_API UMissionRouteMap : public UWidget
{
    GENERATED_BODY()
public:
    Mission* Model=nullptr;
    TFunction<FString()> RegionName;
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
};
