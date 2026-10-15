#include "HeliconTurretActor.h"
AHeliconTurretActor::AHeliconTurretActor()
{
    WeaponType=TEXT("Helicon");
    LegacyGunModel=TEXT("Marakan/28MMGAUSSGUN.MAG");
    LegacyBaseModel=TEXT("");
    LegacyModelScale=1.0f;
    BaseAxis=0;
    LegacyMuzzles.Add(FVector(0.0,10.0,131.0));
}
