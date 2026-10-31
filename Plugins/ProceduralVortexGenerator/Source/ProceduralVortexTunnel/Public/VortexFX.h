


// Copyright 2018-2025 Athian Games. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VortexSpline.h"
#include "NiagaraComponent.h"


#include "VortexFX.generated.h"



USTRUCT(BlueprintType, Category = "Vortex")
struct FVortexFXInfo
{
	GENERATED_BODY()


	//Array of Niagara System, from which one of the FX will be randomly chosen for a given Particle
	UPROPERTY(EditAnywhere, Category = "FX Properties")
	TArray<UNiagaraSystem*> NiagaraSystems;

	UPROPERTY(EditAnywhere, Category = "FX Properties")
	bool bIsEnabled;

	
	//Radius of the Vortex Cylinder, within which the particles will spawn
	UPROPERTY(EditAnywhere, Category = "FX Properties")
	FFloatInterval VortexRadius;

	//SpawnRate of the Niagara Particles per 100 units
	UPROPERTY(EditAnywhere, Category = "FX Properties")
	FFloatInterval FXDensity;

	//Distance between the particles
	UPROPERTY(EditAnywhere, Category = "FX Properties")
	FFloatInterval FXDistance;

	//Length of the particles
	UPROPERTY(EditAnywhere, Category = "FX Properties")
	FFloatInterval FXLength;

	UPROPERTY(EditAnywhere, Category = "FX Properties")
	bool bAlignToSpline;

	//Distance Percentage from Start of the Source Spline, where the niagara particles will start spawning
	UPROPERTY(EditAnywhere, Category = "FX Properties", meta = (UIMin = "0.0", UIMax = "100.0", SliderMin = "0.0", SliderMax = "100.0"))
	float StartDistance;

	//Distance Percentage from Start of the Source Spline, upto which the niagara particles will spawn
	UPROPERTY(EditAnywhere, Category = "FX Properties", meta = (UIMin = "0.0", UIMax = "100.0", SliderMin = "0.0", SliderMax = "100.0"))
	float EndDistance;

	//Probabilty of the Particle Spawning
	UPROPERTY(EditAnywhere, Category = "FX Properties", meta = (UIMin = "0.0", UIMax = "100.0", SliderMin = "0.0", SliderMax = "100.0"))
	float SpawnProbability;

	FVortexFXInfo():StartDistance(0),
					 EndDistance(100),
					 SpawnProbability(100)
	{
		VortexRadius = FFloatInterval(300, 400);
		FXDensity = FFloatInterval(100, 200);
		FXDistance = FFloatInterval(500, 1000);
		FXLength = FFloatInterval(200, 400);
		bIsEnabled = true;
		bAlignToSpline = false;
	}
};



USTRUCT(BlueprintType)
struct FVortexFXComponentInfo
{
	GENERATED_BODY()

	UPROPERTY()
	UNiagaraComponent* NiagaraComponent;


	UPROPERTY()
	TArray<FVector> SplineDirections;

	UPROPERTY()
	UNiagaraSystem* NiagaraSystem;

	UPROPERTY()
	FVector FXLocation;

	UPROPERTY()
	FVector FXDirection;

	UPROPERTY()
	float SpawnRate;

	UPROPERTY()
	float VortexRadius;

	UPROPERTY()
	bool bAlignToSpline;

	FVortexFXComponentInfo(): NiagaraComponent(nullptr), NiagaraSystem(nullptr), SpawnRate(0), VortexRadius(0),
	                          bAlignToSpline(false)
	{
		FXLocation = FVector(0, 0, 0);
	}

	void DestroyFX()
	{
		if(NiagaraComponent && NiagaraComponent->IsValidLowLevel())
		{
			NiagaraComponent->DestroyComponent();
			NiagaraComponent = nullptr;
		}
			
	}
	

	void CreateFX(AActor* Owner,  int32 GroupIndex, int32 FXIndex);
};

USTRUCT(BlueprintType)
struct FVortexFXGroup
{
	GENERATED_BODY()

	UPROPERTY()
	float StartDistance;
	
	UPROPERTY()
	float EndDistance;
	
	UPROPERTY()
	bool GroupEntered;
	
	UPROPERTY()
    bool GroupExited;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VortexFX")
	TArray<FVortexFXComponentInfo> VortexFXComponentInfo;

	void ResetGroup()
	{
		GroupEntered = false;
		GroupExited = false;
		
	}

	void CreateFXInRange(AActor* Owner, float CurrentDistance, float MinEnterDistance, int32 GroupIndex);

	void DestroyFXOutSideRange(float CurrentDistance, float MinExitDistance);
};

UCLASS()
class PROCEDURALVORTEXTUNNEL_API UVortexFXData : public UObject
{
	GENERATED_BODY()

public:
	
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Vortex")
	TArray<FVortexFXInfo> FXInfo;

};

UCLASS()
class PROCEDURALVORTEXTUNNEL_API AVortexFX : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AVortexFX();

#if WITH_EDITOR

	virtual void OnConstruction(const FTransform& Transform) override;
		
#endif

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	bool bIsConstructed;
	
	UPROPERTY()
	TArray<FVortexFXGroup> VortexFXGroups;


	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Source")
	AVortexSpline* SourceSpline;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Niagara")
	UVortexFXData* FXData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group")
	float GroupLength;

	UFUNCTION(BlueprintCallable, Category = "VortexFX")
	void GenerateNiagaraFX();

	UFUNCTION(BlueprintCallable, Category = "VortexFX")
	void ResetNiagaraFX();

	UFUNCTION(BlueprintCallable, Category = "VortexFX", meta=(DisplayName = "Generate Niagara FX By Group"))
	void GenerateNiagaraFXByGroup(float CurrentDistance, float EnterDistance);

	UFUNCTION(BlueprintCallable, Category = "VortexFX",  meta=(DisplayName = "Destroy Niagara FX By Group"))
	void DestroyNiagaraFXByGroup(float CurrentDistance, float ExitDistance);


};


