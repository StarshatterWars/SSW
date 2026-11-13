#pragma once

#include "FreightManifestTypes.h"

class TermDef;
class SimRegion;
class Instruction;

namespace SSWFreightManifest
{
    struct FEndpoint
    {
        FString System;
        FString Sector;
        FString Site;
        FVector RegionPosition = FVector::ZeroVector;
    };

    struct FLoad
    {
        FString Item;
        FString Unit;
        int32 Quantity = 0;
        bool bPassengers = false;
        ESSWFinishedItem FinishedItem = ESSWFinishedItem::None;
        TOptional<FEndpoint> Origin;
        TOptional<FEndpoint> Destination;

        void CaptureOrigin(SimRegion& Region, const FVector& Position);
        void CaptureDestination(Instruction& LastWaypoint);
    };

    struct FProfile
    {
        FSSWCargoCapacity Capacity;
        TArray<FSSWCargoOption> Options;
    };

    void ReadCapacity(FSSWCargoCapacity& Capacity, TermDef& Definition, const char* Filename);
    void AddOption(TArray<FSSWCargoOption>& Options, TermDef& Definition, const char* Filename);
    FProfile LoadProfile(const FString& DefinitionName);
    TOptional<FLoad> Assign(const TArray<FSSWCargoOption>& Options, const FSSWCargoCapacity& Capacity,
        const char* ShipName, const char* Registry);
    bool CanResolve(double DistanceMetres, double SensorRangeMetres);
}
