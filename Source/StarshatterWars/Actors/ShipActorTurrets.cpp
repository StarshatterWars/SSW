#include "ShipActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "ShipDesignRegistry.h"
#include "Weapon.h"
#include "UObject/UObjectThreadContext.h"

namespace
{
    bool InTurretConstructor()
    {
        return FUObjectThreadContext::Get().IsInConstructor > 0;
    }
}

void AShipActor::ConfigureTurretsFromShipDef(const FString& ShipName)
{
    auto MakeScene = [this](FName Name, USceneComponent* Parent) -> USceneComponent*
    {
        USceneComponent* Comp = InTurretConstructor()
            ? CreateDefaultSubobject<USceneComponent>(Name)
            : NewObject<USceneComponent>(this, Name);
        if (!Comp) return nullptr;
        Comp->SetupAttachment(Parent ? Parent : GetRootComponent());
        if (!InTurretConstructor()) Comp->RegisterComponent();
        return Comp;
    };

    if (!TurretMountRoot)
    {
        TurretMountRoot = MakeScene(TEXT("TurretMountRoot"), RootComponent);
    }

    if (TurretMountDefs.Num() == 0)
    {
        const ShipDesign* Design = ShipDesignRegistry::FindLegacy(TCHAR_TO_ANSI(*ShipName));
        if (Design)
        {
            for (int i = 0; i < Design->weapons.size(); ++i)
            {
                Weapon* Gun = Design->weapons[i];
                if (!Gun || !Gun->GetTurret()) continue;

                FShipPointDef Point;
                Point.PointName = FName(*FString::Printf(TEXT("Turret_%d"), i));
                Point.LocalOffset = Gun->GetTurretMountRelative();
                TurretMountDefs.Add(Point);
            }
        }

        if (TurretMountDefs.Num() == 0)
        {
            for (const FShipPointDef& Point : TurretBasePointDefs)
                TurretMountDefs.Add(Point);
        }
    }

    if (!InTurretConstructor())
    {
        for (TObjectPtr<USceneComponent>& Mount : NativeTurretMounts)
        {
            if (Mount) Mount->DestroyComponent();
        }
    }
    NativeTurretMounts.Reset();

    for (int32 Index = 0; Index < TurretMountDefs.Num(); ++Index)
    {
        const FShipPointDef& Def = TurretMountDefs[Index];
        const FName Name = Def.PointName.IsNone()
            ? FName(*FString::Printf(TEXT("TurretMount_%d"), Index))
            : Def.PointName;

        USceneComponent* Mount = MakeScene(Name, TurretMountRoot);
        if (!Mount) continue;

        Mount->SetRelativeLocation(Def.LocalOffset);
        Mount->SetRelativeRotation(Def.LocalRotation);
        NativeTurretMounts.Add(Mount);
    }
}

void AShipActor::ClearTurretActors()
{
    if (InTurretConstructor()) return;

    for (TObjectPtr<AActor>& Turret : TurretComponents)
    {
        if (Turret) Turret->Destroy();
    }
    TurretComponents.Reset();
    SpawnedTurretTypes.Reset();

    for (TObjectPtr<USceneComponent>& Visual : TurretVisuals)
    {
        if (Visual) Visual->DestroyComponent();
    }
    TurretVisuals.Reset();
}

void AShipActor::RebuildTurretActors()
{
    if (InTurretConstructor() || !bEnableTurretActors || !GetWorld()) return;

    ClearTurretActors();

    for (int32 Index = 0; Index < TurretMountDefs.Num(); ++Index)
    {
        USceneComponent* Mount = NativeTurretMounts.IsValidIndex(Index)
            ? NativeTurretMounts[Index].Get() : nullptr;

        TSubclassOf<AActor> TurretClass = nullptr;
        if (TurretClassOverrides.IsValidIndex(Index) && TurretClassOverrides[Index])
            TurretClass = TurretClassOverrides[Index];
        else if (NativeTurretBPSlots.IsValidIndex(Index) && NativeTurretBPSlots[Index])
            TurretClass = NativeTurretBPSlots[Index];

        if (!TurretClass || !Mount)
        {
            if (Mount && !InTurretConstructor())
            {
                UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
                if (Mesh)
                {
                    UStaticMeshComponent* Visual = NewObject<UStaticMeshComponent>(this);
                    Visual->SetStaticMesh(Mesh);
                    Visual->SetupAttachment(Mount);
                    Visual->SetRelativeScale3D(FVector(0.25f, 0.25f, 0.4f));
                    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                    Visual->SetCastShadow(false);
                    Visual->RegisterComponent();
                    TurretVisuals.Add(Visual);
                }
            }
            continue;
        }

        FActorSpawnParameters Params;
        Params.Owner = this;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        AActor* Spawned = GetWorld()->SpawnActor<AActor>(TurretClass, Mount->GetComponentTransform(), Params);
        if (!Spawned) continue;

        Spawned->AttachToComponent(Mount, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
        TurretComponents.Add(Spawned);
        SpawnedTurretTypes.Add(TurretClass);
    }
}

void AShipActor::UpdateTurretActors()
{
    if (InTurretConstructor() || !bEnableTurretActors || TurretComponents.Num() == 0) return;

    for (int32 Index = 0; Index < TurretComponents.Num(); ++Index)
    {
        AActor* Turret = TurretComponents[Index];
        USceneComponent* Mount = NativeTurretMounts.IsValidIndex(Index)
            ? NativeTurretMounts[Index].Get() : nullptr;
        if (!Turret || !Mount) continue;

        Turret->SetActorLocationAndRotation(
            Mount->GetComponentLocation(),
            Mount->GetComponentRotation());
    }
}
