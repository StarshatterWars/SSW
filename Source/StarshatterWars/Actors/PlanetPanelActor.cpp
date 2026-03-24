#include "PlanetPanelActor.h"


APlanetPanelActor::APlanetPanelActor()
{
}

APlanetPanelActor* APlanetPanelActor::SpawnWithPlanetData(
	UWorld* World,
	const FVector& Location,
	const FRotator& Rotation,
	TSubclassOf<APlanetPanelActor> ActorClass,
	const FS_PlanetMap& PlanetInfo,
	ASystemBodyPanelActor* OrbitAuthority,
	const FString& SystemSeed
)
{
	if (!World || !*ActorClass) return nullptr;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.bDeferConstruction = true;

	APlanetPanelActor* NewActor = World->SpawnActor<APlanetPanelActor>(ActorClass, Location, Rotation, Params);
	if (!NewActor) return nullptr;

	NewActor->PlanetData = PlanetInfo;
	NewActor->BodyTilt = PlanetInfo.Tilt;
	NewActor->BodyRadius = PlanetInfo.Radius;
	NewActor->BodyName = PlanetInfo.Name;
	NewActor->BodyType = EBodyUISizeClass::Planet;
	NewActor->TextureName = PlanetInfo.Texture;

	// Your existing setup
	NewActor->InitBody("Planet");

	// -------------------- ORBIT SETUP (NEW) --------------------
	// Enable orbit here so base Tick moves the actor.
	if (OrbitAuthority)
	{
		// Radius used for orbit must be the orbital radius (km), notxbody radius.
		// I’m assuming FS_PlanetMap has OrbitKm (or Orbit). Use your actual field name.
		const float OrbitKm = PlanetInfo.Orbit; // <-- adjust field name if different

		NewActor->InitializeOrbit(
			OrbitAuthority,
			OrbitKm,
			SystemSeed + TEXT("_") + PlanetInfo.Name
		);

		// Optional: keep planet orbits in XY plane at star's Z (or system plane)
		// If you want a system-plane Z offset, set CustomOrbitCenter on authority instead.
	}
	// -----------------------------------------------------------

	NewActor->FinishSpawning(FTransform(Rotation, Location));
	return NewActor;
}



