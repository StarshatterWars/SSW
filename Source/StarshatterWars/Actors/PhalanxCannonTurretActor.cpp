#include "PhalanxCannonTurretActor.h"
APhalanxCannonTurretActor::APhalanxCannonTurretActor()
{
    WeaponType=TEXT("Phalanx Cannon");
    LegacyGunModel=TEXT("Alliance/BallTurret2opt.mag");
    LegacyBaseModel=TEXT("");
    LegacyModelScale=0.3f;
    BaseAxis=0;
    LegacyMuzzles.Add(FVector(16.0,0.0,110.0));
    LegacyMuzzles.Add(FVector(-16.0,0.0,110.0));
}
