#include "Rail5TurretActor.h"
ARail5TurretActor::ARail5TurretActor()
{
    WeaponType=TEXT("Rail-5");
    LegacyGunModel=TEXT("Marakan/Rail-12.mag");
    LegacyBaseModel=TEXT("");
    LegacyModelScale=2.0f;
    BaseAxis=0;
    LegacyMuzzles.Add(FVector(4.0,11.0,75.0));
    LegacyMuzzles.Add(FVector(-4.0,11.0,75.0));
}
