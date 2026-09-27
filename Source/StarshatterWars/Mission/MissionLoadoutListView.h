#pragma once

#include "CoreMinimal.h"
#include "Components/ListView.h"
#include "MissionLoadoutListView.generated.h"

UCLASS()
class STARSHATTERWARS_API UMissionLoadoutListView : public UListView
{
    GENERATED_BODY()

public:
    void SetEntryWidgetClassPublic(TSubclassOf<UUserWidget> InEntryWidgetClass);
};