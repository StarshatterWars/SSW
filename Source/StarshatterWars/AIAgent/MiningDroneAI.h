#pragma once
#include "MiningDroneTypes.h"

struct FMiningDroneFrame
{
    bool bAuthority=false;
    bool bOwnerAlive=true;
    FVector OwnerPosition=FVector::ZeroVector;
    bool bTargetValid=false;
    bool bTargetAssayed=false;
    int32 TargetContact=INDEX_NONE;
    FVector TargetPosition=FVector::ZeroVector;
    double TargetRadius=0;
    FName TargetOre;
    double RemainingTonnes=0;
};
class STARSHATTERWARS_API MiningDroneAI
{
public:
    static bool Eligible(bool Authority,bool Playing,bool Cinematic,bool Exporting,bool Paused);
    static bool Launch(FMiningDroneFleetState&,FGuid,const FMiningDroneBayDefinition&,const FMiningDroneFrame&,FText&);
    static bool Mine(FMiningDroneState&,const FMiningDroneBayDefinition&,const FMiningDroneFrame&,FText&);
    static bool Stop(FMiningDroneState&,const FMiningDroneFrame&,FText&);
    static bool Return(FMiningDroneState&,const FMiningDroneFrame&,FText&);
    static bool Dock(FMiningDroneState&,const FMiningDroneFrame&,TFunctionRef<double(FName,double)> Unload,FText&);
    static bool Tick(FMiningDroneState&,const FMiningDroneBayDefinition&,const FMiningDroneFrame&,double Seconds,double TonnesPerSecond,
        TFunctionRef<double(double)> Extract,TFunctionRef<double(FName,double)> Unload);
    static void Lose(FMiningDroneState&);
};
