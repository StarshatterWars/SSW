// Copyright 2018-2025 Athian Games. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "VortexSpline.h"
#include "PVTLibrary.h"
#include "VortexData.h"
#include "NiagaraComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Actor.h"
#include "VortexRepeater.generated.h"



USTRUCT(BlueprintType, Category = "Vortex")
struct FVortexRepeaterInfo
{
	GENERATED_BODY()


	UPROPERTY(EditAnywhere, Category = "Repeater Properties")
	TArray<FVortexMeshData> RepeaterMeshes;

	UPROPERTY(EditAnyWhere, Category = "Repeater|Properties")
	TArray<FVortexMaterialData> DefaultMaterials;

	UPROPERTY(EditAnyWhere, Category = "Repeater|Properties")
	bool bIsEnabled;
	
	UPROPERTY(EditAnyWhere, Category = "Repeater|Properties")
	bool bEvenDistribution;

	/*Probability of Repeater Mesh Generation in Percentage*/
	UPROPERTY(EditAnyWhere, Category = "Repeater", meta = (UIMin = "0.0", UIMax = "100.0",  SLiderMin = "0.0f", SLiderMax = "100.0f"))
	float Probability;

	UPROPERTY(EditAnyWhere, Category = "Repeater")
	FFloatInterval Distance;

	/*Start Distance Percentage relative to the Spline*/
	UPROPERTY(EditAnyWhere, Category = "Repeater", meta = (EditCondition = "!bEvenDistribution", UIMin = "0.0", UIMax = "100.0",  SLiderMin = "0.0f", SLiderMax = "100.0f"))
	float StartDistance;

	/*End Distance Percentage relative to the Spline*/
	UPROPERTY(EditAnyWhere, Category = "Repeater", meta = (EditCondition = "!bEvenDistribution", UIMin = "0.0", UIMax = "100.0",  SLiderMin = "0.0f", SLiderMax = "100.0f"))
	float EndDistance;

	UPROPERTY(EditAnyWhere, Category = "Repeater|Properties")
	FFloatInterval Scale;

	UPROPERTY(EditAnyWhere, Category = "Repeater|Properties")
	FVector RingOfAvoidance;

	UPROPERTY(EditAnyWhere, Category = "Repeater|Properties")
	FVector MinLocationOffset;

	UPROPERTY(EditAnyWhere, Category = "Repeater|Properties")
	FVector MaxLocationOffset;
	
	UPROPERTY(EditAnyWhere, Category = "Repeater|Properties")
	FRotator StartRotation;

	UPROPERTY(EditAnyWhere, Category = "Repeater|Properties")
	FRotator EndRotation;

	UPROPERTY(EditAnyWhere, Category = "Repeater|Properties")
	FRotator RotationOffset;


	
	FVortexRepeaterInfo(): bIsEnabled(true), bEvenDistribution(false), Probability(100),
	                       StartDistance(0),
	                       EndDistance(100)

	{
		Distance = FFloatInterval(400, 1000);

		Scale = FFloatInterval(1, 1);

		StartRotation = FRotator(0, 0, 0);
		EndRotation = FRotator(0, 0, 0);

		MinLocationOffset = FVector(-100, -100, -100);
		MaxLocationOffset = FVector(100, 100, 100);

		RotationOffset = FRotator(0, 0, 0);
	}
};


UCLASS()
class PROCEDURALVORTEXTUNNEL_API UVortexRepeaterData : public UObject
{
	GENERATED_BODY()

public:
	
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Vortex")
	TArray<FVortexRepeaterInfo> VortexRepeaterInfo;
	
};

USTRUCT()
struct FVortexRepeaterMeshData
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<UMaterialInterface*, UMaterialInstanceDynamic*> DynamicMaterialMap;

	UPROPERTY()
	TArray<UStaticMeshComponent*> RepeaterMeshComponents;

	UPROPERTY()
	TArray<UChildActorComponent*> RepeaterActorComponents;

	void CreateRepeater(AActor* Owner, FVortexRepeaterInfo RepeaterInfo, FTransform RepeaterTransform, int32 GroupIndex, int32 RepeaterIndex);

	void GenerateNextRepeaterData(AActor* Owner, const USplineComponent* CurrentSpline, FVortexRepeaterInfo RepeaterInfo, float CurrentDistance, FTransform& RepeaterTransform,
	                              int32 GroupIndex, int32 RepeaterIndex, UBoxComponent* BoundingBox, bool IsUnBound);

	void GenerateRepeater(AActor* Owner, USplineComponent* CurrentSpline, FVortexRepeaterInfo RepeaterInfo, int32 GroupIndex, UBoxComponent* BoundingBox, bool IsUnBound)
	{
		if(CurrentSpline)
		{
			float SplineLength = CurrentSpline->GetSplineLength();
			float StartDistanceCovered = RepeaterInfo.StartDistance * SplineLength / 100;
			float EndDistanceToCover = RepeaterInfo.EndDistance * SplineLength / 100;

			float RandomDistance = StartDistanceCovered + FMath::RandRange(RepeaterInfo.Distance.Min, RepeaterInfo.Distance.Max);
			RandomDistance = FMath::Min(RandomDistance, EndDistanceToCover);

			int32 Index = 0;
			if(RepeaterInfo.Distance.Max - RepeaterInfo.Distance.Min > 10)
			{
				while(RandomDistance < EndDistanceToCover && RepeaterActorComponents.Num() < 100)
				{
					FTransform RepeaterTransform;
					GenerateNextRepeaterData(Owner, CurrentSpline, RepeaterInfo, RandomDistance, RepeaterTransform, GroupIndex, Index, BoundingBox, IsUnBound);
					RandomDistance += FMath::RandRange(RepeaterInfo.Distance.Min, RepeaterInfo.Distance.Max);
					RandomDistance = FMath::Min(RandomDistance, EndDistanceToCover);
					Index++;
				}
			}
			
		
		}
	}

	void ResetRepeater()
	{
		for (int iIndex = 0; iIndex < RepeaterMeshComponents.Num(); iIndex++)
		{
			if(RepeaterMeshComponents[iIndex])
			{
				RepeaterMeshComponents[iIndex]->DestroyComponent();
				RepeaterMeshComponents[iIndex] = nullptr;
			}
		}

		RepeaterMeshComponents.Empty();

		for (int iIndex = 0; iIndex < RepeaterActorComponents.Num(); iIndex++)
		{
			if(RepeaterActorComponents[iIndex])
			{
				RepeaterActorComponents[iIndex]->DestroyChildActor();
				RepeaterActorComponents[iIndex]->DestroyComponent();
				RepeaterActorComponents[iIndex] = nullptr;
			}
		}

		RepeaterActorComponents.Empty();

   }

	
};

UCLASS()
class PROCEDURALVORTEXTUNNEL_API AVortexRepeater : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AVortexRepeater();

	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "Billboard")
	UBillboardComponent* RootBillboard;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category=Defaulf)
	UBoxComponent* BoundingBox;
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY()
	TArray<FVortexRepeaterMeshData> VortexRepeaterMeshData;
	
	UPROPERTY(EditAnyWhere, Category = "Source")
	AVortexSpline* SourceSpline;
	
	UPROPERTY(EditAnyWhere, BlueprintReadOnly, Category="Repeater")
	bool bIsUnbound;
	
	UPROPERTY(EditAnyWhere, BlueprintReadOnly, Category="Repeater")
	UVortexRepeaterData* RepeaterData;

	UFUNCTION(BlueprintCallable, Category = "VortexFX")
	void ResetRepeaters();
	
	UFUNCTION(BlueprintCallable, Category="Repeater")
	void GenerateRepeaters();

};
