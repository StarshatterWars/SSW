#include "MiningBeamActor.h"

#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

AMiningBeamActor::AMiningBeamActor()
{
    WeaponDefinitionName = TEXT("Mining Laser");
    BeamColor = FLinearColor(0.35f, 3.2f, 2.4f, 1.0f);
    BeamWidth = 5.0f;
    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> Beam(
        TEXT("/Game/Beam_and_laserFX01/Particles/Niagara/NS_ky_laser01.NS_ky_laser01"));
    BeamSystem = Beam.Object;
}
