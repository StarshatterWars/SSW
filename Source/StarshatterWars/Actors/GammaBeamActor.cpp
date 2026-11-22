#include "GammaBeamActor.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"
AGammaBeamActor::AGammaBeamActor(){WeaponDefinitionName=TEXT("Gamma Laser");LegacyBitmap=TEXT("ShotGamma");BeamWidth=25.0f;BeamColor=FLinearColor(0.8f,1.2f,10.0f,1.0f);
 if(IsRunningDedicatedServer()) return;
 static ConstructorHelpers::FObjectFinder<UNiagaraSystem> NativeBeam(TEXT("/Game/Beam_and_laserFX01/Particles/Niagara/NS_ky_beam04.NS_ky_beam04"));
 BeamSystem=NativeBeam.Object;
}
