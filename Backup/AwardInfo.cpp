/*=============================================================================
    Project:        Starshatter Wars (nGenEx Unreal Port)
    Studio:         Fractal Dev Games
    Copyright:      (C) 2024–2026. All Rights Reserved.

    SUBSYSTEM:      Awards
    FILE:           AwardInfo.cpp
    AUTHOR:         Carlos Bott

    IMPLEMENTATION
    ==============
    Thin facade over UAwardInfoRegistry.
=============================================================================*/

#include "AwardInfo.h"
#include "AwardInfoRegistry.h"

// ------------------------------------------------------------
// Ranks
// ------------------------------------------------------------

FString FAwardInfo::RankName(int32 RankId)
{
    return FString(UAwardInfoRegistry::RankName(RankId));
}

FString FAwardInfo::RankAbrv(int32 RankId)
{
    return FString(UAwardInfoRegistry::RankAbrv(RankId));
}

FString FAwardInfo::RankDescription(int32 RankId)
{
    return FString(UAwardInfoRegistry::RankDescription(RankId));
}

FString FAwardInfo::RankAwardText(int32 RankId)
{
    return FString(UAwardInfoRegistry::RankAwardText(RankId));
}

// ------------------------------------------------------------
// Medals
// ------------------------------------------------------------

FString FAwardInfo::MedalName(int32 MedalId)
{
    return FString(UAwardInfoRegistry::MedalName(MedalId));
}

FString FAwardInfo::MedalDescription(int32 MedalId)
{
    return FString(UAwardInfoRegistry::MedalDescription(MedalId));
}

FString FAwardInfo::MedalAwardText(int32 MedalId)
{
    return FString(UAwardInfoRegistry::MedalAwardText(MedalId));
}
