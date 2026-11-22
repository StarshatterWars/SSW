#include "ZolonBeamActor.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"
AZolonBeamActor::AZolonBeamActor(){WeaponDefinitionName=TEXT("Zolon Laser");LegacyBitmap=TEXT("ShotZBeam");BeamWidth=25.0f;BeamColor=FLinearColor(0.3f,8.0f,1.8f,1.0f);
 if(IsRunningDedicatedServer()) return;
 static ConstructorHelpers::FObjectFinder<UNiagaraSystem> NativeBeam(TEXT("/Game/Beam_and_laserFX01/Particles/Niagara/NS_ky_beam09.NS_ky_beam09"));
 BeamSystem=NativeBeam.Object;
}
