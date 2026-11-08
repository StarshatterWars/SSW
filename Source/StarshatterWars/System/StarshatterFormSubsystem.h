#pragma once

#include "Subsystems/GameInstanceSubsystem.h"
#include "StarshatterFormSubsystem.generated.h"

// Retains the reflected type for existing assets. Native screens own their UI.
UCLASS(Abstract)
class STARSHATTERWARS_API UStarshatterFormSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
};
