#include "FighterLandingGearComponent.h"

UFighterLandingGearComponent::UFighterLandingGearComponent()
{
    bEditableWhenInherited = true;
    SetMobility(EComponentMobility::Movable);
    PrimaryComponentTick.bCanEverTick = false;
}

void UFighterLandingGearComponent::ApplyDeployment(float Fraction, bool bEnabled, bool bStateVisible)
{
    Deployment = FMath::IsFinite(Fraction) ? FMath::Clamp(Fraction, 0.f, 1.f) : 0.f;
    if (bAnimateTranslation)
        SetRelativeLocation(FMath::Lerp(RetractedPosition, ExtendedPosition, Deployment));
    bGearVisible = bEnabled && bStateVisible && (!bHideWhenRetracted || Deployment > 0.f);
    SetVisibility(bGearVisible, true);
    if (AActor* Child = GetChildActor()) Child->SetActorHiddenInGame(!bGearVisible);
}
