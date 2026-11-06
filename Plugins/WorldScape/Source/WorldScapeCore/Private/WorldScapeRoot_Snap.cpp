// Copyright 2024 IOLACORP STUDIO. All Rights Reserved

#include "WorldScapeRoot.h"
#include "WorldScape_SnapStruct.h"
#include "WS_FunctionLibrary.h"
#include "Async/Async.h"
#include "Async/ParallelFor.h"
#include "Virtualization/VirtualizationTypes.h"

// ----------------------------------------------------
FWS_SnapSpawnData AWorldScapeRoot::Snap_ComputeCollection( const FTransform& BaseTransform , FRandomStream Random , const TArray<FWS_Grid_Actor_Data>& Actors , const TArray<FWS_Grid_StaticMesh_Data>& StaticMesh , const FWS_VolumeData& WSVolume )
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WS_Snap_ComputeCollection);
	TArray<FWS_Grid_Actor_SpawnData> LActor = Grid_Compute_CollectionActor( Random , BaseTransform , Actors , WSVolume);
	TArray<FWS_Grid_Static_SpawnData> LStaticMesh = Grid_Compute_CollectionStaticMesh( Random , BaseTransform , StaticMesh , WSVolume);
	return FWS_SnapSpawnData(LActor,LStaticMesh);
}
// ----------------------------------------------------
void AWorldScapeRoot::Snap_ComputeCollection_ForActor_Async( AWorldScape_SnapSpawnerActor* SnapActor )
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WS_Snap_ComputeCollection_ForActor_Async);
	if ( IsValid(SnapActor) && IsValid(this) && SnapActor->Get_DataValid() )
	{
		TWeakObjectPtr<AWorldScapeRoot> WeakRoot(this);
		TWeakObjectPtr<AWorldScape_SnapSpawnerActor> WeakSnapActor(SnapActor);
		const FTransform ActorTransform = SnapActor->GetActorTransform();
		const FWS_SnapData SnapData = SnapActor->Get_SnapData();
		const FWS_VolumeData VolumeData = GetWSVolume_ByBox( ActorTransform , SnapData.Extent );
		AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [ WeakRoot , WeakSnapActor , ActorTransform , SnapData , VolumeData ]
		{
			const int32 BaseSeed = FMath::FloorToInt(ActorTransform.GetLocation().Dot(FVector(1024,1024,1024)))*63480;
			FRandomStream Random = FRandomStream(BaseSeed);
			if ( AWorldScapeRoot* WeakRootPtr = WeakRoot.Get() )
			{
				if ( IsValid(WeakRootPtr) )
				{
					FWS_SnapSpawnData ReturnData = WeakRootPtr->Snap_ComputeCollection( ActorTransform , Random , SnapData.Actors , SnapData.StaticMesh , VolumeData );
					AsyncTask(ENamedThreads::GameThread, [ SpawnData = MoveTemp(ReturnData) , WeakSnapActor , WeakRoot ]
					{
						if ( AWorldScapeRoot* WeakRootPtrLocal = WeakRoot.Get() )
						{
							if ( IsValid(WeakRootPtrLocal) )
							{
								if ( AWorldScape_SnapSpawnerActor* SnapActor = WeakSnapActor.Get() )
								{
									if ( IsValid(SnapActor) )
									{
										SnapActor->SetSpawnData(SpawnData , WeakRootPtrLocal->SnapActor_UsePendingManager );
									}
								}
							}
						}
					});
				}
			}
		});
	}
}
// ----------------------------------------------------
FWS_VolumeData AWorldScapeRoot::GetWSVolume_ByBox( const FTransform& Center , const FVector& Extent )
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WS_Snap_GetWSVolume_ByBox);
	const FVector Min = Center.TransformPosition(Extent);
	const FVector Max = Center.TransformPosition(Extent);
	const FBox CoordBox = FBox(Min,Max);
	TArray<FHeightMapVolumeDataCopy> VolumeListVar;
	TArray<FNoiseVolumeData> NoiseVolumeListVar;
	TArray<FTerrainHoleVolumeData> TerrainHoleVolumeListVar;
	// Use Get VolumesData
	GetVolumesData(VolumeListVar, NoiseVolumeListVar, TerrainHoleVolumeListVar, CoordBox); // Get Direct from Fonction.
	TArray<FTransform> VolumeTransformArray;
	for (int32 i = 0; i < VolumeListVar.Num(); i++)
	{
		if (VolumeListVar.IsValidIndex(i))
		{
			VolumeTransformArray.Add(VolumeListVar[i].VolumeLocation);
		}
	}
	return FWS_VolumeData(NoiseVolumeListVar,VolumeListVar,TerrainHoleVolumeListVar,VolumeTransformArray);
}
// ----------------------------------------------------
void AWorldScapeRoot::Snap_QuadraticTransform( TArray<FWS_Grid_Actor_Data>& Actors , TArray<FWS_Grid_StaticMesh_Data>& StaticMesh , const FVector& A , const FVector& B , const FVector& C , const FVector& D )
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WS_Snap_QuadraticTransform);
	const float StartY = A.Y;
	const float EndY = D.Y;
	const float RangeY = EndY - StartY;

	auto ApplySplineOffset = [&](FTransform& Pos)
	{
		float T = (Pos.GetLocation().Y - StartY) / RangeY;
		T = FMath::Clamp(T, 0.0f, 1.0f);
		FVector Offset = UWS_FunctionLibrary::WS_GetSplineOffset(A, B, C, D, T);
		Pos.SetLocation(Pos.GetLocation() + FVector(Offset.X, Offset.Y, 0.0f));
	};
	for (FWS_Grid_Actor_Data& Actor : Actors)
	{
		ApplySplineOffset(Actor.Placement.Transform);
	}
	for (FWS_Grid_StaticMesh_Data& Mesh : StaticMesh)
	{
		ApplySplineOffset(Mesh.Placement.Transform);
	}
}
// ----------------------------------------------------
void AWorldScapeRoot::SnapActor_UpdatePending()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WS_Snap_UpdatePending);
	if ( !SnapActor_PendingArray.IsEmpty() )
	{
		const double StartTime = FPlatformTime::Seconds();
		int32 RemoveIndex = 0;
		for ( int32 i = 0; i < SnapActor_PendingArray.Num(); i++ )
		{
			if ( IsValid(SnapActor_PendingArray[i]) )
			{
				bool NextUpdate = true;
				while(NextUpdate)
				{
					if ( !SnapActor_PendingArray[i]->Next_Pending() )
					{
						RemoveIndex++;
						NextUpdate = false;
						break;
					}
					if ( (FPlatformTime::Seconds() - StartTime) > SnapActor_MaxTimePerTick )
					{
						if ( RemoveIndex != 0 )
						{
							SnapActor_PendingArray.RemoveAt(0,RemoveIndex);
						}
						return;
					}
				}
			}
			else
			{
				RemoveIndex++;
			}
		}
		SnapActor_PendingArray.Empty();
	}
}
// ----------------------------------------------------
void AWorldScapeRoot::SnapActor_AddActorToPending( AWorldScape_SnapSpawnerActor* SnapActor )
{
	if ( IsValid(SnapActor) )
	{
		SnapActor_PendingArray.Add(SnapActor);
	}
}
// ----------------------------------------------------
void AWorldScapeRoot::SnapActor_Clear()
{
	SnapActor_PendingArray.Empty();
}