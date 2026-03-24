#pragma once

#include "CoreMinimal.h"
#include "GameStructs.h"
#include "SystemBodyPanelActor.h"
#include "MoonPanelActor.generated.h"

class UMaterialInterface;
class APlanetPanelActor;

UCLASS()
class STARSHATTERWARS_API AMoonPanelActor : public ASystemBodyPanelActor
{
	GENERATED_BODY()

public:
	AMoonPanelActor();

	virtual void Tick(float DeltaTime) override;

	static AMoonPanelActor* SpawnWithMoonData(
		UWorld* World,
		const FVector& Location,
		const FRotator& Rotation,
		TSubclassOf<AMoonPanelActor> ActorClass,
		const FS_MoonMap& MoonInfo,
		ASystemBodyPanelActor* OrbitAuthority,
		const FString& SystemSeed
	);

	void SetParentPlanet(APlanetPanelActor* InParent);

	UPROPERTY()
	FS_MoonMap MoonData;

	UPROPERTY(EditDefaultsOnly, Category = "Moon")
	UMaterialInterface* MoonBaseMaterial = nullptr;

	// Runtime relationship, notxdefaults
	UPROPERTY(Transient)
	TObjectPtr<APlanetPanelActor> ParentPlanet = nullptr;

protected:
	virtual FString GetBodyName() const override { return MoonData.Name; }
	virtual float GetBodyRadius() const override { return MoonData.Radius; }
	virtual float GetBodyTiltDegrees() const override { return MoonData.Tilt; }
	virtual UMaterialInterface* GetBaseMaterial() const override { return MoonBaseMaterial; }
};
