#include "ShipActor.h"
#include "Components/ChildActorComponent.h"

void AShipActor::ConfigureTurretsFromShipDef(const FString& ShipName)
{
    // Compatibility entry point; constructors author native points after defaults.
    // In particular, Orion calls here before its authored arrays are populated.
    NativeTurretMounts = AuthoredTurretPointComponents;
}

void AShipActor::ClearTurretActors()
{
    // UChildActorComponent owns the lifetime of each visual actor.
    TurretComponents.Reset();
    SpawnedTurretTypes.Reset();
    TurretVisuals.Reset();
}

void AShipActor::RebuildTurretActors()
{
    ClearTurretActors();
    NativeTurretMounts = AuthoredTurretPointComponents;
    TurretMountDefs = TurretBasePointDefs;
    for (int32 Index = 0; Index < AuthoredTurretPointComponents.Num(); ++Index)
    {
        UChildActorComponent* Mount = Cast<UChildActorComponent>(AuthoredTurretPointComponents[Index].Get());
        if (!Mount) continue;
        if (bEnableTurretActors)
        {
            if (TurretBlueprintClasses.IsValidIndex(Index) && TurretBlueprintClasses[Index])
                Mount->SetChildActorClass(TurretBlueprintClasses[Index]);
            else if (TurretClassOverrides.IsValidIndex(Index) && TurretClassOverrides[Index])
                Mount->SetChildActorClass(TurretClassOverrides[Index]);
            else if (NativeTurretBPSlots.IsValidIndex(Index) && NativeTurretBPSlots[Index])
                Mount->SetChildActorClass(NativeTurretBPSlots[Index]);
        }
        Mount->SetVisibility(bEnableTurretActors, true);
        if (AActor* Child = Mount->GetChildActor())
        {
            Child->SetActorHiddenInGame(!bEnableTurretActors);
            TurretComponents.Add(Child);
            SpawnedTurretTypes.Add(Mount->GetChildActorClass());
        }
    }
}

void AShipActor::UpdateTurretActors()
{
    // Child actors inherit their native mount transform without a second spawn path.
}
