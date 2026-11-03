#include "RocketsTurretActor.h"
ARocketsTurretActor::ARocketsTurretActor()
{
    WeaponType=TEXT("Rockets");
    LegacyGunModel=TEXT("RocketLauncherGuns");
    LegacyBaseModel=TEXT("");
    LegacyModelScale=0.15f;
    BaseAxis=0;
}
