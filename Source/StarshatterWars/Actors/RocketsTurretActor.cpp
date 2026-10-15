#include "RocketsTurretActor.h"
ARocketsTurretActor::ARocketsTurretActor()
{
    WeaponType=TEXT("Rockets");
    LegacyGunModel=TEXT("rocket_launcher.mag");
    LegacyBaseModel=TEXT("");
    LegacyModelScale=0.15f;
    BaseAxis=0;
}
