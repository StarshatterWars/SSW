// Copyright 2018-2025 Athian Games. All Rights Reserved. 


#include "VortexRepeater.h"

#include "Components/BillboardComponent.h"

void FVortexRepeaterMeshData::CreateRepeater(AActor* Owner, FVortexRepeaterInfo RepeaterInfo,
	FTransform RepeaterTransform, int32 GroupIndex, int32 RepeaterIndex)
{
	if(Owner)
	{
		if(RepeaterInfo.RepeaterMeshes.Num() > 0)
		{
			int32 RandomMeshIndex = FMath::RandRange(0, RepeaterInfo.RepeaterMeshes.Num() - 1);
			FString RepeaterName = "RepeaterMesh_" + FString::FromInt(GroupIndex) + "_" + FString::FromInt(RepeaterIndex);
			UStaticMeshComponent* RMeshComponent = NewObject<UStaticMeshComponent>(Owner, UStaticMeshComponent::StaticClass(), *RepeaterName, RF_Transactional);
			RMeshComponent->SetupAttachment(Owner->GetRootComponent());
			RMeshComponent->SetStaticMesh(RepeaterInfo.RepeaterMeshes[RandomMeshIndex].Mesh);
			RMeshComponent->SetWorldTransform(RepeaterTransform);

			TMap<int32, UMaterialInterface*> MaterialIDMap;
			UPVTLibrary::GetMaterialIDMap(Owner, RepeaterInfo.RepeaterMeshes[RandomMeshIndex].CustomMaterials, MaterialIDMap);
			UPVTLibrary::GetMaterialIDMap(Owner, RepeaterInfo.DefaultMaterials, MaterialIDMap);

			TArray<int32> MaterialIDs;
			MaterialIDMap.GetKeys(MaterialIDs);
		
			for (int iIndex = 0; iIndex < MaterialIDs.Num(); iIndex++)
			{
				int CurrentMaterialID = MaterialIDs[iIndex];
				UMaterialInstanceDynamic* DynamicMaterial = UPVTLibrary::FindOrAssignDynamicMaterial(Owner, MaterialIDMap[CurrentMaterialID], DynamicMaterialMap);

				if (DynamicMaterial)
				{
					RMeshComponent->SetMaterial(CurrentMaterialID, DynamicMaterial);
			
				}
			}

			RMeshComponent->RegisterComponent();
			RepeaterMeshComponents.Add(RMeshComponent);
		}
	}
	
}

void FVortexRepeaterMeshData::GenerateNextRepeaterData(AActor* Owner, const USplineComponent* CurrentSpline,
	FVortexRepeaterInfo RepeaterInfo, float CurrentDistance, FTransform& RepeaterTransform, int32 GroupIndex,
	int32 RepeaterIndex, UBoxComponent* BoundingBox, bool IsUnBound)
{

	FVector RepeaterLocation = CurrentSpline->GetLocationAtDistanceAlongSpline(CurrentDistance, ESplineCoordinateSpace::World);
	FVector RandomLocationOffset = FMath::Lerp(RepeaterInfo.MinLocationOffset, RepeaterInfo.MaxLocationOffset, FMath::RandRange(0.0f ,1.0f));

	
	if(RandomLocationOffset.X <= 0 && RandomLocationOffset.X >= -RepeaterInfo.RingOfAvoidance.X)
	{
		RandomLocationOffset.X -= -RepeaterInfo.RingOfAvoidance.X;
	}
	else if(RandomLocationOffset.X > 0 && RandomLocationOffset.X <= RepeaterInfo.RingOfAvoidance.X)
	{
		RandomLocationOffset.X += RepeaterInfo.RingOfAvoidance.X;
	}

	if(RandomLocationOffset.Y <= 0 && RandomLocationOffset.Y >= -RepeaterInfo.RingOfAvoidance.Y)
	{
		RandomLocationOffset.Y -= RepeaterInfo.RingOfAvoidance.Y;
	}
	else if(RandomLocationOffset.Y > 0 && RandomLocationOffset.Y <= RepeaterInfo.RingOfAvoidance.Y)
	{
		RandomLocationOffset.Y += RepeaterInfo.RingOfAvoidance.Y;
	}

	if(RandomLocationOffset.Z <= 0 && RandomLocationOffset.Z >= -RepeaterInfo.RingOfAvoidance.Z)
	{
		RandomLocationOffset.Z -= RepeaterInfo.RingOfAvoidance.Z;
	}
	else if(RandomLocationOffset.Z > 0 && RandomLocationOffset.Z <= RepeaterInfo.RingOfAvoidance.Z)
	{
		RandomLocationOffset.Z += RepeaterInfo.RingOfAvoidance.Z;
	}
	
	
	RepeaterLocation += RandomLocationOffset;
					
	bool bValidRepeaterData = false;
	if(IsUnBound)
	{
		bValidRepeaterData = true;
	}
	else
	{
		if(BoundingBox && BoundingBox->IsValidLowLevel())
		{
			if(BoundingBox->Bounds.GetBox().IsInsideOrOn(RepeaterLocation))
			{
				bValidRepeaterData = true;
			}
		}
	}
		
	if(bValidRepeaterData)
	{
		int32 RandomSpawmProbability = FMath::RandRange(0,100);
		if(RandomSpawmProbability < RepeaterInfo.Probability)
		{
			FRotator RepeaterRotation = CurrentSpline->GetRotationAtDistanceAlongSpline(CurrentDistance, ESplineCoordinateSpace::World);
					
			FRotator RandomRotation;
			RandomRotation.Pitch = FMath::RandRange(RepeaterInfo.StartRotation.Pitch, RepeaterInfo.EndRotation.Pitch);
			RandomRotation.Yaw = FMath::RandRange(RepeaterInfo.StartRotation.Yaw, RepeaterInfo.EndRotation.Yaw);
			RandomRotation.Roll = FMath::RandRange(RepeaterInfo.StartRotation.Roll, RepeaterInfo.EndRotation.Roll);
			RandomRotation += RepeaterRotation + RepeaterInfo.RotationOffset;

			float RandomScale = FMath::RandRange(RepeaterInfo.Scale.Min, RepeaterInfo.Scale.Max);

			RepeaterTransform.SetTranslation(RepeaterLocation);
			RepeaterTransform.SetRotation(RandomRotation.Quaternion());
			RepeaterTransform.SetScale3D(FVector(1,1,1) * RandomScale);

			CreateRepeater(Owner, RepeaterInfo, RepeaterTransform, GroupIndex, RepeaterIndex);
		}
	}
		
}

// Sets default values
AVortexRepeater::AVortexRepeater()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootBillboard = CreateDefaultSubobject<UBillboardComponent>("RootBillboard");
	RootComponent = RootBillboard;

	BoundingBox = CreateDefaultSubobject<UBoxComponent>("BoundingBox");
	bIsUnbound = true;
}

// Called when the game starts or when spawned
void AVortexRepeater::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AVortexRepeater::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AVortexRepeater::ResetRepeaters()
{
	for (int iIndex = 0; iIndex < VortexRepeaterMeshData.Num(); iIndex++)
	{
		VortexRepeaterMeshData[iIndex].ResetRepeater();
	}

	VortexRepeaterMeshData.Empty();
}

void AVortexRepeater::GenerateRepeaters()
{
	ResetRepeaters();
	
	if(RepeaterData)
	{
		if(SourceSpline)
		{
			if(USplineComponent* CurrentSpline = SourceSpline->SplineComponent)
			{
				for (int iIndex = 0; iIndex < RepeaterData->VortexRepeaterInfo.Num(); iIndex++)
				{
					FVortexRepeaterInfo CurrentRepeaterInfo =  RepeaterData->VortexRepeaterInfo[iIndex];

					if(CurrentRepeaterInfo.bIsEnabled)
					{
						FVortexRepeaterMeshData NewRepeaterMeshData; 
						NewRepeaterMeshData.GenerateRepeater(this, CurrentSpline, CurrentRepeaterInfo, iIndex, BoundingBox, bIsUnbound);
						VortexRepeaterMeshData.Add(NewRepeaterMeshData);
					}
					
				}

				Modify(true);
		
			}
		
		}
	}
}

