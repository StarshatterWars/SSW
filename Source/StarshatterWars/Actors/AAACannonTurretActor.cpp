#include "AAACannonTurretActor.h"
AAAACannonTurretActor::AAAACannonTurretActor()
{
    WeaponType=TEXT("AAA Cannon");
    LegacyGunModel=TEXT("Ground/AAA2Guns.mag");
    LegacyBaseModel=TEXT("AAATurretBase");
    LegacyModelScale=2.0f;
    BaseAxis=0;
    LegacyMuzzles.Add(FVector(12.0,0.0,112.0));
    LegacyMuzzles.Add(FVector(-12.0,0.0,112.0));
}
