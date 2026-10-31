// Copyright 2018-2025 Athian Games. All Rights Reserved. 

#include "VortexTunnelActor.h"

#include "Async/Async.h"
#include "Components/BillboardComponent.h"


#if WITH_EDITOR
#include "Framework/Notifications/NotificationManager.h"
#include "Editor.h"
#endif


#include "Engine/StaticMesh.h"
#include "Kismet/KismetMathLibrary.h"
#include "Misc/ScopedSlowTask.h"


#define LOCTEXT_NAMESPACE "VortexTunnel"

// Sets default values
AVortexTunnelActor::AVortexTunnelActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootBillboard = CreateDefaultSubobject<UBillboardComponent>("RootBillboard");
	RootComponent = RootBillboard;
	
	
	bIsBusy = false;

	MinSegmentScale = 1;
	MaxSegmentScale = 1;

	RelativeStartDistance = 0;
	RelativeEndDistance = 100;
	
}


// Called when the game starts or when spawned
void AVortexTunnelActor::BeginPlay()
{
	Super::BeginPlay();


	
}

// Called every frame
void AVortexTunnelActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


FVector AVortexTunnelActor::GetMeshDimensions(UStaticMesh* Mesh)
{
	if (Mesh)
	{
		FBox box = Mesh->GetBoundingBox();

		FVector D = box.Max - box.Min;
		return D;

	}

	return FVector();
}

FVector AVortexTunnelActor::GetMeshDimensionsFromScale(FVector MeshScale, FVector meshdimensions)
{
	FVector Dimensions = MeshScale * meshdimensions;
	return Dimensions;

}



#if WITH_EDITOR

void AVortexTunnelActor::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void AVortexTunnelActor::OnConstruction(const FTransform& Transform)
{

	Super::OnConstruction(Transform);
	
	SetActorScale3D(FVector(1,1,1));

	bool bRequiresRegeneration = false;
	if(VortexComponents.Num() > 0)
	{
		if(!VortexComponents[0].MeshComponent)
		{
			bRequiresRegeneration = true;
		}
	}
	
	if(bRequiresRegeneration)
	{
		FinishGeneration();
	}
	

}

#endif

USplineComponent* AVortexTunnelActor::GetCurrentSplineComponent()
{
	USplineComponent* SplineComponent = nullptr;
	if(SourceSpline)
	{
		SplineComponent = SourceSpline->SplineComponent;
	}
	return  SplineComponent;
}

void AVortexTunnelActor::DestroyAllVortex()
{
	for (int iIndex = 0; iIndex < VortexComponents.Num(); iIndex++)
	{
		VortexComponents[iIndex].DestroyVortex();
	}
}

void AVortexTunnelActor::GenerateVortexInRange(float CurrentDistance, float EnterDistance)
{
	if(USplineComponent* CurrentSpline = GetCurrentSplineComponent())
	{
		for (int iIndex = 0; iIndex < VortexComponents.Num(); iIndex++)
		{
		   VortexComponents[iIndex].CreateVortexInRange(this, VortexProperties, iIndex, CurrentSpline, CurrentDistance, EnterDistance);
		}
		
	}
}

void AVortexTunnelActor::DestroyVortexOutOfRange(float CurrentDistance, float ExitDistance)
{
	for (int iIndex = 0; iIndex < VortexComponents.Num(); iIndex++)
	{
		VortexComponents[iIndex].DestroyVortexOutOfRange(CurrentDistance, ExitDistance);
	}
		
}

void AVortexTunnelActor::UpdateScalarProperty(int32 MaterialIndex, FString PropertyName, float PropertyValue)
{
	for (int iIndex = 0; iIndex < VortexComponents.Num(); iIndex++)
	{
		VortexComponents[iIndex].UpdateScalarProperty(MaterialIndex, PropertyName, PropertyValue);
	}
}

void AVortexTunnelActor::UpdateColorProperty(int32 MaterialIndex, FString PropertyName, FLinearColor PropertyValue)
{
	for (int iIndex = 0; iIndex < VortexComponents.Num(); iIndex++)
	{
		VortexComponents[iIndex].UpdateColorProperty(MaterialIndex, PropertyName, PropertyValue);
	}
}



void AVortexTunnelActor::GenerateNextSegment(bool bAsync)
{

#if WITH_EDITOR
	
	if (VortexNotificationWeakPtr.IsValid())
	{
		if(USplineComponent* CurrentSpline = GetCurrentSplineComponent())
		{
			float SplineLength = CurrentSpline->GetSplineLength();

			float RelStart = RelativeStartDistance * SplineLength / 100;
			float RelEnd = RelativeEndDistance * SplineLength / 100;
			float relativelength = RelEnd - RelStart;
			float Completion = FMath::Max(100.0 * (static_cast<float>(DistanceCovered) / relativelength), 0.0);
			FText Progress = FText::FromString("Generating Vortex Mesh : " + FString::FromInt(FMath::FloorToInt(Completion)) + " % Completed");
			VortexNotificationWeakPtr.Pin()->SetText(Progress);
		}
	}

#endif
	

	class FAsyncRequestGeneration : public FNonAbandonableTask
	{
		AVortexTunnelActor* VortexTunnelActor;
		bool bAsyncWork;
		
	public:

		FAsyncRequestGeneration(AVortexTunnelActor* InVortexTunnelActor, bool InAsyncWork)
		{
			VortexTunnelActor = InVortexTunnelActor;
			bAsyncWork = InAsyncWork;
		
		}

		/** Performs work on thread */
		void DoWork()
		{
			if(VortexTunnelActor)
			{
				if(USplineComponent* CurrentSpline = VortexTunnelActor->GetCurrentSplineComponent())
				{

					float PreviousTimeIndex = VortexTunnelActor->CurrentSegment == 0 ? 0 : UKismetMathLibrary::MapRangeClamped(VortexTunnelActor->CurrentSegment - 1,
					0, VortexTunnelActor->NumberOfSegments - 1, 0, 100);
					float CurrentTimeIndex = UKismetMathLibrary::MapRangeClamped(VortexTunnelActor->CurrentSegment, 0, VortexTunnelActor->NumberOfSegments - 1, 0, 100);

					int32 RandomIndex = FMath::RandRange(0, VortexTunnelActor->ValidVortexData.Num() - 1);
					

					UVortexData* CurrentVortexData = VortexTunnelActor->ValidVortexData[RandomIndex];
					TArray<FVortexMeshData> ValidMeshData = CurrentVortexData->GetValidMeshData();

					int32 RandomMeshInfoIndex = FMath::RandRange(0, ValidMeshData.Num() - 1);
					FVortexMeshData CurrentMeshData = ValidMeshData[RandomMeshInfoIndex];

					float SegmentSize;
					if(CurrentMeshData.Mesh)
					{
						
						FVector MeshBounds = CurrentMeshData.Mesh->GetBounds().BoxExtent * 2 * VortexTunnelActor->VortexProperties.RadiusScale;
						
						switch (VortexTunnelActor->VortexProperties.SplineMeshAxis)
						{
						case ESplineMeshAxis::X:
							SegmentSize = MeshBounds.X;
							break;
						case ESplineMeshAxis::Y:
							SegmentSize = MeshBounds.Y;
							break;
						case ESplineMeshAxis::Z:
							SegmentSize = MeshBounds.Z;
							break;
						default: ;
							SegmentSize = MeshBounds.X;
						}

						float RandomSegmentScale = FMath::RandRange(VortexTunnelActor->MinSegmentScale, VortexTunnelActor->MaxSegmentScale);
						SegmentSize *= RandomSegmentScale;
					
						float CurrentDistance = VortexTunnelActor->DistanceCovered;
						float NextDistance = CurrentDistance + SegmentSize;
						VortexTunnelActor->DistanceCovered += SegmentSize;

						FVortexComponent* VortexComponent = new FVortexComponent();
						VortexComponent->CurrentMeshInfo = CurrentMeshData;
						VortexComponent->CurrentVortexData = CurrentVortexData;
						VortexComponent->StartDistance = CurrentDistance;
						VortexComponent->EndDistance = NextDistance;
						VortexComponent->PreviousTimeIndex = PreviousTimeIndex;
						VortexComponent->CurrentTimeIndex = CurrentTimeIndex;

						VortexTunnelActor->VortexComponents.Add(*VortexComponent);

					}
					else
					{
						SegmentSize = 100;
					}

			
					AVortexTunnelActor* CurrentVTActor = VortexTunnelActor;
					float SplineLength = CurrentSpline->GetSplineLength();
						
					bool IsAsync = bAsyncWork;
					AsyncTask(ENamedThreads::GameThread, [CurrentVTActor, SplineLength, IsAsync, SegmentSize]()
					{
						float enddistnace = CurrentVTActor->RelativeEndDistance * SplineLength / 100;
						if(CurrentVTActor->DistanceCovered + SegmentSize <= enddistnace)
						{
							CurrentVTActor->GenerateNextSegment(IsAsync);
						}
						else
						{
							CurrentVTActor->FinishGeneration();
							
						}
					});
					
				}
			}
		}

		FORCEINLINE TStatId GetStatId() const
		{
			RETURN_QUICK_DECLARE_CYCLE_STAT(FAsyncRequestGeneration, STATGROUP_ThreadPoolAsyncTasks);
		}
	};

	if(bAsync)
	{
		(new FAutoDeleteAsyncTask<FAsyncRequestGeneration>(this, bAsync))->StartBackgroundTask();
	}
	else
	{
		(new FAutoDeleteAsyncTask<FAsyncRequestGeneration>(this, bAsync))->StartSynchronousTask();
	}
	
	
}


void AVortexTunnelActor::GenerateVortex(bool bAsync)
{
#if WITH_EDITOR
	Modify();
	GEditor->BeginTransaction(NSLOCTEXT("UnrealEd", "GenerateVortex", "GenerateVortex"));
#endif


	for (int iIndex = 0; iIndex < VortexComponents.Num(); iIndex++)
	{
		if(VortexComponents[iIndex].MeshComponent)
		{
			VortexComponents[iIndex].MeshComponent->DestroyComponent();
			VortexComponents[iIndex].MeshComponent = nullptr;
		}

	}
	VortexComponents.Empty();
	
	ValidVortexData.Empty();
	CurrentSegment = 0;

	
	for (int iIndex = 0; iIndex < VortexData.Num(); iIndex++)
	{
		if(UVortexData* CurrentVortexData = VortexData[iIndex])
		{
			TArray<FVortexMeshData> ValidMeshData = CurrentVortexData->GetValidMeshData();
			if(ValidMeshData.Num() > 0)
			{
				ValidVortexData.Add(CurrentVortexData);
			}
			
		}
	}


	if (ValidVortexData.Num() > 0)
	{
				
#if WITH_EDITOR

		
		if (!VortexNotificationWeakPtr.IsValid())
		{
			FNotificationInfo ErrorNotification(LOCTEXT("VortexNotification", "Generating Vortex"));
			ErrorNotification.bFireAndForget = false;
			ErrorNotification.bUseLargeFont = false;
			VortexNotificationWeakPtr = FSlateNotificationManager::Get().AddNotification(ErrorNotification);
			VortexNotificationWeakPtr.Pin()->SetForegroundColor(FLinearColor(0.9,1,0.9,1));
		}
		else
		{
			VortexNotificationWeakPtr.Pin()->ExpireAndFadeout();
		}
#endif
				

		if(USplineComponent* CurrentSpline = GetCurrentSplineComponent())
		{
			float SplineLength =  CurrentSpline->GetSplineLength();
			DistanceCovered = RelativeStartDistance * SplineLength / 100;
			GenerateNextSegment(bAsync);
		}
		else
		{
#if WITH_EDITOR
	
			if(VortexNotificationWeakPtr.IsValid())
			{
				VortexNotificationWeakPtr.Pin()->SetCompletionState(SNotificationItem::CS_Fail);
				VortexNotificationWeakPtr.Pin()->SetText(LOCTEXT("VortexErrorNotification", "Source Spline Not Provided"));
				VortexNotificationWeakPtr.Pin()->ExpireAndFadeout();
			}
#endif
	
		}
	}
	else
	{
		
#if WITH_EDITOR

		if(VortexNotificationWeakPtr.IsValid())
		{
			VortexNotificationWeakPtr.Pin()->Fadeout();
		}
		
		FNotificationInfo ErrorNotification(LOCTEXT("VortexNotificationInvalid", "Invalid Vortex Parameters"));
		ErrorNotification.bFireAndForget = false;
		ErrorNotification.bUseLargeFont = false;
		VortexNotificationWeakPtr = FSlateNotificationManager::Get().AddNotification(ErrorNotification);
		VortexNotificationWeakPtr.Pin()->SetForegroundColor(FLinearColor(0.9,0,0,1));
		VortexNotificationWeakPtr.Pin()->SetCompletionState(SNotificationItem::CS_Fail);
		
#endif
	}

#if WITH_EDITOR  
	GEditor->EndTransaction();
#endif

}


void AVortexTunnelActor::FinishGeneration()
{
	
#if WITH_EDITOR
	
	if(VortexNotificationWeakPtr.IsValid())
	{
		VortexNotificationWeakPtr.Pin()->SetText(LOCTEXT("VortexGenerationCompletionNotification", "Generation Completed"));
		VortexNotificationWeakPtr.Pin()->ExpireAndFadeout();
	}
#endif
	

	if(USplineComponent* CurrentSpline = GetCurrentSplineComponent())
	{
		FScopedSlowTask MeshCreationTask(VortexComponents.Num(), LOCTEXT("CreatingMesh", "Creating Vortex Mesh"));
		MeshCreationTask.MakeDialog();

		for (int iIndex = 0; iIndex < VortexComponents.Num(); iIndex++)
		{
			if(MeshCreationTask.ShouldCancel())
			{
				break;
			}
			MeshCreationTask.EnterProgressFrame();

			VortexComponents[iIndex].PreviousTimeIndex = iIndex == 0 ? 0 :100.0 *  static_cast<float>(iIndex) / VortexComponents.Num();
			VortexComponents[iIndex].CurrentTimeIndex = 100.0 *  static_cast<float>(iIndex + 1) / VortexComponents.Num();
			VortexComponents[iIndex].CreateVortex(this, VortexProperties, iIndex, CurrentSpline);
			VortexComponents[iIndex].UpdateDynamicMaterial(VortexProperties, CurrentSpline, RelativeStartDistance, RelativeEndDistance);
		}
		
	}
	
	


}

void AVortexTunnelActor::UpdateVortex()
{
	if(USplineComponent* CurrentSpline = GetCurrentSplineComponent())
	{
		for (int iIndex = 0; iIndex < VortexComponents.Num(); iIndex++)
		{
			VortexComponents[iIndex].UpdateDynamicMaterial(VortexProperties, CurrentSpline, RelativeStartDistance, RelativeEndDistance);
			VortexComponents[iIndex].UpdateVortexMeshScalingAtIndex(VortexProperties);
					
		}
	}
	
}

#undef LOCTEXT_NAMESPACE6
