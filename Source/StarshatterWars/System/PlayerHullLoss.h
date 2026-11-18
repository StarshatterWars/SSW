#pragma once
#include "GameStructs.h"

// Called inside the insurance/save transaction after the real loss is validated.
// Preserves pilot progression, other hulls and station inventories.
class FPlayerHullLoss
{
public:
    static bool Finalize(FS_PlayerGameInfo& Pilot,FGuid Hull);
};
