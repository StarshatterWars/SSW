#pragma once
#include "FractalVerseState.h"
class FFractalVerseShipProvisioning
{
public:
    static bool Plan(const FFractalVersePilot& Pilot,int32 SpawnSlot,FFractalVersePlayerShip& Out,FString& Reason);
    static bool Validate(const FFractalVersePlayerShip& Ship);
};
