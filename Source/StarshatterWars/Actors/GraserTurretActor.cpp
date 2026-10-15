#include "GraserTurretActor.h"
AGraserTurretActor::AGraserTurretActor()
{
    WeaponType=TEXT("Graser Turret");
    LegacyGunModel=TEXT("GraserTurret/guns.mag");
    LegacyBaseModel=TEXT("GraserTurret/base.mag");
    LegacyModelScale=2.0f;
    BaseAxis=0;
    LegacyMuzzles.Add(FVector(0.0,0.0,90.0));
}
