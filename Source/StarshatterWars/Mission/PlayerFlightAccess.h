#pragma once
// Cadets fly assigned training sorties. Unassigned flight opens at Ensign.
namespace PlayerFlightAccess
{
    inline bool CanLaunch(bool bAssignedMission,int32 Rank) { return bAssignedMission || Rank>=1; }
}
