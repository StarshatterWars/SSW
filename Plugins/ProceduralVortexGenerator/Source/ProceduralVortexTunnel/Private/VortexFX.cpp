// Copyright 2022, Athian Games. All Rights Reserved. 


#include "VortexFX.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "VortexData.h"

#include "NiagaraFunctionLibrary.h"

void FVortexFXComponentInfo::CreateFX(AActor* Owner, int32 GroupIndex, int32 FXIndex)
{
	if(Owner && NiagaraSystem)
	{
		FString NiagaraComponentName = "Niagara_" + FString::FromInt(GroupIndex) + "_" + FString::FromInt(FXIndex);
		NiagaraComponent = NewObject<UNiagaraComponent>(Owner, *NiagaraComponentName, RF_Transactional);
		NiagaraComponent->SetAsset(NiagaraSystem);
		NiagaraComponent->SetWorldLocation(FXLocation);

		if(bAlignToSpline)
		{
			NiagaraComponent->SetWorldRotation(FXDirection.Rotation());
		}
			
		NiagaraComponent->RegisterComponent();

		NiagaraComponent->SetFloatParameter("VortexRadius", VortexRadius);
		NiagaraComponent->SetFloatParameter("SpawnRate", SpawnRate);
					
		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(NiagaraComponent, "SplineDirections", SplineDirections);
		NiagaraComponent->SetVectorParameter("VortexDirection", FXDirection);
	}
}

void FVortexFXGroup::CreateFXInRange(AActor* Owner, float CurrentDistance, float MinEnterDistance, int32 GroupIndex)
{

	if(Owner)
	{
		if(!GroupEntered && CurrentDistance + MinEnterDistance >= StartDistance)
		{
			GroupEntered = true;
			
			for (int iindex = 0; iindex < VortexFXComponentInfo.Num(); iindex++)
			{
				VortexFXComponentInfo[iindex].CreateFX(Owner, GroupIndex, iindex);
			}
		}
	}
		
}

void FVortexFXGroup::DestroyFXOutSideRange(float CurrentDistance, float MinExitDistance)
{
	if(!GroupExited && CurrentDistance - MinExitDistance >= EndDistance)
	{
		GroupExited = true;

		for (int iindex = 0; iindex < VortexFXComponentInfo.Num(); iindex++)
		{
			VortexFXComponentInfo[iindex].DestroyFX();
		}
			
	}
		
}

// Sets default values
AVortexFX::AVortexFX()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bIsConstructed = false;
	GroupLength = 1000;
	
}

#if WITH_EDITOR

void AVortexFX::OnConstruction(const FTransform& transform)
{
	

	if(!bIsConstructed)
	{
		GenerateNiagaraFX();
		bIsConstructed = true;
	}

	Super::OnConstruction(transform);
}

#endif

// Called when the game starts or when spawned
void AVortexFX::BeginPlay()
{
	ResetNiagaraFX();
	Super::BeginPlay();
	
	
}

// Called every frame
void AVortexFX::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AVortexFX::ResetNiagaraFX()
{
	for (int iIndex = 0; iIndex < VortexFXGroups.Num(); iIndex++)
	{
		for (int jIndex = 0; jIndex < VortexFXGroups[iIndex].VortexFXComponentInfo.Num(); jIndex++)
		{
			VortexFXGroups[iIndex].VortexFXComponentInfo[jIndex].DestroyFX();
			VortexFXGroups[iIndex].ResetGroup();
		}
				
	}

	Modify();
}

void AVortexFX::GenerateNiagaraFX()
{

	ResetNiagaraFX();
	VortexFXGroups.Empty();
	
	if(SourceSpline)
	{
		if(USplineComponent* CurrentSpline = SourceSpline->SplineComponent)
		{
			
			if(FXData)
			{
				float SplineLength = CurrentSpline->GetSplineLength();

				for (int iIndex = 0; iIndex < FXData->FXInfo.Num(); iIndex++)
				{
					FVortexFXInfo CurrentFXInfo = FXData->FXInfo[iIndex];

					if(CurrentFXInfo.bIsEnabled)
					{
						float StartDistance = FMath::Max<float>(0, CurrentFXInfo.StartDistance * SplineLength / 100);
					    float EndDistance = FMath::Min<float>(SplineLength, CurrentFXInfo.EndDistance * SplineLength / 100);
					
					    if(CurrentFXInfo.FXDistance.Min > 0 && CurrentFXInfo.FXDistance.Max > 0 && CurrentFXInfo.NiagaraSystems.Num() > 0)
					    {
						if(StartDistance < EndDistance)
						{
							FVortexFXGroup* NewGroup = new FVortexFXGroup();
							NewGroup->StartDistance = StartDistance;
							NewGroup->EndDistance = NewGroup->StartDistance + GroupLength;
							NewGroup->VortexFXComponentInfo.Empty();
							
							float RandomDistance = StartDistance;
							int32 jIndex = 0;

							while(RandomDistance <= EndDistance)
							{
								if(NewGroup->EndDistance < RandomDistance)
								{
									VortexFXGroups.Add(*NewGroup);
									
									NewGroup = new FVortexFXGroup();
									NewGroup->StartDistance = RandomDistance;
									NewGroup->EndDistance = NewGroup->StartDistance + GroupLength;
								}

								
								int32 RandomFXIndex = FMath::RandRange(0, CurrentFXInfo.NiagaraSystems.Num() - 1) ;

								if(UNiagaraSystem* CurrentNiagaraSystem = CurrentFXInfo.NiagaraSystems[RandomFXIndex])
								{
									int32 RandomSpawmProbability = FMath::RandRange(0,100);
									if(RandomSpawmProbability < CurrentFXInfo.SpawnProbability)
									{
										FVector FXLocation = CurrentSpline->GetLocationAtDistanceAlongSpline(RandomDistance, ESplineCoordinateSpace::World);
										FVector FXDirection = CurrentSpline->GetDirectionAtDistanceAlongSpline(RandomDistance, ESplineCoordinateSpace::World);
										
										float SegmentLength = FMath::RandRange(CurrentFXInfo.FXLength.Min, CurrentFXInfo.FXLength.Max) ;
										float FXDensity = FMath::RandRange(CurrentFXInfo.FXDensity.Min, CurrentFXInfo.FXDensity.Max) ;

										float SpawnRate = SegmentLength * FXDensity / 100;
										SpawnRate = FMath::Min<float>(SpawnRate, 10000.0);

										TArray<FVector> SplineDirections;

										int32 TotalNum = 10;
										for (int kIndex = 0; kIndex < TotalNum; kIndex++)
										{
											float NewDistance = RandomDistance + kIndex  * SegmentLength/TotalNum;
											FVector NewDirection = CurrentSpline->GetDirectionAtDistanceAlongSpline(NewDistance, ESplineCoordinateSpace::Local);
											SplineDirections.Add(NewDirection);
										}
									
										FVortexFXComponentInfo NewFXInfo;
										NewFXInfo.VortexRadius = FMath::RandRange(CurrentFXInfo.VortexRadius.Min, CurrentFXInfo.VortexRadius.Max);
										NewFXInfo.SpawnRate = SpawnRate;
										NewFXInfo.NiagaraSystem = CurrentNiagaraSystem;
										NewFXInfo.FXLocation = FXLocation;
										NewFXInfo.FXDirection = FXDirection;
										NewFXInfo.SplineDirections = SplineDirections;

										NewFXInfo.bAlignToSpline = CurrentFXInfo.bAlignToSpline;

										NewGroup->VortexFXComponentInfo.Add(NewFXInfo);

								
										jIndex++;
									}
								}

								RandomDistance += FMath::RandRange(CurrentFXInfo.FXDistance.Min, CurrentFXInfo.FXDistance.Max) ;
							}
							
						}
					}		
					}
					
				}

					
			}


				
		}
		
	}

	Modify();
}


void AVortexFX::DestroyNiagaraFXByGroup(float CurrentDistance, float ExitDistance)
{
	for (int iIndex = 0; iIndex < VortexFXGroups.Num(); iIndex++)
	{
		VortexFXGroups[iIndex].DestroyFXOutSideRange(CurrentDistance, ExitDistance);
	}
}

void AVortexFX::GenerateNiagaraFXByGroup(float CurrentDistance, float EnterDistance)
{
	for (int iIndex = 0; iIndex < VortexFXGroups.Num(); iIndex++)
	{
		VortexFXGroups[iIndex].CreateFXInRange(this, CurrentDistance, EnterDistance, iIndex);
	}
	
}



