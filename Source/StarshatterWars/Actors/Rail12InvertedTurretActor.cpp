#include "Rail12InvertedTurretActor.h"
ARail12InvertedTurretActor::ARail12InvertedTurretActor()
{
    WeaponType=TEXT("Rail-12 Inverted");
    LegacyGunModel=TEXT("Rail_12TurretInverted");
    LegacyBaseModel=TEXT("");
    LegacyModelScale=3.0f;
    BaseAxis=0;
    LegacyMuzzles.Add(FVector(4.0,-11.0,75.0));
    LegacyMuzzles.Add(FVector(-4.0,-11.0,75.0));
}
