#include "VanguardTurretActor.h"
AVanguardTurretActor::AVanguardTurretActor()
{
    WeaponType=TEXT("Vanguard");
    LegacyGunModel=TEXT("Alliance/Guns1opt.mag");
    LegacyBaseModel=TEXT("VanguardTurretBase");
    LegacyModelScale=1.2f;
    BaseAxis=0;
    LegacyMuzzles.Add(FVector(7.0,0.0,60.0));
    LegacyMuzzles.Add(FVector(-7.0,0.0,60.0));
}
