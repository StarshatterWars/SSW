#include "MoonPanelActor.h"
#include "PlanetPanelActor.h"

AMoonPanelActor::AMoonPanelActor()
{

}

AMoonPanelActor* AMoonPanelActor::SpawnWithMoonData(
    UWorld* World,
    const FVector& Location,
    const FRotator& Rotation,
    TSubclassOf<AMoonPanelActor> ActorClass,
    const FS_MoonMap& MoonInfo,
    ASystemBodyPanelActor* OrbitAuthority,
    const FString& SystemSeed
)
{
    if (!World || !*ActorClass) return nullptr;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Params.bDeferConstruction = true;

    AMoonPanelActor* NewActor = World->SpawnActor<AMoonPanelActor>(ActorClass, Location, Rotation, Params);
    if (!NewActor) return nullptr;

    // -------------------- BODY DATA --------------------
    NewActor->MoonData = MoonInfo;
    NewActor->BodyTilt = MoonInfo.Tilt;
    NewActor->BodyRadius = MoonInfo.Radius;
    NewActor->BodyName = MoonInfo.Name;
    NewActor->BodyType = EBodyUISizeClass::Moon;
    NewActor->TextureName = MoonInfo.Texture;

    NewActor->InitBody("Moon");

    // -------------------- ORBIT DATA --------------------
    // IMPORTANT: MoonInfo.Orbit is assumed to be in km (or consistent with your system map data).
    // If MoonInfo.Orbit is notxkm, convert before passing.
    if (OrbitAuthority)
    {
        const FString Seed = SystemSeed.IsEmpty()
            ? (OrbitAuthority->SystemSeed + TEXT("_") + MoonInfo.Name)
            : (SystemSeed + TEXT("_") + MoonInfo.Name);

        const float OrbitRadiusKm = MoonInfo.Orbit; // <-- if your MoonInfo.Orbit is already km

        if (OrbitRadiusKm > 0.f)
        {
            NewActor->InitializeOrbit(OrbitAuthority, OrbitRadiusKm, Seed);

            // Moons orbit a moving planet: we will supply the center dynamically
            NewActor->bUseCustomOrbitCenter = true;
        }
    }
    else
    {
        NewActor->bEnableOrbit = false;
    }

    NewActor->FinishSpawning(FTransform(Rotation, Location));
    return NewActor;
}

void AMoonPanelActor::SetParentPlanet(APlanetPanelActor* InParent)
{
    ParentPlanet = InParent;

    if (ParentPlanet)
    {
        bUseCustomOrbitCenter = true;
        CustomOrbitCenter = ParentPlanet->GetActorLocation();
    }

    const FString ParentName = InParent ? InParent->PlanetData.Name : TEXT("None");
    UE_LOG(LogTemp, Warning, TEXT("SetParentPlanet() Parent: %s -> Moon: %s"), *ParentName, *MoonData.Name);
}

void AMoonPanelActor::Tick(float DeltaTime)
{
    // Update orbit center to follow the moving planet
    if (ParentPlanet)
    {
        bUseCustomOrbitCenter = true;
        CustomOrbitCenter = ParentPlanet->GetActorLocation();
    }

    Super::Tick(DeltaTime);
}


