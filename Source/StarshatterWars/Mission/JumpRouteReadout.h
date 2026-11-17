#pragma once
#include "CoreMinimal.h"
class Sim;
class Ship;
struct FJumpRouteReadout
{
    TArray<FString> Systems;
    FString Gate, Region, Status;
    FVector EntryPoint = FVector::ZeroVector;
    double Range = -1;
    bool bHasEntryPoint = false;
    bool bArrived = false;
};
class FJumpRouteModel
{
public:
    static TArray<FString> RemainingRoute(const TArray<FString>& Route, const FString& Current);
    static FJumpRouteReadout Build(Sim&, Ship&, const TArray<FString>& Selected);
};
