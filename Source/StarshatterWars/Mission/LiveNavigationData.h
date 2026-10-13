#pragma once
#include "CoreMinimal.h"
#include "MissionNavObjectListObject.h"

// Value snapshots only: map and list never own or retain simulation objects.
struct FLiveNavigationContact
{
    uint32 Id = 0;
    FString Name, ClassName, DesignName, System, Sector;
    FVector Location = FVector::ZeroVector;
    FLinearColor Color = FLinearColor::White;
    EMissionNavObjectType Type = EMissionNavObjectType::None;
    double Distance = 0;
    bool bLocal = false;
    bool bPlayer = false;
};
struct FLiveNavigationWaypoint
{
    FVector Location = FVector::ZeroVector;
    FString Sector, Label;
    bool bNext = false;
};
