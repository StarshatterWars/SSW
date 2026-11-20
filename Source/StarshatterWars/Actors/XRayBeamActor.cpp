#include "XRayBeamActor.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"
AXRayBeamActor::AXRayBeamActor(){WeaponDefinitionName=TEXT("XRay Laser");LegacyBitmap=TEXT("shot9");BeamWidth=20.0f;BeamColor=FLinearColor(6.0f,5.4f,0.3f,1.0f);
 static ConstructorHelpers::FObjectFinder<UNiagaraSystem> NativeBeam(TEXT("/Game/Beam_and_laserFX01/Particles/Niagara/NS_ky_beam03.NS_ky_beam03"));
 BeamSystem=NativeBeam.Object;
}
