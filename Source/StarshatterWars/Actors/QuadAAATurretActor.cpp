#include "QuadAAATurretActor.h"
AQuadAAATurretActor::AQuadAAATurretActor()
{
    WeaponType=TEXT("Quad AAA");
    LegacyGunModel=TEXT("Ground/AAA4Guns.mag");
    LegacyBaseModel=TEXT("Ground/AAA4Turret.mag");
    LegacyModelScale=2.0f;
    BaseAxis=0;
    LegacyMuzzles.Add(FVector(29.0,9.0,68.0));
    LegacyMuzzles.Add(FVector(-29.0,9.0,68.0));
    LegacyMuzzles.Add(FVector(29.0,-8.0,68.0));
    LegacyMuzzles.Add(FVector(-29.0,-8.0,68.0));
}
