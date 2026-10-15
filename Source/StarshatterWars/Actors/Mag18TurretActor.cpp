#include "Mag18TurretActor.h"
AMag18TurretActor::AMag18TurretActor()
{
    WeaponType=TEXT("Mag-18");
    LegacyGunModel=TEXT("Marakan/18INMAGGUN.MAG");
    LegacyBaseModel=TEXT("");
    LegacyModelScale=4.0f;
    BaseAxis=0;
    LegacyMuzzles.Add(FVector(0.0,12.0,81.0));
}
