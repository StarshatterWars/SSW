#include "BlockadeCannonTurretActor.h"
ABlockadeCannonTurretActor::ABlockadeCannonTurretActor()
{
    WeaponType=TEXT("Blockade Cannon");
    LegacyGunModel=TEXT("BlockadeCannonTurret");
    LegacyBaseModel=TEXT("BlockadeTurretBase");
    LegacyModelScale=1.0f;
    BaseAxis=0;
}
