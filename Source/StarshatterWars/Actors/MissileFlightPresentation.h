#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/MovementComponent.h"
#include "Components/PrimitiveComponent.h"

namespace MissileFlightPresentation
{
    inline void Release(AActor* Missile)
    {
        if (!Missile) return;
        // Flight visuals have an independent lifetime, governed by SimShot.
        // Keep the launch transform while releasing any Blueprint attachment.
        Missile->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        Missile->SetOwner(nullptr);
        Missile->SetActorEnableCollision(false);
        Missile->SetActorTickEnabled(false);
        TArray<UMovementComponent*> Movement;
        Missile->GetComponents(Movement);
        for (UMovementComponent* Component : Movement) Component->SetComponentTickEnabled(false);
        TArray<UPrimitiveComponent*> Primitives;
        Missile->GetComponents(Primitives);
        for (UPrimitiveComponent* Component : Primitives)
        {
            Component->SetSimulatePhysics(false);
            Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Component->SetMobility(EComponentMobility::Movable);
        }
    }
}
