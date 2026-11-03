#include "MuzzleFlashExplosionActor.h"
#include "Misc/PackageName.h"
AMuzzleFlashExplosionActor::AMuzzleFlashExplosionActor(){ExplosionType=EExplosionType::NONE;VisualScale=.05f;FallbackLifetime=.1f;}
TSubclassOf<AExplosionActor> AMuzzleFlashExplosionActor::MuzzleClass()
{
 const FString P=TEXT("/Game/Effects/Explosions/MuzzleFlashExplosion/BP_MuzzleFlashExplosion");
 if(FPackageName::DoesPackageExist(P))if(UClass* C=LoadClass<AExplosionActor>(nullptr,*(P+TEXT(".BP_MuzzleFlashExplosion_C"))))return C;
 return StaticClass();
}
