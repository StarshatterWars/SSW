#pragma once

#include "CoreMinimal.h"
#include "GameStructs.h"
#include "SystemBodyPanelActor.h"
#include "SystemMapUtils.h"
#include "PlanetPanelActor.generated.h"

class UMaterialInterface;

UCLASS()
class STARSHATTERWARS_API APlanetPanelActor : public ASystemBodyPanelActor
{
	GENERATED_BODY()

public:
	APlanetPanelActor();

	/*static APlanetPanelActor* SpawnWithPlanetData(
		UWorld* World,
		const FVector& Location,
		const FRotator& Rotation,
		TSubclassOf<APlanetPanelActor> ActorClass,
		FS_PlanetMap PlanetInfo
	);*/

	static APlanetPanelActor* SpawnWithPlanetData(
		UWorld* World,
		const FVector& Location,
		const FRotator& Rotation,
		TSubclassOf<APlanetPanelActor> ActorClass,
		const FS_PlanetMap& PlanetInfo,
		ASystemBodyPanelActor* OrbitAuthority,
		const FString& SystemSeed
	);

	// Data payload
	UPROPERTY()
	FS_PlanetMap PlanetData;

	UPROPERTY(EditDefaultsOnly, Category = "Planet")
	UMaterialInterface* PlanetBaseMaterial = nullptr;

protected:
	// Base hooks
	virtual FString GetBodyName() const override { return PlanetData.Name; }
	virtual float GetBodyRadius() const override { return PlanetData.Radius; }
	virtual float GetBodyTiltDegrees() const override { return PlanetData.Tilt; }
	virtual UMaterialInterface* GetBaseMaterial() const override { return PlanetBaseMaterial; }
};
