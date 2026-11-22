#pragma once
// Every departure requires an assigned sortie, including independent Free Flight.
namespace PlayerFlightAccess
{
    inline bool CanLaunch(bool bAssignedMission,int32 /*Rank*/) { return bAssignedMission; }
}
