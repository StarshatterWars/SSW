#include "LCATurretActor.h"
ALCATurretActor::ALCATurretActor()
{
    WeaponType=TEXT("LCA Turret");
    LegacyGunModel=TEXT("Alliance/LCATurretGuns.mag");
    LegacyBaseModel=TEXT("Alliance/LCaTurretBase.mag");
    LegacyModelScale=0.12f;
    BaseAxis=0;
}
