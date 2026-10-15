#include "BlockadeCannonTurretActor.h"
ABlockadeCannonTurretActor::ABlockadeCannonTurretActor()
{
    WeaponType=TEXT("Blockade Cannon");
    LegacyGunModel=TEXT("mine_guns.mag");
    LegacyBaseModel=TEXT("mine_base.mag");
    LegacyModelScale=1.0f;
    BaseAxis=0;
}
