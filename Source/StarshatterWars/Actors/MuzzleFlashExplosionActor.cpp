#include "MuzzleFlashExplosionActor.h"
#include "Misc/PackageName.h"
#include "Components/MaterialBillboardComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
AMuzzleFlashExplosionActor::AMuzzleFlashExplosionActor(){ExplosionType=EExplosionType::NONE;VisualScale=.05f;FallbackLifetime=.1f;
 FlashGlow=CreateDefaultSubobject<UMaterialBillboardComponent>(TEXT("FlashGlow"));FlashGlow->SetupAttachment(EffectRoot);FlashGlow->SetCollisionEnabled(ECollisionEnabled::NoCollision);FlashGlow->SetCastShadow(false);
 static ConstructorHelpers::FObjectFinder<UMaterialInterface> M(TEXT("/Game/Models/Weapons/Bolt/Shared/M_BoltGlow.M_BoltGlow"));if(M.Succeeded())GlowMaterial=M.Object;
}
TSubclassOf<AExplosionActor> AMuzzleFlashExplosionActor::MuzzleClass()
{
 const FString P=TEXT("/Game/Effects/Explosions/MuzzleFlashExplosion/BP_MuzzleFlashExplosion");
 if(FPackageName::DoesPackageExist(P))if(UClass* C=LoadClass<AExplosionActor>(nullptr,*(P+TEXT(".BP_MuzzleFlashExplosion_C"))))return C;
 return StaticClass();
}

void AMuzzleFlashExplosionActor::SetFlashAppearance(const FString& FlashId,float Scale)
{
 if(!GlowMaterial)return;
 const FString Id=FPaths::GetBaseFilename(FlashId).ToLower();FLinearColor Color=FLinearColor::White;
 if(Id==TEXT("flash1"))Color=FLinearColor(.15,1,.25);
 else if(Id==TEXT("flash2") || Id==TEXT("flash3"))Color=FLinearColor(.7,.2,1);
 else if(Id==TEXT("flash4") || Id==TEXT("flash6") || Id==TEXT("flash9"))Color=FLinearColor(1,.85,.1);
 else if(Id==TEXT("flash5"))Color=FLinearColor(1,.1,.05);
 else if(Id==TEXT("flash7"))Color=FLinearColor(.15,.4,1);
 GlowMID=UMaterialInstanceDynamic::Create(GlowMaterial,this);GlowMID->SetVectorParameterValue(TEXT("BoltColor"),Color);GlowMID->SetScalarParameterValue(TEXT("GlowIntensity"),6.f);
 FMaterialSpriteElement E;E.Material=GlowMID;E.BaseSizeX=E.BaseSizeY=20.f*FMath::Abs(Scale);FlashGlow->SetElements({E});
}
