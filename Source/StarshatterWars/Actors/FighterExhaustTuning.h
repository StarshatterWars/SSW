#pragma once
#include "CoreMinimal.h"
namespace FighterExhaustTuning
{
    inline constexpr const TCHAR* StockSystem=TEXT("/Game/RocketThrusterExhaustFX/FX/NS_RocketExhaust_Blue.NS_RocketExhaust_Blue");
    inline constexpr const TCHAR* FighterSystem=TEXT("/Game/Effects/Engines/NS_SSW_FighterExhaust_Blue.NS_SSW_FighterExhaust_Blue");
    inline constexpr float SpawnScale=.65f;
    inline float VisualPower(float Power)
    {
        Power=FMath::Clamp(Power,0.f,1.5f);
        return Power<=1.f?Power:1.f+(Power-1.f)*.35f;
    }
}
