#include "FighterTurretActor.h"
AFighterTurretActor::AFighterTurretActor()
{
    WeaponType=TEXT("Fighter Turret");
    LegacyGunModel=TEXT("turret.mag");
    LegacyBaseModel=TEXT("");
    LegacyModelScale=0.22f;
    BaseAxis=0;
}
