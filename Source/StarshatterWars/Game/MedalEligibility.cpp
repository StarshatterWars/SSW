#include "MedalEligibility.h"
bool MedalEligibility::MeetsRequirements(const FMedalInfo& M,const FMedalEligibilityContext& C)
{
    if(!M.bEligibilityConfigured || M.MedalId<=0 || (M.MedalId & (M.MedalId-1))!=0 || (C.Medals&M.MedalId)!=0)return false;
    if(C.Rank<M.MinRank || C.Rank>M.MaxRank || (C.Medals&M.RequiredAwards)!=M.RequiredAwards)return false;
    if(C.ShipClass<M.MinShipClass || C.ShipClass>M.MaxShipClass)return false;
    if(C.TotalPoints<M.TotalPoints || C.TotalMissions<M.TotalMissions)return false;
    if(M.CampaignId && M.bCampaignCompleteRequired)
    { if(!C.bCompletedCampaign)return false; }
    else if(M.bDynamicCampaignOnly && C.bHasCampaign && !C.bDynamicCampaign)return false;
    if(C.MissionPoints<M.MissionPoints || C.Kills<M.Kills)return false;
    if(M.Lost<0 && (C.Deaths || C.Collisions))return false;
    return C.Deaths>=M.Lost && C.Collisions>=M.Collision;
}
