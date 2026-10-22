#include "TrellisPlanetActor.h"
#include "ProceduralMeshComponent.h"

ATrellisPlanetActor::ATrellisPlanetActor()
{
    bForceRings = true;
    RingMaterialName = TEXT("Ring2");
    ForcedInnerRingRatio = 1.2f;
    ForcedOuterRingRatio = 1.9f;
    PlanetRadiusUnits = 3500.0f;
}

void ATrellisPlanetActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    RefreshTrellisRings();
}

void ATrellisPlanetActor::BeginPlay()
{
    Super::BeginPlay();
    // Enforce after Blueprint BeginPlay as well as native/BP construction.
    RefreshTrellisRings();
}

void ATrellisPlanetActor::RefreshTrellisRings()
{
    bForceRings = true;
    if (RingMaterialName.TrimStartAndEnd().IsEmpty()) RingMaterialName = TEXT("Ring2");
    SetRingMaterialByName(RingMaterialName);
    const bool bReady = HasSpawnedRings();
    UE_LOG(LogTemp, Display, TEXT("[TrellisRings] Actor=%s Class=%s Ready=%d Sections=%d Material=%s"),
        *GetName(), *GetClass()->GetPathName(), bReady ? 1 : 0,
        IsValid(RuntimeRingMesh) ? RuntimeRingMesh->GetNumSections() : 0,
        IsValid(RuntimeRingMesh) ? *GetNameSafe(RuntimeRingMesh->GetMaterial(0)) : TEXT("None"));
}

bool ATrellisPlanetActor::HasSpawnedRings() const
{
    if (!IsValid(RuntimeRingMesh) || !RuntimeRingMesh->IsRegistered() ||
        !RuntimeRingMesh->IsVisible() || RuntimeRingMesh->bHiddenInGame || IsHidden() ||
        !RuntimeRingMesh->GetMaterial(0)) return false;
    const FProcMeshSection* Section = RuntimeRingMesh->GetProcMeshSection(0);
    return Section && Section->bSectionVisible && Section->ProcVertexBuffer.Num() > 0 &&
        Section->ProcIndexBuffer.Num() >= 3;
}
