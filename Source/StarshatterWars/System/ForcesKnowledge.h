#pragma once
#include "CoreMinimal.h"
enum class EForcesScope : uint8 { Sector, System, Neighbors };
struct FForcesReport
{
    uint32 Id=0;
    FGuid HullId;
    FString Status;
    bool bAlliedOperationalAccess=false;
    FString Name,Design,Group,System,Sector;
    double Integrity=0,Speed=0;
    int32 IFF=0;
    int32 ObserverIFF=0;
    bool bCampaignAllowed=true;
    bool bReported=false;
};
class FForcesKnowledge
{
public:
    static EForcesScope ScopeForLevel(int32 Level);
    static TArray<FForcesReport> Filter(const TArray<FForcesReport>& Reports,int32 ObserverIFF,
        FName CurrentSystem,FName CurrentSector,FName SelectedSector,EForcesScope Requested,int32 SensorLevel,const TSet<FName>& Neighbors);
};
