#include "GraserBeamActor.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"
AGraserBeamActor::AGraserBeamActor(){WeaponDefinitionName=TEXT("Graser Turret");LegacyBitmap=TEXT("ShotGamma");BeamWidth=50.0f;BeamColor=FLinearColor(0.8f,1.2f,10.0f,1.0f);
 static ConstructorHelpers::FObjectFinder<UNiagaraSystem> NativeBeam(TEXT("/Game/Beam_and_laserFX01/Particles/Niagara/NS_ky_beam04.NS_ky_beam04"));
 BeamSystem=NativeBeam.Object;
}
