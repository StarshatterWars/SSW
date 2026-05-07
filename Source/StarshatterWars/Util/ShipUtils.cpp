#include "ShipUtils.h"

EExplosionType ShipUtils::ExplosionTypeFromInt(int32 Value)
{
    switch (Value)
    {
    case 1:  return EExplosionType::SHIELD_FLASH;
    case 2:  return EExplosionType::HULL_FLASH;
    case 3:  return EExplosionType::BEAM_FLASH;
    case 4:  return EExplosionType::SHOT_BLAST;
    case 5:  return EExplosionType::HULL_BURST;
    case 6:  return EExplosionType::HULL_FIRE;
    case 7:  return EExplosionType::PLASMA_LEAK;
    case 8:  return EExplosionType::SMOKE_TRAIL;
    case 9:  return EExplosionType::SMALL_FIRE;
    case 10: return EExplosionType::SMALL_EXPLOSION;
    case 11: return EExplosionType::LARGE_EXPLOSION;
    case 12: return EExplosionType::LARGE_BURST;
    case 13: return EExplosionType::NUKE_EXPLOSION;
    case 14: return EExplosionType::QUANTUM_FLASH;
    case 15: return EExplosionType::HYPER_FLASH;
    default: return EExplosionType::NONE;
    }
}

int32 ShipUtils::ExplosionTypeToInt(EExplosionType Type)
{
    return static_cast<int32>(Type);
}