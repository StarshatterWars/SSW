#include "LCATurretActor.h"
ALCATurretActor::ALCATurretActor()
{
    WeaponType=TEXT("LCA Turret");
    LegacyGunModel=TEXT("Alliance/LCATurretGuns.mag");
    LegacyBaseModel=TEXT("LCATurretBase");
    LegacyModelScale=0.12f;
    BaseAxis=0;
}
