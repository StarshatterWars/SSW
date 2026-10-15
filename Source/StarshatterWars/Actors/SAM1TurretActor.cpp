#include "SAM1TurretActor.h"
ASAM1TurretActor::ASAM1TurretActor()
{
    WeaponType=TEXT("SAM-1");
    LegacyGunModel=TEXT("Ground/SAMRail.mag");
    LegacyBaseModel=TEXT("Ground/SAMTurret.mag");
    LegacyModelScale=2.0f;
    BaseAxis=0;
}
