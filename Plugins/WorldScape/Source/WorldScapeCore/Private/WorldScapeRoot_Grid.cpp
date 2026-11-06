// Copyright 2021 IOLACORP STUDIO. All Rights Reserved


#include "Constraint.h"
#include "PoissonDiscSampling.h"

#include "WorldScapeRoot.h"
#include "WorldScape_GridCellData.h"
#include "WorldScape_GridStruct.h"
#include "WorldScape_GridDataAsset.h"
#include "WorldScape_GridSMProxy.h"
#include "WorldScape_GridSerializedData.h"
#include "Algo/RandomShuffle.h"
#include "Async/Async.h"
#include "Async/ParallelFor.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Serialization/ObjectReader.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"


void AWorldScapeRoot::Grid_CheckStaticData()
{
	if ( Grid_CellData )
	{
		if ( Grid_CellData->Grid_StaticData.Num() == ((Grid_Data_Size * Grid_Data_Size)*6) )
		{
			Grid_StaticData_IsValid = true;
		}
		else
		{
			Grid_StaticData_IsValid = false;
		}
	}
}
// --------------------------------------
void AWorldScapeRoot::Grid_Update( const TArray<FVector>& Locations , const float& DeltaTime )
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WS_Grid_Update);
	TArray<FWS_Grid> Grids;
	TMap<int64, FWS_Grid> GridMap;
	for ( const auto& Location : Locations )
	{
		if ( (GetActorLocation()-Location).SquaredLength() < FMath::Square(PlanetScale+Grid_DistanceToLoad) )
		{
			FWS_Grid Local = FWS_Grid( GetActorLocation() , Location , Grid_Size , PlanetScale );
			Grids.Add( Local );
			if ( GetWorld()->GetNetMode() == NM_DedicatedServer )
			{
				if ( !Grid_SkipAdjacentCellOnDedicated )
				{
					Grids.Append(Local.Get_Adjacent_Cells(GetActorLocation(),PlanetScale));
				}
			}
			else
			{
				Grids.Append(Local.Get_Adjacent_Cells(GetActorLocation(),PlanetScale));
			}
		}
	}
	for ( const auto& Grid : Grids )
	{
		GridMap.Add(Grid.Hash,Grid);
	}
	// load Storage
	Grid_UpdateStorage( Grids );
	// Compute load / unload
	TArray<FWS_Grid> ToLoad;
	TArray<int64> ToUnload;
	
	for ( auto& Cell : Loaded_Cell )
	{
		if ( GridMap.Contains(Cell.Key) )
		{
			GridMap.Remove(Cell.Key);	
		}
		else
		{
			ToUnload.Add(Cell.Key);
		}
	}
	for ( auto& Cell : GridMap )
	{
		ToLoad.Add(Cell.Value);
	}
	// Set Load Unload
	for ( const auto& Cell : ToLoad )
	{
		WS_Grid_LoadCell(Cell);
	}
	for ( const auto& Cell : ToUnload )
	{
		WS_Grid_UnLoadCell( Cell );
	}
	Grid_UpdateProxyCounter += DeltaTime;
	if ( Grid_UpdateProxyCounter >= Grid_ProxyUpdateTime )
	{
		Grid_UpdateProxyCounter = 0.0f;
		Grid_Update_Proxy( Locations );
		Grid_PendingSpawn_RemoveInvalid();
	}
	// Auto reload thread
	if ( !Grid_ThreadPool.IsEmpty() )
	{
		for ( int32 i = 0; i < Grid_ThreadPool.Num(); i++ )
		{
			if ( Grid_ThreadPool[i] )
			{
				if ( !ArrayQueue_AsyncCompute[i].IsEmpty() && Grid_ThreadPool[i]->IsDone() && ThreadValidation[i] )
				{
					Grid_ThreadValidation = true;
					Grid_ThreadPool[i]->StartBackgroundTask( GThreadPool, EQueuedWorkPriority::Highest );
				}
				else if ( Grid_ThreadPool[i]->IsDone() && !ThreadValidation[i] )
				{
					ThreadValidation[i] = true;
				}
			}
		}
	}
	
	// Check timeout
	if ( !Grid_ThreadPool.IsEmpty() )
	{
		if ( Grid_ThreadTimeout )
		{
			Grid_TimeCounter += DeltaTime;
			if ( Grid_TimeCounter >= Grid_Timeout )
			{
				for ( int32 i = 0 ; i < Grid_ThreadPool.Num() ; i++ )
				{
					if ( !ArrayQueue_AsyncCompute[i].IsEmpty() )
					{
						ThreadValidation[i] = false;
						//Grid_ThreadPool[i]->EnsureCompletion();
						//delete Grid_ThreadPool[i];
						while(!ArrayQueue_AsyncCompute[i].IsEmpty())
						{
							FWS_Grid_Queue Local;
							if (ArrayQueue_AsyncCompute[i].Dequeue(Local))
							{
								Grid_Thread_AddPending( Local );
							}
						}
						
						//Grid_ThreadPool.RemoveAt(i);
						//ArrayQueue_AsyncCompute.RemoveAt(i);
						//ThreadValidation.RemoveAt(i);
						
						/*
						ThreadValidation.AddDefaulted();
						ArrayQueue_AsyncCompute.AddDefaulted();
						FAsyncTask<GridGenerationThread>* Thread = new FAsyncTask<GridGenerationThread>(this,ArrayQueue_AsyncCompute.Last(),ThreadValidation.Last(),Queue_Spawn);
						Grid_ThreadPool.Add(Thread);
						UE_LOG(LogTemp, Warning, TEXT("Error , Thread is reloaded"));
						*/
						UE_LOG(LogTemp, Warning, TEXT("Error , Thread is reloaded"));
					}
				}
				Grid_TimeCounter = 0.0f;
			}
		}
	}
	
	if ( Loaded_Cell.IsEmpty() && Grid_DestroyAllOnOutDistance )
	{
		Grid_DestroyAllCell();
	}
	if ( Loaded_Cell.IsEmpty() )
	{
		Pending_SpawnCell.Empty();
		Queue_AsyncCompute.Empty();
		Queue_Spawn.Empty();
		Grid_ThreadValidation = false;
		Grid_Thread_StopThreadAndClearQueue();
	}
}
// --------------------------------------
void AWorldScapeRoot::WS_Grid_LoadCell( const FWS_Grid& Cell )
{
	if ( Pending_Unload.Contains(Cell.Hash) )
	{
		Pending_Unload.Remove(Cell.Hash);
	}
	if ( !Loaded_Cell.Contains(Cell.Hash) )
	{
		Loaded_Cell.Add(Cell.Hash,FWS_Grid_Container(Cell));
		WS_Grid_Spawn_Multi( Cell ); // New Version
		Grid_CheckVolumeDataToLoad( Cell );
		WS_Grid_LoadCell_Delegate.Broadcast( Cell );
		if ( Grid_Debug )
		{
			UE_LOG(LogTemp, Log, TEXT("WS Grid : Start Load Cell : %d") , Cell.FlatIndex );
		}
	}
}
// --------------------------------------
void AWorldScapeRoot::WS_Grid_UnLoadCell( const int64& CellHash )
{
	if ( FWS_Grid_Container* Data = Loaded_Cell.Find(CellHash) )
	{
		if ( Grid_UsePendingDestroy )
		{
			WS_Grid_UnLoadCell_Delegate.Broadcast((*Data).Coord);
			Pending_DestroyCell.Add(MoveTemp(*Data));
			Loaded_Cell.Remove(CellHash);
			if ( Grid_Debug )
			{
				UE_LOG(LogTemp, Log, TEXT("WS Grid : Unload Cell "));
			}
		}
		else
		{
			WS_Grid_DestroyCell( CellHash );
		}
		if ( Pending_Unload.Contains(CellHash) )
		{
			Pending_Unload.Remove(CellHash);
		}
	}
	else if ( Pending_Unload.Contains(CellHash) )
	{
		Pending_Unload.Remove(CellHash);
	}
}
// --------------------------------------
// Update Pending
void AWorldScapeRoot::Grid_Pending_Update( const float DeltaTime )
{
	if ( DeltaTime >= Grid_SkipLimiteInterval && Grid_EnabledSkipLimite )
	{
		return;
	}
	if ( Grid_UsePendingSpawn )
	{
		Grid_PendingSpawn_Update( DeltaTime );
		if ( Grid_PendingWork )
		{
			Grid_PendingWork = false;
			WS_Grid_EndPendingLoad.Broadcast();
		}
	}
	if ( Grid_UsePendingDestroy )
	{
		Grid_PendingDestroy_Update( DeltaTime );
	}
	if ( Grid_UseThread )
	{
		TempQueueUpdate( DeltaTime );
		/*
		if ( Queue_AsyncCompute.IsEmpty() && !Grid_ThreadPool.IsEmpty() )
		{
			//Grid_Thread_StopThreadAndClearQueue();
		}
		*/
	}
}
// --------------------------------------
void AWorldScapeRoot::Grid_PendingSpawn_Update( const float DeltaTime )
{
	if ( !Pending_SpawnCell.IsEmpty() || !Queue_Spawn.IsEmpty() )
	{
		if ( !Grid_PendingWork )
		{
			Grid_PendingWork = true;
			WS_Grid_StartPendingLoad.Broadcast();
		}
		if ( UWorld* World = GetWorld() )
		{
			const float AutoScale = Grid_AutoScalingPending ? FMath::Max(FMath::GetMappedRangeValueUnclamped(FVector2D(Grid_BaseInterval,Grid_BaseInterval*2),FVector2D(1.0,0.5),DeltaTime),0.5) : 1.0;
			const int32 MaxSpawn = (World->GetNetMode() == NM_DedicatedServer) ? Grid_MaxSpawnPerSecond_OnDedicated * DeltaTime * AutoScale : Grid_MaxSpawnPerSecond * DeltaTime * AutoScale;
			if ( Grid_UseThread )
			{
				if ( !Queue_Spawn.IsEmpty() )
				{
					const int32 LoopSpawn = FMath::Max(MaxSpawn,1);
					for ( int32 i = 0; i < LoopSpawn; i++ )
					{
						FWS_Grid_SpawnPending Local;
						if ( Queue_Spawn.Dequeue(Local) )
						{
							if ( Grid_CheckSpawnBySeed )
							{
								if ( FWS_Grid_Container* LookupData = Loaded_Cell.Find(Local.GridCoord.Hash) )
								{
									if ( (*LookupData).InstanceSeed.Contains(Local.Seed) )
									{
										//UE_LOG(LogTemp, Warning, TEXT("WS Grid : Error , Seed already found , Spawn is cancelled : %d"), Local.Seed);
									}
									else
									{
										(*LookupData).InstanceSeed.Add(Local.Seed);
										Grid_Make_Instance(Local.GridCoord,Local.InstanceTransform,Local.VizData,Local.Actors,Local.StaticMesh,Local.IsFoliage);
									}
								}
								else
								{
									if ( Grid_Debug ) {UE_LOG(LogTemp, Warning, TEXT("WS Grid : Error , Cell not Found"));}
								}
							}
							else
							{
								Grid_Make_Instance(Local.GridCoord,Local.InstanceTransform,Local.VizData,Local.Actors,Local.StaticMesh,Local.IsFoliage);
							}
						}
					}
					Grid_TimeCounter = 0.0f;
				}
			}
			else
			{
				const int32 LoopSpawn = FMath::Max(FMath::Min(MaxSpawn,Pending_SpawnCell.Num()),1);
				for ( int32 i = 0; i < LoopSpawn; i++ )
				{
					//Grid_Make_Instance(Pending_SpawnCell[i].GridCoord,Pending_SpawnCell[i].InstanceTransform,Pending_SpawnCell[i].VizData,Pending_SpawnCell[i].Actors,Pending_SpawnCell[i].StaticMesh,Pending_SpawnCell[i].IsFoliage);
					if ( Grid_CheckSpawnBySeed )
					{
						if ( FWS_Grid_Container* LookupData = Loaded_Cell.Find(Pending_SpawnCell[i].GridCoord.Hash) )
						{
							if ( (*LookupData).InstanceSeed.Contains(Pending_SpawnCell[i].Seed) )
							{
								//UE_LOG(LogTemp, Warning, TEXT("WS Grid : Error , Seed already found , Spawn is cancelled : %d"), Pending_SpawnCell[i].Seed);
							}
							else
							{
								(*LookupData).InstanceSeed.Add(Pending_SpawnCell[i].Seed);
								Grid_Make_Instance(Pending_SpawnCell[i].GridCoord,Pending_SpawnCell[i].InstanceTransform,Pending_SpawnCell[i].VizData,Pending_SpawnCell[i].Actors,Pending_SpawnCell[i].StaticMesh,Pending_SpawnCell[i].IsFoliage);
							}
						}
						else
						{
							if ( Grid_Debug ) {UE_LOG(LogTemp, Warning, TEXT("WS Grid : Error , Cell not Found"));}
						}
					}
					else
					{
						Grid_Make_Instance(Pending_SpawnCell[i].GridCoord,Pending_SpawnCell[i].InstanceTransform,Pending_SpawnCell[i].VizData,Pending_SpawnCell[i].Actors,Pending_SpawnCell[i].StaticMesh,Pending_SpawnCell[i].IsFoliage);
					}
				}
				Pending_SpawnCell.RemoveAt(0,LoopSpawn);
			}
		}
	}
}
// --------------------------------------
void AWorldScapeRoot::Grid_PendingDestroy_Update( const float DeltaTime )
{
	if ( !Pending_DestroyCell.IsEmpty() )
	{
		int32 Count = 0;
		TArray<int32> PendingToRemove;
		bool OnOver = false;
		
		if ( UWorld* World = GetWorld() )
		{
			const float AutoScale = Grid_AutoScalingPending ? FMath::Max(FMath::GetMappedRangeValueUnclamped(FVector2D(Grid_BaseInterval,Grid_BaseInterval*2),FVector2D(1.0,0.5),DeltaTime),0.5) : 1.0;
			int32 MaxDestroy = (World->GetNetMode() == NM_DedicatedServer) ? Grid_MaxDestroyPerSecond_OnDedicated * DeltaTime * AutoScale : Grid_MaxDestroyPerSecond * DeltaTime * AutoScale;
			MaxDestroy = FMath::Max(MaxDestroy,1);
			for ( int32 i = 0; i < Pending_DestroyCell.Num(); i++ )
			{
				int32 InternalCount = 0;
				for ( int32 y = 0 ; y < Pending_DestroyCell[i].Instances.Num() ; y++ )
				{
					InternalCount++;
					Grid_DestroyCellInstance( Pending_DestroyCell[i].Instances[y] );
					Count++;
					if ( Count >= MaxDestroy )
					{
						OnOver = true;
						if ( Pending_DestroyCell[i].Instances.Num() > InternalCount )
						{
							Pending_DestroyCell[i].Instances.RemoveAt(0,InternalCount);
							return;
						}
						else
						{
							Pending_DestroyCell.RemoveAt(i);
							return;
						}
					}
				}
				PendingToRemove.Add(i);
			}
			Pending_DestroyCell.RemoveAt(0,PendingToRemove.Num());
		}
	}
}
// --------------------------------------
void AWorldScapeRoot::Grid_PendingSpawn_RemoveInvalid()
{
	if ( !Pending_SpawnCell.IsEmpty() )
	{
		Pending_SpawnCell.RemoveAll([this](const auto& Data)
	   {
		   return !Loaded_Cell.Contains(Data.GridCoord.Hash); // Condition pour les éléments invalides
	   });
	}
}
// --------------------------------------
void AWorldScapeRoot::Grid_DestroyAllCell()
{
	if ( !Loaded_Cell.IsEmpty() )
	{
		for ( auto& CellData : Loaded_Cell )
		{
			WS_Grid_DestroyCell( CellData.Key , false);
		}
		for ( auto& CellData : Pending_DestroyCell )
		{
			for ( auto& InstanceData : CellData.Instances )
			{
				Grid_DestroyCellInstance( InstanceData );
			}
		}
	}
	Loaded_Cell.Empty();
	Pending_DestroyCell.Empty();
	Pending_SpawnCell.Empty();
}
// --------------------------------------
void AWorldScapeRoot::Grid_DestroyCellInstance( const FWS_Grid_Instance& Instance )
{
	for ( auto& ActorInst : Instance.Actors_Instance )
	{
		if ( IsValid(ActorInst) )
		{
			ActorInst->UnregisterAllComponents();
			ActorInst->Destroy();
		}
	}
	if ( IsValid(Instance.SM_Proxy) )
	{
		if ( Instance.SM_Proxy->IsFoliage )
		{
			Instance.SM_Proxy->Clear_StaticMeshList();
		}
		Instance.SM_Proxy->UnregisterAllComponents();
		Instance.SM_Proxy->Destroy();
	}
}
// --------------------------------------
void AWorldScapeRoot::WS_Grid_DestroyCell( const int64& CellHash , bool AutoDestroy )
{
	if ( FWS_Grid_Container* Data = Loaded_Cell.Find(CellHash) )
	{
		for ( auto& InstanceData : (*Data).Instances )
		{
			Grid_DestroyCellInstance( InstanceData );
		}
	}
	if ( AutoDestroy )
	{
		Loaded_Cell.Remove(CellHash);
	}
	if ( Grid_Debug )
	{
		UE_LOG(LogTemp, Log, TEXT("WS Grid : Destroy Cell : %d") , CellHash );
	}
}
// --------------------------------------
void AWorldScapeRoot::WS_Grid_Spawn_Multi( const FWS_Grid& GridCoord )
{
	//const double SpawnSize = (UE_DOUBLE_PI*2.0*(PlanetScale*0.5) * 0.25) / Grid_Size;
	const double SpawnSize = GridCoord.SizeData.CircleInternalSize*Grid_Size_Scale;
	const int32 BaseSeed = FMath::FloorToInt(GridCoord.CellCenterLocation.Dot(FVector(1024,1024,1024)))*63480 + GridCoord.FlatIndex;
	
	float SpawnScale = Grid_BaseSpawnScale;
	TArray<UWorldScape_GridDataAsset*> GridDataAssets = Grid_BaseSpawnData;

	
	// Compute Cell Data
	if ( Grid_HaveStaticData && Grid_StaticData_IsValid && Grid_CellData )
	{
		const FWS_Grid DataGrid = FWS_Grid(GetActorLocation(),GridCoord.CellCenterLocation,Grid_Data_Size,PlanetScale);

		if ( Grid_CellData->Grid_StaticData.IsValidIndex(DataGrid.FlatIndex) )
		{
			if ( Grid_CellData->Grid_StaticData[DataGrid.FlatIndex].OverrideDataAsset )
			{
				GridDataAssets.Empty();
				for ( auto& Data : Grid_CellData->Grid_StaticData[DataGrid.FlatIndex].GridDataAsset )
				{
					if ( Data )
					{
						GridDataAssets.Add(Data);
					}
				}
			}
			else
			{
				if ( Grid_CellData->Grid_StaticData[DataGrid.FlatIndex].AdditionalDataAsset )
				{
					for ( auto& Data : Grid_CellData->Grid_StaticData[DataGrid.FlatIndex].GridDataAsset )
					{
						if ( Data )
						{
							GridDataAssets.Add(Data);
						}
					}
				}
			}
			SpawnScale = Grid_CellData->Grid_StaticData[DataGrid.FlatIndex].SpawnScale;
		}
	}
	
	
	SpawnScale *= Grid_GlobalSpawnScale;
	if ( GridDataAssets.IsEmpty() )
	{
		return;
	}
	if ( SpawnScale > 0.0f && !GridDataAssets.IsEmpty() )
	{
		// Make Zone Data
		TArray<FWS_Grid_InvalidZone> ZoneData = Grid_MakeInvalidZoneData( GridCoord.CellCenterLocation , SpawnSize , Grid_InvalidVolume );
		const FWS_VolumeData WSVolume = Grid_GetWSVolume( GridCoord );
		
		for ( int32 X = 0; X < GridDataAssets.Num(); X++ )
		{
			if ( !GridDataAssets[X] )
			{
				continue;
			}
			if ( !GridDataAssets[X]->Enabled )
			{
				continue;
			}
			if ( UWorld* World = GetWorld() )
			{
				if ( (World->GetNetMode() == NM_DedicatedServer || Grid_SimulateDedicated) && !GridDataAssets[X]->SpawnableOnDedicated )
				{
					if ( Grid_Debug )
					{
						UE_LOG(LogTemp, Log, TEXT("WS Grid : Not Spawnable OnDedicated : %d") , GridCoord.FlatIndex );
					}
					continue;
				}
				if ( Grid_SkipStaticMeshOnDedicated && (World->GetNetMode() == NM_DedicatedServer || Grid_SimulateDedicated) )
				{
					if ( GridDataAssets[X]->SpawnType == StaticMesh || GridDataAssets[X]->SpawnType == CollectionStaticMesh || GridDataAssets[X]->SpawnType == Foliage )
					{
						if ( Grid_Debug )
						{
							UE_LOG(LogTemp, Log, TEXT("WS Grid : Skip OnDedicated : %d") , GridCoord.FlatIndex );
						}
						continue;
					}
				}
			}
			int32 SeedInstance = BaseSeed + (X*X*X) + GridDataAssets[X]->SeedOffset ;
			if ( Grid_Debug )
			{
				UE_LOG(LogTemp, Log, TEXT("WS Grid : Start Load Cell Instance : %d : %d") , GridCoord.FlatIndex , SeedInstance );
			}
			if ( FWS_Grid_Container* Cell = Loaded_Cell.Find(GridCoord.Hash) )
			{
				(*Cell).OnPendingLoad += 1;
				const int32 TestSpawn = GridDataAssets[X]->Max_TestToSpawn*SpawnScale;
				const int32 MaxSpawn = GridDataAssets[X]->Max_Spawn*SpawnScale;

				if ( Grid_UseThread )
				{
					switch ( GridDataAssets[X]->SpawnType )
				{
				case :: Actor:
					{
						const FWS_Grid_SpawnConf SpawnConf = FWS_Grid_SpawnConf(GridDataAssets[X]->SamplingType,SeedInstance,GridCoord,GridDataAssets[X]->Collection_ProxyData,GridDataAssets[X]->SpawnParams,GridDataAssets[X]->Z_Offset,TestSpawn,MaxSpawn,ZoneData,SpawnSize);
						FWS_Grid_Queue LocalData = FWS_Grid_Queue(GridDataAssets[X]->SpawnType,SpawnConf);
						LocalData.Add_Actor(GridDataAssets[X]->ActorsDatas);
						LocalData.SetWS_Volume(WSVolume);
						Grid_Thread_AddPending(LocalData);
						break;
					}
				case :: StaticMesh:
					{
						const FWS_Grid_SpawnConf SpawnConf = FWS_Grid_SpawnConf(GridDataAssets[X]->SamplingType,SeedInstance,GridCoord,GridDataAssets[X]->Collection_ProxyData,GridDataAssets[X]->SpawnParams,GridDataAssets[X]->Z_Offset,TestSpawn,MaxSpawn,ZoneData,SpawnSize);
						FWS_Grid_Queue LocalData = FWS_Grid_Queue(GridDataAssets[X]->SpawnType,SpawnConf);
						LocalData.Add_StaticMesh(GridDataAssets[X]->StaticMeshDatas);
						LocalData.SetWS_Volume(WSVolume);
						Grid_Thread_AddPending(LocalData);
						break;
					}
				case :: Collection:
					{
						const FWS_Grid_SpawnConf SpawnConf = FWS_Grid_SpawnConf(GridDataAssets[X]->SamplingType,SeedInstance,GridCoord,GridDataAssets[X]->Collection_ProxyData,GridDataAssets[X]->SpawnParams,GridDataAssets[X]->Z_Offset,TestSpawn,MaxSpawn,ZoneData,SpawnSize);
						FWS_Grid_Queue LocalData = FWS_Grid_Queue(GridDataAssets[X]->SpawnType,SpawnConf);
						LocalData.Add_CollectionActor(GridDataAssets[X]->Collection_ActorsDatas);
						if ( !(Grid_SkipStaticMeshOnDedicated && GetWorld()->GetNetMode() == NM_DedicatedServer) )
						{
							LocalData.Add_StaticMesh(GridDataAssets[X]->Collection_StaticMeshDatas);
						}
						LocalData.SetWS_Volume(WSVolume);
						Grid_Thread_AddPending(LocalData);
						break;
					}
				case :: CollectionStaticMesh:
					{
						const FWS_Grid_SpawnConf SpawnConf = FWS_Grid_SpawnConf(GridDataAssets[X]->SamplingType,SeedInstance,GridCoord,GridDataAssets[X]->Collection_ProxyData,GridDataAssets[X]->SpawnParams,GridDataAssets[X]->Z_Offset,TestSpawn,MaxSpawn,ZoneData,SpawnSize);
						FWS_Grid_Queue LocalData = FWS_Grid_Queue(GridDataAssets[X]->SpawnType,SpawnConf);
						LocalData.Add_StaticMesh(GridDataAssets[X]->Collection_StaticMeshDatas);
						LocalData.SetWS_Volume(WSVolume);
						Grid_Thread_AddPending(LocalData);
						break;
					}
				case :: Foliage:
					{
						const FWS_Grid_SpawnConf SpawnConf = FWS_Grid_SpawnConf(GridDataAssets[X]->SamplingType,SeedInstance,GridCoord,GridDataAssets[X]->Collection_ProxyData,GridDataAssets[X]->SpawnParams,GridDataAssets[X]->Z_Offset,TestSpawn,MaxSpawn,ZoneData,SpawnSize);
						FWS_Grid_Queue LocalData = FWS_Grid_Queue(GridDataAssets[X]->SpawnType,SpawnConf);
						LocalData.Add_StaticMesh(GridDataAssets[X]->StaticMeshDatas);
						LocalData.SetFoliageSectorSize(GridDataAssets[X]->FoliageSectorSize);
						LocalData.SetWS_Volume(WSVolume);
						Grid_Thread_AddPending(LocalData);
						break;
					}
				case :: DataAsset:
					{
						if ( !GridDataAssets[X]->Spawners.IsEmpty() )
						{
							for ( const auto& LocalData : GridDataAssets[X]->Spawners )
							{
								if ( LocalData.GridDataAsset->Enabled && ( LocalData.GridDataAsset->SpawnType == EWS_Grid_SpawnType::Actor || LocalData.GridDataAsset->SpawnType == EWS_Grid_SpawnType::StaticMesh || LocalData.GridDataAsset->SpawnType == EWS_Grid_SpawnType::Collection || LocalData.GridDataAsset->SpawnType == EWS_Grid_SpawnType::CollectionStaticMesh ) )
								{
									if ( float LocalScale = SpawnScale * LocalData.SpawnScale > 0.0f )
									{
										(*Cell).OnPendingLoad += 1;
										int32 LocalSeed = SeedInstance + LocalData.GridDataAsset->SeedOffset;
										LocalSeed += 1;
										const int32 LocalTestSpawn = LocalData.GridDataAsset->Max_TestToSpawn*SpawnScale;
										const int32 LocalMaxSpawn = LocalData.GridDataAsset->Max_Spawn*SpawnScale;
										const FWS_Grid_SpawnConf SpawnConf = FWS_Grid_SpawnConf(LocalData.GridDataAsset->SamplingType,LocalSeed,GridCoord,LocalData.GridDataAsset->Collection_ProxyData,LocalData.GridDataAsset->SpawnParams,LocalData.GridDataAsset->Z_Offset,LocalTestSpawn,LocalMaxSpawn,ZoneData,SpawnSize);
										WS_Grid_DirectSpawnDataAsset( SpawnConf , LocalData.GridDataAsset , WSVolume);
									}
								}
							}
						}
						break;
					}
				case :: OnlyData:
					{
						break;
					}
				}
				}
				else
				{
					switch ( GridDataAssets[X]->SpawnType )
				{
				case :: Actor:
					{
						const FWS_Grid_SpawnConf SpawnConf = FWS_Grid_SpawnConf(GridDataAssets[X]->SamplingType,SeedInstance,GridCoord,GridDataAssets[X]->Collection_ProxyData,GridDataAssets[X]->SpawnParams,GridDataAssets[X]->Z_Offset,TestSpawn,MaxSpawn,ZoneData,SpawnSize);
						Grid_ComputeSpawn_Actor( SpawnConf , GridDataAssets[X]->ActorsDatas ,WSVolume);
						break;
					}
				case :: StaticMesh:
					{
						const FWS_Grid_SpawnConf SpawnConf = FWS_Grid_SpawnConf(GridDataAssets[X]->SamplingType,SeedInstance,GridCoord,GridDataAssets[X]->Collection_ProxyData,GridDataAssets[X]->SpawnParams,GridDataAssets[X]->Z_Offset,TestSpawn,MaxSpawn,ZoneData,SpawnSize);
						Grid_ComputeSpawn_StaticMesh( SpawnConf , GridDataAssets[X]->StaticMeshDatas ,WSVolume);
						break;
					}
				case :: Collection:
					{
						const FWS_Grid_SpawnConf SpawnConf = FWS_Grid_SpawnConf(GridDataAssets[X]->SamplingType,SeedInstance,GridCoord,GridDataAssets[X]->Collection_ProxyData,GridDataAssets[X]->SpawnParams,GridDataAssets[X]->Z_Offset,TestSpawn,MaxSpawn,ZoneData,SpawnSize);
						Grid_ComputeSpawn_Collection( SpawnConf , GridDataAssets[X]->Collection_ActorsDatas , GridDataAssets[X]->Collection_StaticMeshDatas ,WSVolume);
						break;
					}
				case :: CollectionStaticMesh:
					{
						const FWS_Grid_SpawnConf SpawnConf = FWS_Grid_SpawnConf(GridDataAssets[X]->SamplingType,SeedInstance,GridCoord,GridDataAssets[X]->Collection_ProxyData,GridDataAssets[X]->SpawnParams,GridDataAssets[X]->Z_Offset,TestSpawn,MaxSpawn,ZoneData,SpawnSize);
						Grid_ComputeSpawn_CollectionStaticMesh( SpawnConf , GridDataAssets[X]->Collection_StaticMeshDatas ,WSVolume);
						break;
					}
				case :: Foliage:
					{
						const FWS_Grid_SpawnConf SpawnConf = FWS_Grid_SpawnConf(GridDataAssets[X]->SamplingType,SeedInstance,GridCoord,GridDataAssets[X]->Collection_ProxyData,GridDataAssets[X]->SpawnParams,GridDataAssets[X]->Z_Offset,TestSpawn,MaxSpawn,ZoneData,SpawnSize);
						Grid_ComputeSpawn_Foliage( SpawnConf  , GridDataAssets[X]->StaticMeshDatas , GridDataAssets[X]->FoliageSectorSize ,WSVolume);
						break;
					}
				case :: DataAsset:
					{
						if ( !GridDataAssets[X]->Spawners.IsEmpty() )
						{
							for ( const auto& LocalData : GridDataAssets[X]->Spawners )
							{
								if ( LocalData.GridDataAsset->Enabled && ( LocalData.GridDataAsset->SpawnType == EWS_Grid_SpawnType::Actor || LocalData.GridDataAsset->SpawnType == EWS_Grid_SpawnType::StaticMesh || LocalData.GridDataAsset->SpawnType == EWS_Grid_SpawnType::Collection || LocalData.GridDataAsset->SpawnType == EWS_Grid_SpawnType::CollectionStaticMesh ) )
								{
									if ( float LocalScale = SpawnScale * LocalData.SpawnScale > 0.0f )
									{
										(*Cell).OnPendingLoad += 1;
										int32 LocalSeed = SeedInstance + LocalData.GridDataAsset->SeedOffset;
										LocalSeed += 1;
										const int32 LocalTestSpawn = LocalData.GridDataAsset->Max_TestToSpawn*SpawnScale;
										const int32 LocalMaxSpawn = LocalData.GridDataAsset->Max_Spawn*SpawnScale;
										const FWS_Grid_SpawnConf SpawnConf = FWS_Grid_SpawnConf(LocalData.GridDataAsset->SamplingType,LocalSeed,GridCoord,LocalData.GridDataAsset->Collection_ProxyData,LocalData.GridDataAsset->SpawnParams,LocalData.GridDataAsset->Z_Offset,LocalTestSpawn,LocalMaxSpawn,ZoneData,SpawnSize);
										WS_Grid_DirectSpawnDataAsset( SpawnConf , LocalData.GridDataAsset , WSVolume );
									}
								}
							}
						}
						break;
					}
				case :: OnlyData:
					{
						break;
					}
				}
				}
			}
		}
		Grid_Thread_CheckAndMake();
	}
}
// --------------------------------------
void AWorldScapeRoot::WS_Grid_DirectSpawnDataAsset( const FWS_Grid_SpawnConf& SpawnConf , const UWorldScape_GridDataAsset* DataAsset , const FWS_VolumeData& WSVolume)
{
	if ( Grid_UseThread )
	{
		switch ( DataAsset->SpawnType )
		{
		case :: Actor:
			{
				FWS_Grid_Queue LocalData = FWS_Grid_Queue(DataAsset->SpawnType,SpawnConf);
				LocalData.Add_Actor(DataAsset->ActorsDatas);
				LocalData.SetWS_Volume(WSVolume);
				Grid_Thread_AddPending(LocalData);
				break;
			}
		case :: StaticMesh:
			{
				FWS_Grid_Queue LocalData = FWS_Grid_Queue(DataAsset->SpawnType,SpawnConf);
				LocalData.Add_StaticMesh(DataAsset->StaticMeshDatas);
				LocalData.SetWS_Volume(WSVolume);
				Grid_Thread_AddPending(LocalData);
				break;
			}
		case :: Collection:
			{
				FWS_Grid_Queue LocalData = FWS_Grid_Queue(DataAsset->SpawnType,SpawnConf);
				LocalData.Add_CollectionActor(DataAsset->Collection_ActorsDatas);
				if ( !(Grid_SkipStaticMeshOnDedicated && GetWorld()->GetNetMode() == NM_DedicatedServer) )
				{
					LocalData.Add_StaticMesh(DataAsset->Collection_StaticMeshDatas);
				}
				LocalData.SetWS_Volume(WSVolume);
				Grid_Thread_AddPending(LocalData);
				break;
			}
		case :: CollectionStaticMesh:
			{
				FWS_Grid_Queue LocalData = FWS_Grid_Queue(DataAsset->SpawnType,SpawnConf);
				LocalData.Add_StaticMesh(DataAsset->Collection_StaticMeshDatas);
				LocalData.SetWS_Volume(WSVolume);
				Grid_Thread_AddPending(LocalData);
				break;
			}
		case :: Foliage:
			{
				FWS_Grid_Queue LocalData = FWS_Grid_Queue(DataAsset->SpawnType,SpawnConf);
				LocalData.Add_StaticMesh(DataAsset->StaticMeshDatas);
				LocalData.SetFoliageSectorSize(DataAsset->FoliageSectorSize);
				LocalData.SetWS_Volume(WSVolume);
				Grid_Thread_AddPending(LocalData);
				break;
			}
		case :: DataAsset:
			{
				break;
			}
		case :: OnlyData:
			{
				break;
			}
		}
		Grid_Thread_CheckAndMake();
	}
	else
	{
		switch ( DataAsset->SpawnType )
		{
		case :: Actor:
			{
				Grid_ComputeSpawn_Actor( SpawnConf , DataAsset->ActorsDatas ,WSVolume);
				break;
			}
		case :: StaticMesh:
			{
				Grid_ComputeSpawn_StaticMesh( SpawnConf , DataAsset->StaticMeshDatas ,WSVolume);
				break;
			}
		case :: Collection:
			{
				Grid_ComputeSpawn_Collection( SpawnConf , DataAsset->Collection_ActorsDatas , DataAsset->Collection_StaticMeshDatas ,WSVolume);
				break;
			}
		case :: CollectionStaticMesh:
			{
				Grid_ComputeSpawn_CollectionStaticMesh( SpawnConf , DataAsset->Collection_StaticMeshDatas ,WSVolume);
				break;
			}
		case :: Foliage:
			{
				Grid_ComputeSpawn_Foliage( SpawnConf  , DataAsset->StaticMeshDatas , DataAsset->FoliageSectorSize ,WSVolume);
				break;
			}
		case :: DataAsset:
			{
				break;
			}
		case :: OnlyData:
			{
				break;
			}
		}
	}
}
// --------------------------------------
void AWorldScapeRoot::Grid_ComputeSpawn_Actor( const FWS_Grid_SpawnConf& SpawnConf , const TArray<FWS_Grid_Spawner_Actor>& Actors ,const FWS_VolumeData& WS_Volume)
{
	TWeakObjectPtr<AWorldScapeRoot> WeakRoot(this);
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [ SpawnConf , Actors , this ,WS_Volume, WeakRoot]
	{
		FRandomStream Random = FRandomStream(SpawnConf.Seed);
		int32 SpawnCount = 0;
		TArray<FVector> SpawnLocations;
		TArray<FWS_Grid_Actor_SpawnData_Viz> SpawnDataOut;
		if ( AWorldScapeRoot* WorldScapeRoot = WeakRoot.Get() )
		{
			switch (SpawnConf.SamplingType)
			{
			case ::CircleRND:
				{
					bool Over = false;
					for ( int32 i = 0; i < SpawnConf.Max_TestToSpawn; i++ )
					{
						if ( Over )
						{
							break;
						}
						FVector TestLocation;
						if ( !WeakRoot.Get() )
						{
							return;
						}
						if ( WorldScapeRoot->WS_Grid_SearchPointOnDisk( SpawnConf.ZoneData , TestLocation , Random , SpawnConf.SpawnParams , SpawnLocations , SpawnConf.GridCoord.CellCenterLocation , SpawnConf.GridCoord.CellCenterNormal , SpawnConf.ZoneSize ,WS_Volume) )
						{
							FTransform Local;
							if ( !WeakRoot.Get() )
							{
								return;
							}
							if ( WorldScapeRoot->WS_Grid_SnapPoint( Local ,Random,SpawnConf.SpawnParams, TestLocation , SpawnConf.GridCoord.CellCenterNormal , SpawnConf.Z_Offset ,WS_Volume ) )
							{
								SpawnLocations.Add(Local.GetLocation());
						
								for ( int32 y = 0 ; y < Actors.Num() ; y++ )
								{
									const int32 Index = UKismetMathLibrary::RandomIntegerFromStream(Random,Actors.Num()-1);
									if ( UKismetMathLibrary::RandomFloatInRangeFromStream(Random,0.0f,1.0f)  < Actors[Index].SpawnableChance )
									{
										//UClass* LoadedClass = Actors[Index].Class.LoadSynchronous();
										//SpawnDataOut.Add(FWS_Grid_Actor_SpawnData_Viz(LoadedClass,Local,Actors[Index].ProxyData));
										if ( Actors.IsValidIndex(Index) )
										{
											if ( !WeakRoot.Get() )
											{
												return;
											}
											SpawnDataOut.Add(FWS_Grid_Actor_SpawnData_Viz(Actors[Index].Class,Local,Actors[Index].ProxyData,Actors[Index]));
											SpawnCount += 1;
											if ( SpawnCount >= SpawnConf.Max_Spawn )
											{
												Over = true;
												break;
											}
											break;
										}
									}
								}
							}
						}
						else
						{
							break;
						}
					}
					break;
				}
			case ::PoissonDisk:
				{
					break;
				}
			case ::Grid:
				{
					break;
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("WS : Error weak root"));
			return;
		}
		AsyncTask(ENamedThreads::GameThread, [ GridCoord = SpawnConf.GridCoord , Viz = SpawnConf.Proxy , OutData = MoveTemp(SpawnDataOut) , this , WeakRoot ]
		{
			if ( AWorldScapeRoot* WorldScapeRoot = WeakRoot.Get() )
			{
				if ( FWS_Grid_Container* inCell = Loaded_Cell.Find(GridCoord.Hash) )
				{
					(*inCell).OnPendingLoad -= 1;
					if ( (*inCell).OnPendingLoad == 0 )
					{
						(*inCell).OnLoad = false;
					}
					if ( WorldScapeRoot->Grid_UsePendingSpawn )
					{
						for ( const auto& Data : OutData )
						{
							WorldScapeRoot->Pending_SpawnCell.Add(FWS_Grid_SpawnPending( GridCoord,Data.Data.SpawnableTransform,Data.ProxyData,{Data.Data},TArray<FWS_Grid_Static_SpawnData>()));
						}
					}
					else
					{
						for ( const auto& Data : OutData )
						{
							WorldScapeRoot->Grid_Make_Instance(GridCoord,Data.Data.SpawnableTransform,Data.ProxyData,{Data.Data},TArray<FWS_Grid_Static_SpawnData>());
						}
					}
				}
			}
		});
	});
}
// --------------------------------------
void AWorldScapeRoot::Grid_ComputeSpawn_StaticMesh( const FWS_Grid_SpawnConf& SpawnConf  , const TArray<FWS_Grid_StaticMesh_Data>& StaticMesh ,const FWS_VolumeData& WS_Volume )
{
	TWeakObjectPtr<AWorldScapeRoot> WeakRoot(this);
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [ SpawnConf , StaticMesh , this ,WS_Volume, WeakRoot]
	{
		FRandomStream Random = FRandomStream(SpawnConf.Seed);
		int32 SpawnCount = 0;
		TArray<FVector> SpawnLocations;
		TArray<FWS_Grid_Static_SpawnData> SpawnDataOut;
		if ( AWorldScapeRoot* WorldScapeRoot = WeakRoot.Get() )
		{
			switch (SpawnConf.SamplingType)
			{
			case ::CircleRND:
				{
					bool Over = false;
					for ( int32 i = 0; i < SpawnConf.Max_TestToSpawn; i++ )
					{
						if ( Over )
						{
							break;
						}
						if ( !WeakRoot.Get() )
						{
							return;
						}
						FVector TestLocation;
						if ( WorldScapeRoot->WS_Grid_SearchPointOnDisk( SpawnConf.ZoneData , TestLocation , Random , SpawnConf.SpawnParams , SpawnLocations , SpawnConf.GridCoord.CellCenterLocation , SpawnConf.GridCoord.CellCenterNormal , SpawnConf.ZoneSize, WS_Volume) )
						{
							FTransform Local;
							if ( !WeakRoot.Get() )
							{
								return;
							}
							if ( WorldScapeRoot->WS_Grid_SnapPoint( Local ,Random,SpawnConf.SpawnParams, TestLocation , SpawnConf.GridCoord.CellCenterNormal , SpawnConf.Z_Offset ,WS_Volume ) )
							{
								SpawnLocations.Add(Local.GetLocation());
								for ( int32 j = 0 ; j < StaticMesh.Num() ; j++ )
								{
									const int32 Index = UKismetMathLibrary::RandomIntegerFromStream(Random,StaticMesh.Num()-1);
									if ( UKismetMathLibrary::RandomFloatInRangeFromStream(Random,0.0f,1.0f) < StaticMesh[Index].SpawnableChance )
									{
										if ( !WeakRoot.Get() )
										{
											return;
										}
										SpawnDataOut.Add(FWS_Grid_Static_SpawnData(StaticMesh[Index].StaticMesh,WS_Grid_ComputePlacement(Random,Local,StaticMesh[Index].MeshBounds,StaticMesh[Index].Placement,WS_Volume),StaticMesh[Index].Collision,StaticMesh[Index].CastShadow));
										SpawnCount += 1;
										if ( SpawnCount >= SpawnConf.Max_Spawn )
										{
											Over = true;
											break;
										}
										break;
										/*
										SpawnDataOut.Add(FWS_Grid_Static_SpawnData(StaticMesh[Index].StaticMesh,Local));
										SpawnCount += 1;
										if ( SpawnCount >= SpawnConf.Max_Spawn )
										{
											break;
										}
										*/
									}
								}
							}
						}
						else
						{
							break;
						}
					}
					break;
				}
			case ::PoissonDisk:
				{
					break;
				}
			case ::Grid:
				{
					break;
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("WS : Error weak root"));
			return;
		}
		AsyncTask(ENamedThreads::GameThread, [ GridCoord = SpawnConf.GridCoord , Viz = SpawnConf.Proxy , OutData = MoveTemp(SpawnDataOut) , this , WeakRoot ]
		{
			if ( AWorldScapeRoot* WorldScapeRoot = WeakRoot.Get() )
			{
				if ( FWS_Grid_Container* inCell = Loaded_Cell.Find(GridCoord.Hash) )
				{
					(*inCell).OnPendingLoad -= 1;
					if ( (*inCell).OnPendingLoad == 0 )
					{
						(*inCell).OnLoad = false;
					}
					const TArray<FWS_Grid_Actor_SpawnData> Empty;
					if ( WorldScapeRoot->Grid_UsePendingSpawn )
					{
						for ( const auto& Data : OutData )
						{
							WorldScapeRoot->Pending_SpawnCell.Add(FWS_Grid_SpawnPending(GridCoord,Data.SpawnableTransform,Viz,Empty, {Data} ));
						}
					}
					else
					{
						for ( const auto& Data : OutData )
						{
							WorldScapeRoot->Grid_Make_Instance(GridCoord,Data.SpawnableTransform,Viz,Empty, {Data} );
						}
					}
				}
			}
		});
	});
}
// --------------------------------------
void AWorldScapeRoot::Grid_ComputeSpawn_Collection( const FWS_Grid_SpawnConf& SpawnConf  , const TArray<FWS_Grid_Actor_Data>& Actors , const TArray<FWS_Grid_StaticMesh_Data>& StaticMesh ,const FWS_VolumeData& WS_Volume )
{
	TWeakObjectPtr<AWorldScapeRoot> WeakRoot(this);
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [ SpawnConf , Actors , StaticMesh , this ,WS_Volume, WeakRoot]
	{
		FRandomStream Random = FRandomStream(SpawnConf.Seed);
		int32 SpawnCount = 0;
		TArray<FVector> SpawnLocations;
		
		TArray<FWS_Grid_Collection_SpawnData> SpawnDataOut;
		
		if ( AWorldScapeRoot* WorldScapeRoot = WeakRoot.Get() )
		{
			switch (SpawnConf.SamplingType)
			{
			case ::CircleRND:
				{
					for ( int32 i = 0; i < SpawnConf.Max_TestToSpawn; i++ )
					{
						FVector TestLocation;
						if ( !WeakRoot.Get() )
						{
							return;
						}
						if ( WorldScapeRoot->WS_Grid_SearchPointOnDisk( SpawnConf.ZoneData , TestLocation , Random , SpawnConf.SpawnParams , SpawnLocations , SpawnConf.GridCoord.CellCenterLocation , SpawnConf.GridCoord.CellCenterNormal , SpawnConf.ZoneSize,WS_Volume ) )
						{
							FTransform Local;
							if ( !WeakRoot.Get() )
							{
								return;
							}
							if ( WorldScapeRoot->WS_Grid_SnapPoint( Local ,Random,SpawnConf.SpawnParams, TestLocation , SpawnConf.GridCoord.CellCenterNormal , SpawnConf.Z_Offset ,WS_Volume) )
							{
								SpawnLocations.Add(Local.GetLocation());
								if ( !WeakRoot.Get() )
								{
									return;
								}
								TArray<FWS_Grid_Actor_SpawnData> LActor = Grid_Compute_CollectionActor( Random , Local , Actors ,WS_Volume);
								TArray<FWS_Grid_Static_SpawnData> LStaticMesh =  Grid_Compute_CollectionStaticMesh( Random , Local , StaticMesh,WS_Volume );
								SpawnDataOut.Add(FWS_Grid_Collection_SpawnData(Local,LActor,LStaticMesh));
								SpawnCount += 1;
								if ( SpawnCount >= SpawnConf.Max_Spawn )
								{
									break;
								}
							}
						}
						else
						{
							break;
						}
					}
					break;
				}
			case ::PoissonDisk:
				{
					break;
				}
			case ::Grid:
				{
					break;
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("WS : Error weak root"));
			return;
		}
		AsyncTask(ENamedThreads::GameThread, [ GridCoord = SpawnConf.GridCoord , Viz = SpawnConf.Proxy , OutData = MoveTemp(SpawnDataOut) , this , WeakRoot ]
		{
			if ( AWorldScapeRoot* WorldScapeRoot = WeakRoot.Get() )
			{
				if ( FWS_Grid_Container* inCell = Loaded_Cell.Find(GridCoord.Hash) )
				{
					(*inCell).OnPendingLoad -= 1;
					if ( (*inCell).OnPendingLoad == 0 )
					{
						(*inCell).OnLoad = false;
					}
					TArray<FWS_Grid_Collection_SpawnData>& MutableData = const_cast<TArray<FWS_Grid_Collection_SpawnData>&>(OutData);
					if ( WorldScapeRoot->Grid_UsePendingSpawn )
					{
						for ( auto& Data : MutableData )
						{
							WorldScapeRoot->Pending_SpawnCell.Add(FWS_Grid_SpawnPending(GridCoord,Data.BaseTransform,Viz,MoveTemp(Data.Actors),MoveTemp(Data.StaticMeshes)));
						}
					}
					else
					{
						for ( auto& Data : MutableData )
						{
							WorldScapeRoot->Grid_Make_Instance(GridCoord,Data.BaseTransform,Viz,MoveTemp(Data.Actors),MoveTemp(Data.StaticMeshes));
						}
					}
				}
			}
		});
	});
}
// --------------------------------------
void AWorldScapeRoot::Grid_ComputeSpawn_CollectionStaticMesh( const FWS_Grid_SpawnConf& SpawnConf  , const TArray<FWS_Grid_StaticMesh_Data>& StaticMesh ,const FWS_VolumeData& WS_Volume)
{
	TWeakObjectPtr<AWorldScapeRoot> WeakRoot(this);
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [ SpawnConf , StaticMesh , this ,WS_Volume, WeakRoot]
	{
		FRandomStream Random = FRandomStream(SpawnConf.Seed);
		int32 SpawnCount = 0;
		TArray<FVector> SpawnLocations;
		
		TArray<FWS_Grid_Collection_SpawnData> SpawnDataOut;
		
		if ( AWorldScapeRoot* WorldScapeRoot = WeakRoot.Get() )
		{
			switch (SpawnConf.SamplingType)
			{
			case ::CircleRND:
				{
					for ( int32 i = 0; i < SpawnConf.Max_TestToSpawn; i++ )
					{
						FVector TestLocation;
						if ( !WeakRoot.Get() )
						{
							return;
						}
						if ( WorldScapeRoot->WS_Grid_SearchPointOnDisk( SpawnConf.ZoneData , TestLocation , Random , SpawnConf.SpawnParams , SpawnLocations , SpawnConf.GridCoord.CellCenterLocation , SpawnConf.GridCoord.CellCenterNormal , SpawnConf.ZoneSize ,WS_Volume) )
						{
							FTransform Local;
							if ( !WeakRoot.Get() )
							{
								return;
							}
							if ( WorldScapeRoot->WS_Grid_SnapPoint( Local ,Random,SpawnConf.SpawnParams, TestLocation , SpawnConf.GridCoord.CellCenterNormal , SpawnConf.Z_Offset ,WS_Volume) )
							{
								SpawnLocations.Add(Local.GetLocation());
								if ( !WeakRoot.Get() )
								{
									return;
								}
								TArray<FWS_Grid_Static_SpawnData> LStaticMesh =  Grid_Compute_CollectionStaticMesh( Random , Local , StaticMesh ,WS_Volume);
								SpawnDataOut.Add(FWS_Grid_Collection_SpawnData(Local,LStaticMesh));
								SpawnCount += 1;
								if ( SpawnCount >= SpawnConf.Max_Spawn )
								{
									break;
								}
							}
						}
						else
						{
							break;
						}
					}
					break;
				}
			case ::PoissonDisk:
				{
					break;
				}
			case ::Grid:
				{
					break;
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("WS : Error weak root"));
			return;
		}
		AsyncTask(ENamedThreads::GameThread, [ GridCoord = SpawnConf.GridCoord , Viz = SpawnConf.Proxy , OutData = MoveTemp(SpawnDataOut) , this , WeakRoot ]
		{
			if ( AWorldScapeRoot* WorldScapeRoot = WeakRoot.Get() )
			{
				if ( FWS_Grid_Container* inCell = Loaded_Cell.Find(GridCoord.Hash) )
				{
					(*inCell).OnPendingLoad -= 1;
					if ( (*inCell).OnPendingLoad == 0 )
					{
						(*inCell).OnLoad = false;
					}
					TArray<FWS_Grid_Collection_SpawnData>& MutableData = const_cast<TArray<FWS_Grid_Collection_SpawnData>&>(OutData);
					if ( WorldScapeRoot->Grid_UsePendingSpawn )
					{
						for ( auto& Data : MutableData )
						{
							WorldScapeRoot->Pending_SpawnCell.Add(FWS_Grid_SpawnPending(GridCoord,Data.BaseTransform,Viz,MoveTemp(Data.Actors),MoveTemp(Data.StaticMeshes)));
						}
					}
					else
					{
						for ( auto& Data : MutableData )
						{
							WorldScapeRoot->Grid_Make_Instance(GridCoord,Data.BaseTransform,Viz,MoveTemp(Data.Actors),MoveTemp(Data.StaticMeshes));
						}
					}
				}
			}
		});
	});
}
// --------------------------------------
void AWorldScapeRoot::Grid_ComputeSpawn_Foliage( const FWS_Grid_SpawnConf& SpawnConf  , const TArray<FWS_Grid_StaticMesh_Data>& StaticMesh , const double& FoliageSectorSize , const FWS_VolumeData& WS_Volume )
{
	TWeakObjectPtr<AWorldScapeRoot> WeakRoot(this);
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [ SpawnConf , StaticMesh , this , FoliageSectorSize ,WS_Volume, WeakRoot]
	{
		FRandomStream Random = FRandomStream(SpawnConf.Seed);
		
		// Compute Grid for FoliageSector
		const FTransform Base = FTransform(FRotationMatrix::MakeFromZ(SpawnConf.GridCoord.CellCenterNormal).ToQuat() ,SpawnConf.GridCoord.CellCenterLocation);

		TArray<FWS_Grid_FoliageSpawnData> FoliagesSector;
		double FSectorSize = FoliageSectorSize;
		const int32 Step = FMath::Max((SpawnConf.ZoneSize*2.2)/FoliageSectorSize,1);
		if ( (SpawnConf.ZoneSize*2.2)/FoliageSectorSize < 1.0f )
		{
			FSectorSize = SpawnConf.ZoneSize;
		}
		if ( AWorldScapeRoot* WorldScapeRoot = WeakRoot.Get() )
		{
			switch (SpawnConf.SamplingType)
			{
			case ::CircleRND:
				{
					const FVector Forward = Base.GetRotation().GetForwardVector();
					const FVector Right = Base.GetRotation().GetRightVector();
					const FVector OffsetStart = ((((Step / 2) * Forward * FSectorSize) * -1) + (((Step / 2) * Right * FSectorSize) * -1)) + SpawnConf.GridCoord.CellCenterLocation;
					
					for (int32 x = 0; x < Step; x++)
					{
						for (int32 y = 0; y < Step; y++)
						{
							FVector NewCenter = OffsetStart + (x * Forward * FSectorSize) + (y * Right * FSectorSize);
							TArray<FWS_Grid_Static_SpawnData> SpawnDataOut;
							int32 SpawnCount = 0;
							TArray<FVector> SpawnLocations;
							
							bool Over = false;
							for ( int32 i = 0; i < SpawnConf.Max_TestToSpawn; i++ )
							{
								if ( Over )
								{
									break;
								}
								FVector TestLocation;
								if ( !WeakRoot.Get() )
								{
									return;
								}
								if ( WorldScapeRoot->WS_Grid_SearchPointOnDisk( SpawnConf.ZoneData , TestLocation , Random , SpawnConf.SpawnParams , SpawnLocations , NewCenter , SpawnConf.GridCoord.CellCenterNormal , FSectorSize*0.65 , WS_Volume , 0.25 ) )
								{
									FTransform Local;
									if ( !WeakRoot.Get() )
									{
										return;
									}
									if ( WorldScapeRoot->WS_Grid_SnapPoint( Local ,Random,SpawnConf.SpawnParams, TestLocation , SpawnConf.GridCoord.CellCenterNormal , SpawnConf.Z_Offset , WS_Volume) )
									{
										SpawnLocations.Add(Local.GetLocation());
										for ( int32 j = 0 ; j < StaticMesh.Num() ; j++ )
										{
											const int32 Index = UKismetMathLibrary::RandomIntegerFromStream(Random,StaticMesh.Num()-1);
											if ( UKismetMathLibrary::RandomFloatInRangeFromStream(Random,0.0f,1.0f) < StaticMesh[Index].SpawnableChance )
											{
												if ( !WeakRoot.Get() )
												{
													return;
												}
												SpawnDataOut.Add(FWS_Grid_Static_SpawnData(StaticMesh[Index].StaticMesh,WS_Grid_ComputePlacement(Random,Local,StaticMesh[Index].MeshBounds,StaticMesh[Index].Placement,WS_Volume),StaticMesh[Index].Collision,StaticMesh[Index].CastShadow));
												SpawnCount += 1;
												if ( SpawnCount >= SpawnConf.Max_Spawn )
												{
													Over = true;
													break;
												}
												break;
											}
										}
									}
								}
								else
								{
									break;
								}
							}
							if ( !SpawnDataOut.IsEmpty() )
							{
								const FTransform BaseTransform = FTransform( Base.Rotator(),NewCenter);
								FoliagesSector.Add(FWS_Grid_FoliageSpawnData(BaseTransform,SpawnDataOut));
								if ( !WeakRoot.Get() )
								{
									return;
								}
								if ( FoliagesSector.Num() > WorldScapeRoot->Grid_FoliageBlockSend )
								{
									AsyncTask(ENamedThreads::GameThread, [ GridCoord = SpawnConf.GridCoord , Viz = SpawnConf.Proxy , OutData = MoveTemp(FoliagesSector) , this , WeakRoot ]
									{
										if ( AWorldScapeRoot* WorldScapeRoot = WeakRoot.Get() )
										{
											if ( FWS_Grid_Container* inCell = Loaded_Cell.Find(GridCoord.Hash) )
											{
												(*inCell).OnPendingLoad -= 1;
												if ( (*inCell).OnPendingLoad == 0 )
												{
													(*inCell).OnLoad = false;
												}
												const TArray<FWS_Grid_Actor_SpawnData> ActorNull;
												TArray<FWS_Grid_FoliageSpawnData>& MutableData = const_cast<TArray<FWS_Grid_FoliageSpawnData>&>(OutData);
												if ( WorldScapeRoot->Grid_UsePendingSpawn )
												{
													for ( auto& Data : MutableData )
													{
														WorldScapeRoot->Pending_SpawnCell.Add(FWS_Grid_SpawnPending(GridCoord,Data.BaseTransform,Viz,ActorNull,MoveTemp(Data.MeshData),true));
													}
												}	
												else
												{
													for ( auto& Data : MutableData )
													{
														WorldScapeRoot->Grid_Make_Instance(GridCoord,Data.BaseTransform,Viz,ActorNull,MoveTemp(Data.MeshData), true);
													}
												}
											}
										}
									});
									FoliagesSector.Empty();
								}
							}
						}
					}
				}
			case ::PoissonDisk:
				{
					break;
				}
			case ::Grid:
				{
					break;
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("WS : Error weak root"));
			return;
		}
		if ( !FoliagesSector.IsEmpty() )
		{
			AsyncTask(ENamedThreads::GameThread, [ GridCoord = SpawnConf.GridCoord , Viz = SpawnConf.Proxy , OutData = MoveTemp(FoliagesSector) , this , WeakRoot ]
			{
				if ( AWorldScapeRoot* WorldScapeRoot = WeakRoot.Get() )
				{
					if ( FWS_Grid_Container* inCell = Loaded_Cell.Find(GridCoord.Hash) )
					{
						(*inCell).OnPendingLoad -= 1;
						if ( (*inCell).OnPendingLoad == 0 )
						{
							(*inCell).OnLoad = false;
						}
						const TArray<FWS_Grid_Actor_SpawnData> ActorNull;
						TArray<FWS_Grid_FoliageSpawnData>& MutableData = const_cast<TArray<FWS_Grid_FoliageSpawnData>&>(OutData);
						if ( WorldScapeRoot->Grid_UsePendingSpawn )
						{
							for ( auto& Data : MutableData )
							{
								WorldScapeRoot->Pending_SpawnCell.Add(FWS_Grid_SpawnPending(GridCoord,Data.BaseTransform,Viz,ActorNull,MoveTemp(Data.MeshData),true));
							}
						}
						else
						{
							for ( auto& Data : MutableData )
							{
								WorldScapeRoot->Grid_Make_Instance(GridCoord,Data.BaseTransform,Viz,ActorNull,MoveTemp(Data.MeshData), true);
							}
						}
					}
				}
			});
		}
	});
}
// --------------------------------------
bool AWorldScapeRoot::WS_Grid_SearchPointOnDisk( const TArray<FWS_Grid_InvalidZone>& ZoneData , FVector& Out , FRandomStream& Random , const FWS_Grid_SpawnParams& SpawnParams , const TArray<FVector>& Spawned , const FVector& Center , const FVector& UpVector , const float& Radius , const FWS_VolumeData& WS_Volume, const float& TestScale )
{
	for ( int32 i = 0 ; i < Grid_RNDCircle_TestLimite*TestScale ; i++ )
	{
		const FVector Direction = FVector(1,0,0).RotateAngleAxis(UKismetMathLibrary::RandomFloatInRangeFromStream(Random,1.0f,360.0f),UpVector);
		const FVector Location = Center+(Direction*UKismetMathLibrary::RandomFloatInRangeFromStream(Random,0.0f,Radius));
		if ( Grid_CheckInValidZone( ZoneData , Location ) )
		{
			if ( !Spawned.IsEmpty() )
			{
				for ( const auto& Last : Spawned )
				{
					if ( (Last-Location).SquaredLength() > (SpawnParams.InvalidSize*SpawnParams.InvalidSize) )
					{
						Out = Location;
						if ( bOcean )
						{
							if (!SpawnParams.SpawnableInWater)
							{
								const double Ground = GetNoise_WSVolume(Location, WS_Volume , false).Height;
								const double Water = GetNoise_WSVolume(Location, WS_Volume , true).Height;
								
								//const double Ground = GetGroundHeight(Location, false);
								//const double Water = GetGroundHeight(Location, true);
								if ( Ground > Water )
								{
									if ( Grid_CheckBySpawnParams( Location, GetNoise_WSVolume(Location, WS_Volume) , SpawnParams , WS_Volume ) )
									{
										return true;
									}
									return false;
								}
							}
							else
							{
								if ( Grid_CheckBySpawnParams( Location, GetNoise_WSVolume(Location, WS_Volume) , SpawnParams , WS_Volume ) )
								{
									return true;
								}
								return false;
							}
						}
						else
						{
							if ( Grid_CheckBySpawnParams( Location, GetNoise_WSVolume(Location, WS_Volume) , SpawnParams , WS_Volume ) )
							{
								return true;
							}
							return false;
						}
					}
				}
			}
			else
			{
				Out = Location;
				if (!SpawnParams.SpawnableInWater)
				{
					const double Ground = GetNoise_WSVolume(Location, WS_Volume , false).Height;
					const double Water = GetNoise_WSVolume(Location, WS_Volume , true).Height;
					//const double Ground = GetGroundHeight(Location, false);
					//const double Water = GetGroundHeight(Location, true);
					if ( Ground > Water )
					{
						if ( Grid_CheckBySpawnParams( Location, GetNoise_WSVolume(Location, WS_Volume) , SpawnParams , WS_Volume ) )
						{
							return true;
						}
						return false;
					}
				}
				else
				{
					if ( Grid_CheckBySpawnParams( Location, GetNoise_WSVolume(Location, WS_Volume) , SpawnParams , WS_Volume ) )
					{
						return true;
					}
					return false;
				}
			}
		}
	}
	return false;
}
// --------------------------------------
bool AWorldScapeRoot::WS_Grid_SnapPoint( FTransform& Transform , FRandomStream& Random , const FWS_Grid_SpawnParams& SpawnParams , const FVector& Location , const FVector& UpVector , const double& Offset ,const FWS_VolumeData& WS_Volume )
{
	if ( !IsValid(this) )
	{
		return false;
	}
	const FVector A = FVector(1,0,0).RotateAngleAxis(1.0, UpVector);
	const FVector B = FVector(1,0,0).RotateAngleAxis(120.0, UpVector);
	const FVector C = FVector(1,0,0).RotateAngleAxis(-120.0, UpVector);

	const FVector Dir0 = ((Location + A*SpawnParams.SpawnSize*0.5)-GetActorLocation()).GetSafeNormal();
	const FVector T0 = GetActorLocation()+(Dir0*(PlanetScale + GetNoise_WSVolume(Location + A*SpawnParams.SpawnSize*0.5,WS_Volume).Height));
	const FVector Dir1 = ((Location + B*SpawnParams.SpawnSize*0.5)-GetActorLocation()).GetSafeNormal();
	const FVector T1 = GetActorLocation()+(Dir1*(PlanetScale + GetNoise_WSVolume(Location + B*SpawnParams.SpawnSize*0.5,WS_Volume).Height));
	const FVector Dir2 = ((Location + C*SpawnParams.SpawnSize*0.5)-GetActorLocation()).GetSafeNormal();
	const FVector T2 = GetActorLocation()+(Dir2*(PlanetScale + GetNoise_WSVolume(Location + C*SpawnParams.SpawnSize*0.5,WS_Volume).Height));
	const FVector Normal = FVector::CrossProduct(T1 - T0, T2 - T0).GetSafeNormal();
	
	if ( (1.0f - UpVector.Dot(Normal)) < SpawnParams.MaxSlope )
	{
		if ( SpawnParams.CheckDifference )
		{
			TArray<double> H_Sample;
			double H_Count = 0.0f;
			const FVector Forward = FVector(1, 0, 0).RotateAngleAxis(0.0f,UpVector);
			const FVector Right = FVector(1, 0, 0).RotateAngleAxis(90.0f,UpVector);
			const FVector StartOffset = Forward*(-SpawnParams.SpawnSize/2) + Right*(-SpawnParams.SpawnSize/2);
			for (int32 x = 0; x < 5; x++)
			{
				for (int32 y = 0; y < 5; y++)
				{
					const double H = GetNoise_WSVolume(StartOffset+(Forward*((SpawnParams.SpawnSize/5)*x) + Right*((SpawnParams.SpawnSize/5)*y)),WS_Volume).Height;
					H_Sample.Add(H);
					H_Count += H;
				}
			}
			const double HDifference = FMath::Max(H_Sample) - FMath::Min(H_Sample);
			if ( HDifference > SpawnParams.Max_ElevationDifference )
			{
				return false;
			}
		}
		const FVector Dir = (Location-GetActorLocation()).GetSafeNormal();
		//const double Ground = GetGroundHeight_HOnly(Location);
		const double Ground = GetNoise_WSVolume(Location,WS_Volume).Height;
		const FVector OutLocation = GetActorLocation() + (Dir* (PlanetScale + Ground + Offset ));
		Transform.SetLocation(OutLocation);
		const FVector RND_Dir = FVector(1,0,0).RotateAngleAxis(UKismetMathLibrary::RandomFloatInRangeFromStream(Random,1.0f,360.0f), Normal);
		//Transform.SetRotation(FRotationMatrix::MakeFromZ(Normal).ToQuat());
		Transform.SetRotation(SpawnParams.RotateToGround ? FRotationMatrix::MakeFromZX(Normal,RND_Dir).ToQuat() : FRotationMatrix::MakeFromZX(Dir,RND_Dir).ToQuat() );
		return true;
	}
	return false;
}
// --------------------------------------
bool AWorldScapeRoot::Grid_CheckBySpawnParams( const FVector& Location ,  const FNoiseData& Data , const FWS_Grid_SpawnParams& SpawnParams , const FWS_VolumeData& WS_Volume)
{
	if ( UKismetMathLibrary::InRange_FloatFloat(Data.Temperature,SpawnParams.MinTemperature,SpawnParams.MaxTemperature))
	{
		if ( UKismetMathLibrary::InRange_FloatFloat(Data.Humidity,SpawnParams.MinHumidity,SpawnParams.MaxHumidity))
		{
			FNoiseData NData = GetNoise_WSVolume(Location,WS_Volume);
			//const DVector ECEFPosition = WorldToECEF(DVector(Location));
			//FNoiseData NData = GetNoise(ECEFPosition.ToFVector(), false);
			const double H = NData.Height;
			if ( UKismetMathLibrary::InRange_FloatFloat(H,SpawnParams.MinElevation,SpawnParams.MaxElevation))
			{
				return true;
			}
		}
	}
	return false;
}
// --------------------------------------
TArray<FVector> AWorldScapeRoot::WS_Grid_SearchPoissonDisk( const TArray<FWS_Grid_InvalidZone>& ZoneData , const int32& MaxSpawn , FRandomStream& Random , const FWS_Grid_SpawnParams& SpawnParams , const FVector& Center , const float& Radius ,const FWS_VolumeData& WS_Volume)
{
	const FVector Tangent = GetTangentFromECEFPoint(Center);
	const FVector BiTangent = GetBiTangentFromECEFPoint(Center);
	// Get a cluster of points
	TArray<FVector> PointsList;
	
	const float averageRadius = 1/SpawnParams.PoissonDiscDensity * Radius;
	const float minRadius = averageRadius - averageRadius * SpawnParams.PoissonDiscDensityVariation*0.01f;
	const float maxRadius = averageRadius + averageRadius * SpawnParams.PoissonDiscDensityVariation*0.01f;
	TArray< FPoissonObject> GeneratedPoints = FPoissonDiscSampling::GeneratePoints(minRadius, maxRadius, FVector2D(Radius), Random, 30);
	// Shuffle Data
	TArray<int32> ShuffleData;
	Grid_ShuffleFromStream(GeneratedPoints.Num(),ShuffleData,Random);

	int32 Count = 0;
	
	for (int32 j = 0; j < ShuffleData.Num(); j++)
	{
		FVector FinalPosition = Center + Tangent * GeneratedPoints[ShuffleData[j]].Position.X + BiTangent * GeneratedPoints[ShuffleData[j]].Position.Y - (Tangent* Radius*0.5f + BiTangent * Radius * 0.5f);
		if ( Grid_CheckInValidZone( ZoneData , FinalPosition ) )
		{
			Grid_CheckBySpawnParams( FinalPosition , GetNoise_WSVolume(FinalPosition,WS_Volume, false) , SpawnParams , WS_Volume);
			PointsList.Add(FinalPosition);
			Count++;
			if ( Count >= MaxSpawn )
			{
				return PointsList;
			}
		}
	}
	return PointsList;
}
// --------------------------------------
TArray<FVector> AWorldScapeRoot::WS_Grid_SearchGridSampling( const TArray<FWS_Grid_InvalidZone>& ZoneData , const int32& MaxSpawn , FRandomStream& Random , const FWS_Grid_SpawnParams& SpawnParams , const FTransform& BaseTransform , const double& BaseSize ,const FWS_VolumeData& WS_Volume)
{
	const int32 Step = FMath::Max((BaseSize)/SpawnParams.SpawnSize,1);
	
	const FVector Forward = BaseTransform.GetRotation().GetForwardVector();
	const FVector Right = BaseTransform.GetRotation().GetRightVector();
	const FVector OffsetStart = ((((Step / 2) * Forward * SpawnParams.SpawnSize) * -1) + (((Step / 2) * Right * SpawnParams.SpawnSize) * -1)) + BaseTransform.GetLocation();
					
	TArray<FVector> PointsList;
	for (int32 x = 0; x < Step; x++)
	{
		for (int32 y = 0; y < Step; y++)
		{
			PointsList.Add(OffsetStart + (x * Forward * SpawnParams.SpawnSize) + (y * Right * SpawnParams.SpawnSize));
		}
	}
	// Shuffle Data
	TArray<int32> ShuffleData;
	Grid_ShuffleFromStream(PointsList.Num(),ShuffleData,Random);
	int32 Count = 0;
	TArray<FVector> Out;
	for (int32 j = 0; j < ShuffleData.Num(); j++)
	{
		if ( Grid_CheckInValidZone( ZoneData , PointsList[ShuffleData[j]] ) )
		{
			Grid_CheckBySpawnParams( PointsList[ShuffleData[j]] , GetNoise_WSVolume(PointsList[ShuffleData[j]], WS_Volume,false) , SpawnParams ,WS_Volume );
			Out.Add(PointsList[ShuffleData[j]]);
			Count++;
			if ( Count >= MaxSpawn )
			{
				return PointsList;
			}
		}
	}
	return Out;
}
// --------------------------------------
TArray<FVector> AWorldScapeRoot::WS_Grid_GenerateZoneGrid( const FTransform& BaseTransform , const double& Size , const int32 Step )
{
	const double StepSize = Size / Step;
	const FVector Forward = BaseTransform.GetRotation().GetForwardVector();
	const FVector Right = BaseTransform.GetRotation().GetRightVector();
	const FVector OffsetStart = ((((Step / 2) * Forward * StepSize) * -1) + (((Step / 2) * Right * StepSize) * -1)) + BaseTransform.GetLocation();
					
	TArray<FVector> PointsList;
	for (int32 x = 0; x < Step; x++)
	{
		for (int32 y = 0; y < Step; y++)
		{
			PointsList.Add(OffsetStart + (x * Forward * StepSize) + (y * Right * StepSize));
		}
	}
	return PointsList;
}
// --------------------------------------
TArray<FWS_Grid_InvalidZone> AWorldScapeRoot::Grid_MakeInvalidZoneData( const FVector& CellLocation , const double& CellSize , const TArray<AWS_Grid_InvalidSphere*>& Zone )
{
	if ( !Zone.IsEmpty() )
	{
		TArray<FWS_Grid_InvalidZone> Local;
		for ( const auto& Area : Zone )
		{
			if ( IsValid( Area ) )
			{
				if ( (CellLocation-Area->GetActorLocation()).SquaredLength() < FMath::Square(CellSize + Area->WS_ZoneGetSize()) )
				{
					Local.Add(FWS_Grid_InvalidZone(Area->GetActorLocation(),Area->WS_ZoneGetSize()));
				}
			}
		}
		return Local;
	}
	return TArray<FWS_Grid_InvalidZone>();
}
// --------------------------------------
bool AWorldScapeRoot::Grid_CheckInValidZone( const TArray<FWS_Grid_InvalidZone>& Zone , const FVector& Location )
{
	if ( !Zone.IsEmpty() )
	{
		for ( const auto& Area : Zone )
		{
			if ( (Area.Location-Location).SquaredLength() < FMath::Square(Area.Size))
			{
				return false;
			}
		}
	}
	return true;
}
// --------------------------------------
FTransform AWorldScapeRoot::WS_Grid_ComputePlacement( FRandomStream& Random , const FTransform& BaseTransform , /*const FVector& Extend*/ const FBoxSphereBounds& Bound , const FWS_Grid_Placement_Data& Placement ,const FWS_VolumeData& WS_Volume)
{
	const FBoxSphereBounds LocalBound = FBoxSphereBounds(Bound.Origin,Bound.BoxExtent*BaseTransform.GetScale3D(),Bound.SphereRadius*BaseTransform.GetScale3D().GetAbsMax());
	const double BoundOffset = LocalBound.Origin.Z - LocalBound.BoxExtent.Z; 
	//const FVector ComputeExtend = BaseTransform.GetScale3D() * Extend;
	FVector Location = BaseTransform.TransformPosition( Placement.Transform.GetLocation() );
	FRotator Rotation = BaseTransform.TransformRotation(Placement.Transform.GetRotation() ).Rotator();
	FVector Forward = Rotation.Vector();
	FVector Scale = Placement.Transform.GetScale3D();
	if ( Placement.Use_DiskRND )
	{
		const float RNDAngle = UKismetMathLibrary::RandomFloatInRangeFromStream(Random,1.0f,360.0f);
		const FVector DiskRND = FVector(1.0,0.0,0.0).RotateAngleAxis(RNDAngle,BaseTransform.GetRotation().GetUpVector()) * Placement.DiskRND_Size;
		Location += DiskRND;
	}
	// TODO make rotate box extend by transform
	if ( Placement.SnapGround && !Placement.RotateToGround )
	{
		const double H = GetNoise_WSVolume(Location, WS_Volume,false).Height;
		const FVector DirBase = (Location-GetActorLocation()).GetSafeNormal();
		//Location = GetActorLocation() + ( DirBase * (PlanetScale+H+ComputeExtend.Z+Placement.Z_Offset) );
		Location = GetActorLocation() + ( DirBase * (PlanetScale + H + Placement.Z_Offset - BoundOffset) );
	}
	if ( Placement.RotateToGround )
	{
		//const float MaxSize = Extend.GetAbsMax();
		const float MaxSize = LocalBound.SphereRadius;
		FTransform SnapTransform = Grid_Math_GetTransformNormalAt( Location , MaxSize , UKismetMathLibrary::RandomFloatInRangeFromStream(Random,Placement.Yaw_Rotate.X,Placement.Yaw_Rotate.Y), Placement.Z_Offset - BoundOffset , Forward , Placement.RND_Yaw , WS_Volume);
		Rotation = SnapTransform.TransformRotation(Placement.Transform.GetRotation()).Rotator();
		Location = SnapTransform.GetLocation();
	}
	if ( Placement.RND_LinearScale )
	{
		Scale *= UKismetMathLibrary::RandomFloatInRangeFromStream(Random,Placement.LinearScale.X,Placement.LinearScale.Y);
	}
	return FTransform(Rotation,Location,Scale);
}
// --------------------------------------
TArray<FWS_Grid_Actor_SpawnData> AWorldScapeRoot::Grid_Compute_CollectionActor( FRandomStream& Random , const FTransform& BaseTransform , const TArray<FWS_Grid_Actor_Data>& Collection ,const FWS_VolumeData& WS_Volume)
{
	TArray<FWS_Grid_Actor_SpawnData> Local;
	for ( const auto& Data : Collection )
	{
		if ( Data.SpawnableChance < 1.0f )
		{
			if ( UKismetMathLibrary::RandomFloatInRangeFromStream(Random,0.0f,1.0f) > Data.SpawnableChance )
			{
				continue;
			}
		}
		Local.Add(FWS_Grid_Actor_SpawnData(Data.Class,WS_Grid_ComputePlacement(Random,BaseTransform,Data.ActorBounds,Data.Placement,WS_Volume),Data));
	}
	return Local;
}
// --------------------------------------
TArray<FWS_Grid_Static_SpawnData> AWorldScapeRoot::Grid_Compute_CollectionStaticMesh( FRandomStream& Random , const FTransform& BaseTransform , const TArray<FWS_Grid_StaticMesh_Data>& Collection ,const FWS_VolumeData& WS_Volume)
{
	TArray<FWS_Grid_Static_SpawnData> Local;
	for ( const auto& Data : Collection )
	{
		if ( Data.SpawnableChance < 1.0f )
		{
			if ( UKismetMathLibrary::RandomFloatInRangeFromStream(Random,0.0f,1.0f) > Data.SpawnableChance )
			{
				continue;
			}
		}
		Local.Add(FWS_Grid_Static_SpawnData(Data.StaticMesh,WS_Grid_ComputePlacement(Random,BaseTransform,Data.MeshBounds,Data.Placement,WS_Volume),Data.Collision,Data.CastShadow));
	}
	return Local;
}
// --------------------------------------
void AWorldScapeRoot::Grid_Make_Instance( const FWS_Grid& GridCoord , const FTransform& InstanceTransform , const FWS_Grid_Proxy& VizData , const TArray<FWS_Grid_Actor_SpawnData>& Actors , const TArray<FWS_Grid_Static_SpawnData>& StaticMesh , const bool IsFoliage )
{
	TRACE_CPUPROFILER_EVENT_SCOPE(Grid_Make_Instance);
	if ( UWorld* World = GetWorld() )
	{
		if ( FWS_Grid_Container* CellData = Loaded_Cell.Find(GridCoord.Hash) )
		{
			FWS_Grid_Instance NewInstance = FWS_Grid_Instance();
			NewInstance.Add_ActorData(Actors);
			NewInstance.Add_StaticMeshData(StaticMesh);
			
			if ( VizData.UseProxy )
			{
				if ( UStaticMesh* SMProxy = VizData.ProxyMesh.LoadSynchronous() )
				{
					FActorSpawnParameters SpawnParams;
					SpawnParams.ObjectFlags = RF_Transient;
					SpawnParams.Owner = this;
					SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					if ( AWorldScape_GridSMProxy* Proxy = World->SpawnActor<AWorldScape_GridSMProxy>(AWorldScape_GridSMProxy::StaticClass(),InstanceTransform.GetLocation(),InstanceTransform.GetRotation().Rotator(), SpawnParams) )
					{
						Proxy->Set_ProxyMesh(SMProxy);
						NewInstance.SM_Proxy = Proxy;
						FWS_Grid_Visualisation_Data LocalViz = FWS_Grid_Visualisation_Data(InstanceTransform.GetLocation(),VizData.ProxySwitchDistance);
						LocalViz.IsVisible = false;
						LocalViz.Switch_Visualisation = true;
						NewInstance.Visibility = LocalViz;
						if ( IsFoliage )
						{
							Proxy->SetFoliageMode();
						}

						if ( VizData.Use_AdditionalComponent )
						{
							if ( UClass* LoadedAdditional = VizData.AdditionalComponentClass.LoadSynchronous())
							{
								UActorComponent* Comp = NewObject<UActorComponent>( Proxy , LoadedAdditional , TEXT("GridAdditionalComp"));
								Proxy->AddInstanceComponent(Comp);
								Comp->RegisterComponent();
							}
						}
						Proxy->RegisterAllComponents();
					}
					else
					{
						if ( Grid_Debug )
						{
							UE_LOG(LogTemp, Log, TEXT("WS Grid : Proxy Fail "));
						}
					}
				}
				else
				{
					FWS_Grid_Visualisation_Data LocalViz = FWS_Grid_Visualisation_Data(InstanceTransform.GetLocation(),VizData.ProxySwitchDistance);
					LocalViz.IsVisible = false;
					LocalViz.Switch_Visualisation = true;
					NewInstance.Visibility = LocalViz;

					FActorSpawnParameters SpawnParams;
					SpawnParams.ObjectFlags = RF_Transient;
					SpawnParams.Owner = this;
					SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					if ( AWorldScape_GridSMProxy* Proxy = World->SpawnActor<AWorldScape_GridSMProxy>(AWorldScape_GridSMProxy::StaticClass(),InstanceTransform.GetLocation(),InstanceTransform.GetRotation().Rotator(), SpawnParams) )
					{
						NewInstance.SM_Proxy = Proxy;
						if ( IsFoliage )
						{
							Proxy->SetFoliageMode();
						}
						if ( VizData.Use_AdditionalComponent )
						{
							if ( UClass* LoadedAdditional = VizData.AdditionalComponentClass.LoadSynchronous())
							{
								UActorComponent* Comp = NewObject<UActorComponent>( Proxy , LoadedAdditional , TEXT("GridAdditionalComp"));
								Proxy->AddInstanceComponent(Comp);
								Comp->RegisterComponent();
							}
							Proxy->RegisterAllComponents();
						}
					}
					else
					{
						if ( Grid_Debug )
						{
							UE_LOG(LogTemp, Log, TEXT("WS Grid : Proxy Fail "));
						}
					}
				}
			}
			else
			{
				if ( !StaticMesh.IsEmpty() )
				{
					FActorSpawnParameters SpawnParams;
					SpawnParams.ObjectFlags = RF_Transient;
					SpawnParams.Owner = this;
					SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					if ( AWorldScape_GridSMProxy* Proxy = World->SpawnActor<AWorldScape_GridSMProxy>(AWorldScape_GridSMProxy::StaticClass(),InstanceTransform.GetLocation(),InstanceTransform.GetRotation().Rotator(), SpawnParams) )
					{
						if ( IsFoliage )
						{
							Proxy->SetFoliageMode();
						}
						NewInstance.SM_Proxy = Proxy;
						Proxy->Make_StaticMeshList(StaticMesh);
					}
					else
					{
						if ( Grid_Debug )
						{
							UE_LOG(LogTemp, Log, TEXT("WS Grid : Proxy Fail "));
						}
					}
				}
				for ( auto& InstanceActor : Actors )
				{
					FActorSpawnParameters SpawnParams;
					SpawnParams.ObjectFlags = RF_Transient;
					SpawnParams.Owner = this ;
					SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					UClass* LoadedClassActor = InstanceActor.SoftClass.LoadSynchronous();
					
					if ( InstanceActor.UseSerialize && !InstanceActor.SerializedData.IsEmpty() )
					{
						SpawnParams.bDeferConstruction = true;
						//AActor* SpawnedActor = World->SpawnActorDeferred<AActor>(LoadedClassActor, InstanceActor.SpawnableTransform , this , nullptr , ESpawnActorCollisionHandlingMethod::AlwaysSpawn );
						AActor* SpawnedActor = World->SpawnActor<AActor>(LoadedClassActor,InstanceActor.SpawnableTransform.GetLocation(),InstanceActor.SpawnableTransform.GetRotation().Rotator(),SpawnParams);
						if ( IsValid(SpawnedActor) )
						{
							SpawnedActor->SetFlags(RF_Transient);
							NewInstance.Actors_Instance.Add(SpawnedActor);
							// Add Deserialize Actor Data
							FMemoryReader Ar(InstanceActor.SerializedData, true);
							FObjectAndNameAsStringProxyArchive Archive(Ar, true);
							Archive.ArIsSaveGame = true;
							Archive.ArIgnoreOuterRef = true;
							SpawnedActor->Serialize(Archive);
							//SpawnedActor->SerializeScriptProperties(Archive);
							SpawnedActor->SetActorTransform(InstanceActor.SpawnableTransform);
							SpawnedActor->FinishSpawning(InstanceActor.SpawnableTransform);
						}
						else
						{
							if ( Grid_Debug )
							{
								UE_LOG(LogTemp, Log, TEXT("WS Grid : Fail spawn actor"));
							}
						}
					}
					else
					{
						AActor* SpawnedActor = World->SpawnActor<AActor>(LoadedClassActor,InstanceActor.SpawnableTransform.GetLocation(),InstanceActor.SpawnableTransform.GetRotation().Rotator(),SpawnParams);
						//AActor* SpawnedActor = World->SpawnActor<AActor>(InstanceActor.SpawnableClass,InstanceActor.SpawnableTransform.GetLocation(),InstanceActor.SpawnableTransform.GetRotation().Rotator(),SpawnParams);
						if ( IsValid(SpawnedActor) )
						{
							SpawnedActor->RegisterAllComponents();
							NewInstance.Actors_Instance.Add(SpawnedActor);
						}
						else
						{
							if ( Grid_Debug )
							{
								UE_LOG(LogTemp, Log, TEXT("WS Grid : Fail spawn actor"));
							}
						}
					}
					
				}
			}
			(*CellData).Instances.Add(NewInstance);
		}
	}
}
// --------------------------------------
void AWorldScapeRoot::Grid_Update_Proxy( const TArray<FVector>& Locations )
{
	TRACE_CPUPROFILER_EVENT_SCOPE(Grid_Update_Proxy);
	if ( Locations.IsEmpty() || Loaded_Cell.IsEmpty() )
	{
		return;
	}
	TMap<int64,FWS_Grid_UpdateViz> Local;
	for ( auto& Cell : Loaded_Cell )
	{
		TArray<FWS_Grid_Visualisation_Data> Instances;
		for ( auto& Instance : Cell.Value.Instances )
		{
			Instances.Add(Instance.Visibility);
		}
		Local.Add(Cell.Key,FWS_Grid_UpdateViz(Instances));
	}

	TWeakObjectPtr<AWorldScapeRoot> WeakRoot(this);
	
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [ this , Locations, Viz = MoveTemp(Local), WeakRoot]
	{
		TMap<int64,FWS_Grid_VizValue> Out;
		if ( AWorldScapeRoot* WorldScapeRoot = WeakRoot.Get() )
		{
			for ( const auto& CellIndex : Viz )
			{
				TArray<bool> BoolLocal;
				for ( const auto& Instance : CellIndex.Value.Viz )
				{
					BoolLocal.Add(Grid_Check_Proxy( Instance , Locations ));
				}
				Out.Add( CellIndex.Key , FWS_Grid_VizValue(BoolLocal) );
			}
			AsyncTask(ENamedThreads::GameThread, [ this , WeakRoot , VizData = MoveTemp(Out) ]
			{
				if ( AWorldScapeRoot* WorldScapeRootOut = WeakRoot.Get() )
				{
					for ( const auto& OutCell : VizData )
					{
						if ( FWS_Grid_Container* CellData = WorldScapeRootOut ->Loaded_Cell.Find(OutCell.Key) )
						{
							for ( int32 i = 0 ; i < OutCell.Value.Viz.Num() ; i++ )
							{
								if ( (*CellData).Instances.IsValidIndex(i) )
								{
									if ( (*CellData).Instances[i].Visibility.IsVisible != OutCell.Value.Viz[i] && (*CellData).Instances[i].Visibility.Switch_Visualisation )
									{
										(*CellData).Instances[i].Visibility.IsVisible = OutCell.Value.Viz[i];
										if ( (*CellData).Instances[i].SM_Proxy )
										{
											(*CellData).Instances[i].SM_Proxy->Visible_Proxy(!OutCell.Value.Viz[i]);
											if ( !OutCell.Value.Viz[i] )
											{
												(*CellData).Instances[i].SM_Proxy->Clear_StaticMeshList();
											}
											else
											{
												if ( !(*CellData).Instances[i].StaticMeshData.IsEmpty() )
												{
													(*CellData).Instances[i].SM_Proxy->Make_StaticMeshList((*CellData).Instances[i].StaticMeshData);
												}
											}
										}
										if ( (*CellData).Instances[i].Visibility.IsVisible )
										{
											if ( UWorld* World = WorldScapeRootOut ->GetWorld() )
											{
												for ( auto& InstanceActor : (*CellData).Instances[i].ActorData )
												{
													FActorSpawnParameters SpawnParams;
													SpawnParams.ObjectFlags = RF_Transient;
													SpawnParams.Owner = WorldScapeRootOut ;
													SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
													UClass* LoadedClassActor = InstanceActor.SoftClass.LoadSynchronous();

													if ( InstanceActor.UseSerialize && !InstanceActor.SerializedData.IsEmpty() )
													{
														SpawnParams.bDeferConstruction = true;
														//AActor* SpawnedActor = World->SpawnActorDeferred<AActor>(LoadedClassActor, InstanceActor.SpawnableTransform , this , nullptr , ESpawnActorCollisionHandlingMethod::AlwaysSpawn );
														AActor* SpawnedActor = World->SpawnActor<AActor>(LoadedClassActor,InstanceActor.SpawnableTransform.GetLocation(),InstanceActor.SpawnableTransform.GetRotation().Rotator(),SpawnParams);
														//AActor* SpawnedActor = World->SpawnActor<AActor>(InstanceActor.SpawnableClass,InstanceActor.SpawnableTransform.GetLocation(),InstanceActor.SpawnableTransform.GetRotation().Rotator(),SpawnParams);
														if ( IsValid(SpawnedActor) )
														{
															SpawnedActor->SetFlags(RF_Transient);
															(*CellData).Instances[i].Actors_Instance.Add(SpawnedActor);
															// Add Deserialize Actor Data
															FMemoryReader Ar(InstanceActor.SerializedData, true);
															FObjectAndNameAsStringProxyArchive Archive(Ar, true);
															Archive.ArIsSaveGame = true;
															Archive.ArIgnoreOuterRef = true;
															SpawnedActor->Serialize(Archive);
															//SpawnedActor->SerializeScriptProperties(Archive);
															SpawnedActor->SetActorTransform(InstanceActor.SpawnableTransform);
															SpawnedActor->FinishSpawning(InstanceActor.SpawnableTransform);
														}
													}
													else
													{
														AActor* SpawnedActor = World->SpawnActor<AActor>(LoadedClassActor,InstanceActor.SpawnableTransform.GetLocation(),InstanceActor.SpawnableTransform.GetRotation().Rotator(),SpawnParams);
														//AActor* SpawnedActor = World->SpawnActor<AActor>(InstanceActor.SpawnableClass,InstanceActor.SpawnableTransform.GetLocation(),InstanceActor.SpawnableTransform.GetRotation().Rotator(),SpawnParams);
														if ( IsValid(SpawnedActor) )
														{
															SpawnedActor->RegisterAllComponents();
															(*CellData).Instances[i].Actors_Instance.Add(SpawnedActor);
														}
													}
												}
											}
										}
										else
										{
											for ( auto& InstanceActor : (*CellData).Instances[i].Actors_Instance )
											{
												if ( IsValid(InstanceActor))
												{
													InstanceActor->UnregisterAllComponents();
													InstanceActor->Destroy();
												}
											}
											(*CellData).Instances[i].Actors_Instance.Empty();
										}
									}
								}
							}
						}
					}
				}
			});
		}
	});
}
// --------------------------------------
bool AWorldScapeRoot::Grid_Check_Proxy( const FWS_Grid_Visualisation_Data& Data , const TArray<FVector>& Locations)
{
	if ( Data.Switch_Visualisation )
	{
		for ( const auto& Loc : Locations )
		{
			if ( (Loc-Data.Location).SquaredLength() < Data.VisualisationSizeSquared )
			{
				return true;
			}
		}
		return false;
	}
	else
	{
		return true;
	}
}
// --------------------------------------
void AWorldScapeRoot::Grid_MakeBaseStaticData()
{
	if ( Grid_CellData )
	{
		int32 Size = Grid_Data_Size;
		Grid_CellData->Grid_StaticData.Empty();
		Grid_CellData->Grid_StaticData.Reserve(Size * Size * 6);

		for (int32 Face = 0; Face < 6; Face++)
		{
			for (int32 Y = 0; Y < Size; Y++)
			{
				for (int32 X = 0; X < Size; X++)
				{
					int32 AdjustedX = X - (Size / 2);
					int32 AdjustedY = Y - (Size / 2);
					Grid_CellData->Grid_StaticData.Add(FWS_Grid(AdjustedX, AdjustedY, Face, Size, GetActorLocation(), PlanetScale));
				}
			}
		}
	}
	/*
	Grid_StaticData.Empty();
	Grid_StaticData.Reserve((Grid_Size * Grid_Size)*6);

	for ( int32 i = 0 ; i < 6 ; i++ )
	{
		for ( int32 x = -Grid_Size/2 ; x < (Grid_Size/2); x++ )
		{
			for ( int32 y = -Grid_Size/2 ; y < (Grid_Size/2); y++ )
			{
				Grid_StaticData.Add(FWS_Grid( x, y , i , Grid_Size , GetActorLocation() , PlanetScale ));
			}
		}
	}
	*/
}
// --------------------------------------
void AWorldScapeRoot::Grid_UpdateInfo()
{
#if WITH_EDITOR
	if ( FoliageGenerationThreadPool != nullptr )
	{
		FoliageThread = true;
	}
	else
	{
		FoliageThread = false;
	}
	OnPendingAsyncCompute = 0;
	for ( const auto& Data : ArrayQueue_AsyncCompute )
	{
		if ( !Data.IsEmpty() )
		{
			OnPendingAsyncCompute++;
		}
	}
	TempQueueLength = TempsQueue.Num();
	//OnPendingAsyncCompute = Queue_AsyncCompute.IsEmpty() ? false : true;
	OnWaitingSpawn = Queue_Spawn.IsEmpty() ? false : true;
	
	NumberOfThread = Grid_ThreadPool.Num();
	NumberOfCellLoaded = Loaded_Cell.Num();
	LeftPendingToSpawn = Pending_SpawnCell.Num();
	LeftPendingUnLoad = Pending_Unload.Num();
	// Count Destroy
	int32 CountDestroy = 0;
	for ( const auto& DestroyData : Pending_DestroyCell )
	{
		CountDestroy += DestroyData.Instances.Num();
	}
	LeftPendingToDestroy = CountDestroy;
	// Count volume
	int32 CountVolume = 0;
	for ( const auto& VolumeData : Grid_SpawnerVolumes )
	{
		CountVolume += VolumeData.Value.Spawners.Num();
	}
	NumberOfRegisterdVolume = CountVolume;
#endif
}
// --------------------------------------
FTransform AWorldScapeRoot::Grid_Math_GetTransformNormalAt( const FVector& Location , const double Size , const double Yaw , const double Z_Offset , const FVector& Forward , const bool& RND_Yaw , const FWS_VolumeData& WS_Volume)
{
	const FVector Dir = (Location-GetActorLocation()).GetSafeNormal();
	const double H = GetNoise_WSVolume(Location,WS_Volume).Height;
	const FVector Ground = GetActorLocation() + ( Dir*(PlanetScale+H+Z_Offset));
	const double Offset = FMath::Max(1.0f,Size);
	
	const FVector Off0 = FVector(Offset,0,0).RotateAngleAxis(1.0,Dir.GetSafeNormal())+Location;
	const FVector Off1 = FVector(Offset,0,0).RotateAngleAxis(120.0,Dir.GetSafeNormal())+Location;
	const FVector Off2 = FVector(Offset,0,0).RotateAngleAxis(240.0,Dir.GetSafeNormal())+Location;
	const FVector D0 = (Off0-GetActorLocation()).GetSafeNormal();
	const FVector D1 = (Off1-GetActorLocation()).GetSafeNormal();
	const FVector D2 = (Off2-GetActorLocation()).GetSafeNormal();
	const double H0 = GetNoise_WSVolume(Off0,WS_Volume).Height;
	const double H1 = GetNoise_WSVolume(Off1,WS_Volume).Height;
	const double H2 = GetNoise_WSVolume(Off2,WS_Volume).Height;
	const FVector T0 = GetActorLocation() + ( D0*(PlanetScale+H0));
	const FVector T1 = GetActorLocation() + ( D1*(PlanetScale+H1));
	const FVector T2 = GetActorLocation() + ( D2*(PlanetScale+H2));
	
	const FVector Normal = FVector::CrossProduct(T1 - T0, T2 - T0).GetSafeNormal();

	if ( RND_Yaw )
	{
		const FVector XAs = FVector(1.0,0,0).RotateAngleAxis(FMath::Max(1.0,Yaw),Dir);
		return FTransform(FRotationMatrix::MakeFromZX(Normal,XAs).Rotator(),Ground,FVector(1.0,1.0,1.0));
	}
	else
	{
		return FTransform(FRotationMatrix::MakeFromZX(Normal,Forward).Rotator(),Ground,FVector(1.0,1.0,1.0));
	}
}
// --------------------------------------
bool AWorldScapeRoot::Grid_GetOnPendingLoad()
{
	return Grid_PendingWork;
}
// --------------------------------------
void AWorldScapeRoot::Grid_RegisterSpawnerVolume( AWorldScape_GridSpawnerVolume* Spawner , FWS_Grid& Coord )
{
	if ( IsValid(Spawner) && EnabledGrid )
	{
		Coord = FWS_Grid( GetActorLocation() , Spawner->GetActorLocation() , Grid_Size , PlanetScale );
		const FWS_Grid_SpawnerVolumeData SpawnerData = FWS_Grid_SpawnerVolumeData(Coord,Spawner->GetActorLocation(),Spawner->WS_ZoneGetSize(),Spawner->SpawnDataAsset,Spawner->SpawnScale);
		if ( FWS_Grid_SpawnerArray* Index = Grid_SpawnerVolumes.Find(Coord.Hash) )
		{
			(*Index).Spawners.Add(Spawner,SpawnerData);
		}
		else
		{
			Grid_SpawnerVolumes.Add(Coord.Hash,FWS_Grid_SpawnerArray(Spawner,SpawnerData));
		}
		if ( Loaded_Cell.Contains(Coord.Hash) && !Pending_Unload.Contains(Coord.Hash) )
		{
			Grid_SpawnByVolume( Coord , SpawnerData );
		}
	}
}
// --------------------------------------
void AWorldScapeRoot::Grid_UnRegisterSpawnerVolume( AWorldScape_GridSpawnerVolume* Spawner , const FWS_Grid& Coord )
{
	if ( IsValid(Spawner) )
	{
		if ( FWS_Grid_SpawnerArray* Index = Grid_SpawnerVolumes.Find(Coord.Hash) )
		{
			(*Index).Spawners.Remove(Spawner);
			if ( (*Index).Spawners.IsEmpty() )
			{
				Grid_SpawnerVolumes.Remove(Coord.Hash);
			}
		}
	}
}
// --------------------------------------
void AWorldScapeRoot::Grid_SpawnByVolume( const FWS_Grid& GridCoord , const FWS_Grid_SpawnerVolumeData& Data )
{
	TRACE_CPUPROFILER_EVENT_SCOPE(Grid_SpawnByVolume);
	const double SpawnSize = Data.Size;
	const int32 BaseSeed = FMath::FloorToInt(Data.Position.Dot(FVector(1024,1024,1024)))*63480 + GridCoord.FlatIndex;
	float SpawnScale = Data.ScaleSpawn;

	TArray<UWorldScape_GridDataAsset*> GridDataAssets;
	for ( const auto& Asset : Data.SpawnDataAsset )
	{
		if ( UWorldScape_GridDataAsset* Loaded = Asset.LoadSynchronous() )
		{
			GridDataAssets.Add(Loaded);
		}
	}
	
	SpawnScale *= Grid_GlobalSpawnScale;
	if ( GridDataAssets.IsEmpty() )
	{
		return;
	}
	if ( SpawnScale > 0.0f && !GridDataAssets.IsEmpty() )
	{
		// Make Zone Data
		TArray<FWS_Grid_InvalidZone> ZoneData = Grid_MakeInvalidZoneData( Data.Position , SpawnSize , Grid_InvalidVolume );
		const FWS_VolumeData WSVolume = Grid_GetWSVolume( GridCoord );
		
		for ( int32 X = 0; X < GridDataAssets.Num(); X++ )
		{
			if ( !GridDataAssets[X] )
			{
				continue;
			}
			if ( !GridDataAssets[X]->Enabled )
			{
				continue;
			}
			if ( UWorld* World = GetWorld() )
			{
				if ( World->GetNetMode() == NM_DedicatedServer && !GridDataAssets[X]->SpawnableOnDedicated )
				{
					continue;
				}
			}
			int32 SeedInstance = BaseSeed + (X*X*X) + GridDataAssets[X]->SeedOffset ;
			if ( FWS_Grid_Container* Cell = Loaded_Cell.Find(GridCoord.Hash) )
			{
				(*Cell).OnPendingLoad += 1;
				const int32 TestSpawn = GridDataAssets[X]->Max_TestToSpawn*SpawnScale;
				const int32 MaxSpawn = GridDataAssets[X]->Max_Spawn*SpawnScale;
				FWS_Grid DerivedCoord = GridCoord;
				DerivedCoord.CellCenterLocation = Data.Position;

				if ( Grid_UseThread )
				{
					switch ( GridDataAssets[X]->SpawnType )
				{
				case :: Actor:
					{
						const FWS_Grid_SpawnConf SpawnConf = FWS_Grid_SpawnConf(GridDataAssets[X]->SamplingType,SeedInstance,DerivedCoord,GridDataAssets[X]->Collection_ProxyData,GridDataAssets[X]->SpawnParams,GridDataAssets[X]->Z_Offset,TestSpawn,MaxSpawn,ZoneData,SpawnSize);
						FWS_Grid_Queue LocalData = FWS_Grid_Queue(GridDataAssets[X]->SpawnType,SpawnConf);
						LocalData.Add_Actor(GridDataAssets[X]->ActorsDatas);
						LocalData.SetWS_Volume(WSVolume);
						Grid_Thread_AddPending(LocalData);
						break;
					}
				case :: StaticMesh:
					{
						const FWS_Grid_SpawnConf SpawnConf = FWS_Grid_SpawnConf(GridDataAssets[X]->SamplingType,SeedInstance,DerivedCoord,GridDataAssets[X]->Collection_ProxyData,GridDataAssets[X]->SpawnParams,GridDataAssets[X]->Z_Offset,TestSpawn,MaxSpawn,ZoneData,SpawnSize);
						FWS_Grid_Queue LocalData = FWS_Grid_Queue(GridDataAssets[X]->SpawnType,SpawnConf);
						LocalData.Add_StaticMesh(GridDataAssets[X]->StaticMeshDatas);
						LocalData.SetWS_Volume(WSVolume);
						Grid_Thread_AddPending(LocalData);
						break;
					}
				case :: Collection:
					{
						const FWS_Grid_SpawnConf SpawnConf = FWS_Grid_SpawnConf(GridDataAssets[X]->SamplingType,SeedInstance,DerivedCoord,GridDataAssets[X]->Collection_ProxyData,GridDataAssets[X]->SpawnParams,GridDataAssets[X]->Z_Offset,TestSpawn,MaxSpawn,ZoneData,SpawnSize);
						FWS_Grid_Queue LocalData = FWS_Grid_Queue(GridDataAssets[X]->SpawnType,SpawnConf);
						LocalData.Add_CollectionActor(GridDataAssets[X]->Collection_ActorsDatas);
						LocalData.Add_StaticMesh(GridDataAssets[X]->Collection_StaticMeshDatas);
						LocalData.SetWS_Volume(WSVolume);
						Grid_Thread_AddPending(LocalData);
						break;
					}
				case :: CollectionStaticMesh:
					{
						const FWS_Grid_SpawnConf SpawnConf = FWS_Grid_SpawnConf(GridDataAssets[X]->SamplingType,SeedInstance,DerivedCoord,GridDataAssets[X]->Collection_ProxyData,GridDataAssets[X]->SpawnParams,GridDataAssets[X]->Z_Offset,TestSpawn,MaxSpawn,ZoneData,SpawnSize);
						FWS_Grid_Queue LocalData = FWS_Grid_Queue(GridDataAssets[X]->SpawnType,SpawnConf);
						LocalData.Add_StaticMesh(GridDataAssets[X]->Collection_StaticMeshDatas);
						LocalData.SetWS_Volume(WSVolume);
						Grid_Thread_AddPending(LocalData);
						break;
					}
				case :: Foliage:
					{
						const FWS_Grid_SpawnConf SpawnConf = FWS_Grid_SpawnConf(GridDataAssets[X]->SamplingType,SeedInstance,DerivedCoord,GridDataAssets[X]->Collection_ProxyData,GridDataAssets[X]->SpawnParams,GridDataAssets[X]->Z_Offset,TestSpawn,MaxSpawn,ZoneData,SpawnSize);
						FWS_Grid_Queue LocalData = FWS_Grid_Queue(GridDataAssets[X]->SpawnType,SpawnConf);
						LocalData.Add_StaticMesh(GridDataAssets[X]->StaticMeshDatas);
						LocalData.SetFoliageSectorSize(GridDataAssets[X]->FoliageSectorSize);
						LocalData.SetWS_Volume(WSVolume);
						Grid_Thread_AddPending(LocalData);
						break;
					}
				case :: DataAsset:
					{
						if ( !GridDataAssets[X]->Spawners.IsEmpty() )
						{
							for ( const auto& LocalData : GridDataAssets[X]->Spawners )
							{
								if ( LocalData.GridDataAsset->Enabled && ( LocalData.GridDataAsset->SpawnType == EWS_Grid_SpawnType::Actor || LocalData.GridDataAsset->SpawnType == EWS_Grid_SpawnType::StaticMesh || LocalData.GridDataAsset->SpawnType == EWS_Grid_SpawnType::Collection || LocalData.GridDataAsset->SpawnType == EWS_Grid_SpawnType::CollectionStaticMesh ) )
								{
									if ( float LocalScale = SpawnScale * LocalData.SpawnScale > 0.0f )
									{
										(*Cell).OnPendingLoad += 1;
										int32 LocalSeed = SeedInstance + LocalData.GridDataAsset->SeedOffset;
										LocalSeed += 1;
										const int32 LocalTestSpawn = LocalData.GridDataAsset->Max_TestToSpawn*SpawnScale;
										const int32 LocalMaxSpawn = LocalData.GridDataAsset->Max_Spawn*SpawnScale;
										const FWS_Grid_SpawnConf SpawnConf = FWS_Grid_SpawnConf(LocalData.GridDataAsset->SamplingType,LocalSeed,DerivedCoord,LocalData.GridDataAsset->Collection_ProxyData,LocalData.GridDataAsset->SpawnParams,LocalData.GridDataAsset->Z_Offset,LocalTestSpawn,LocalMaxSpawn,ZoneData,SpawnSize);
										WS_Grid_DirectSpawnDataAsset( SpawnConf , LocalData.GridDataAsset , WSVolume);
									}
								}
							}
						}
						break;
					}
				case :: OnlyData:
					{
						break;
					}
				}
					
				}
				else
				{
					switch ( GridDataAssets[X]->SpawnType )
				{
				case :: Actor:
					{
						const FWS_Grid_SpawnConf SpawnConf = FWS_Grid_SpawnConf(GridDataAssets[X]->SamplingType,SeedInstance,DerivedCoord,GridDataAssets[X]->Collection_ProxyData,GridDataAssets[X]->SpawnParams,GridDataAssets[X]->Z_Offset,TestSpawn,MaxSpawn,ZoneData,SpawnSize);
						Grid_ComputeSpawn_Actor( SpawnConf , GridDataAssets[X]->ActorsDatas ,WSVolume);
						break;
					}
				case :: StaticMesh:
					{
						const FWS_Grid_SpawnConf SpawnConf = FWS_Grid_SpawnConf(GridDataAssets[X]->SamplingType,SeedInstance,DerivedCoord,GridDataAssets[X]->Collection_ProxyData,GridDataAssets[X]->SpawnParams,GridDataAssets[X]->Z_Offset,TestSpawn,MaxSpawn,ZoneData,SpawnSize);
						Grid_ComputeSpawn_StaticMesh( SpawnConf , GridDataAssets[X]->StaticMeshDatas ,WSVolume);
						break;
					}
				case :: Collection:
					{
						const FWS_Grid_SpawnConf SpawnConf = FWS_Grid_SpawnConf(GridDataAssets[X]->SamplingType,SeedInstance,DerivedCoord,GridDataAssets[X]->Collection_ProxyData,GridDataAssets[X]->SpawnParams,GridDataAssets[X]->Z_Offset,TestSpawn,MaxSpawn,ZoneData,SpawnSize);
						Grid_ComputeSpawn_Collection( SpawnConf , GridDataAssets[X]->Collection_ActorsDatas , GridDataAssets[X]->Collection_StaticMeshDatas ,WSVolume);
						break;
					}
				case :: CollectionStaticMesh:
					{
						const FWS_Grid_SpawnConf SpawnConf = FWS_Grid_SpawnConf(GridDataAssets[X]->SamplingType,SeedInstance,DerivedCoord,GridDataAssets[X]->Collection_ProxyData,GridDataAssets[X]->SpawnParams,GridDataAssets[X]->Z_Offset,TestSpawn,MaxSpawn,ZoneData,SpawnSize);
						Grid_ComputeSpawn_CollectionStaticMesh( SpawnConf , GridDataAssets[X]->Collection_StaticMeshDatas ,WSVolume);
						break;
					}
				case :: Foliage:
					{
						const FWS_Grid_SpawnConf SpawnConf = FWS_Grid_SpawnConf(GridDataAssets[X]->SamplingType,SeedInstance,DerivedCoord,GridDataAssets[X]->Collection_ProxyData,GridDataAssets[X]->SpawnParams,GridDataAssets[X]->Z_Offset,TestSpawn,MaxSpawn,ZoneData,SpawnSize);
						Grid_ComputeSpawn_Foliage( SpawnConf  , GridDataAssets[X]->StaticMeshDatas , GridDataAssets[X]->FoliageSectorSize ,WSVolume);
						break;
					}
				case :: DataAsset:
					{
						if ( !GridDataAssets[X]->Spawners.IsEmpty() )
						{
							for ( const auto& LocalData : GridDataAssets[X]->Spawners )
							{
								if ( LocalData.GridDataAsset->Enabled && ( LocalData.GridDataAsset->SpawnType == EWS_Grid_SpawnType::Actor || LocalData.GridDataAsset->SpawnType == EWS_Grid_SpawnType::StaticMesh || LocalData.GridDataAsset->SpawnType == EWS_Grid_SpawnType::Collection || LocalData.GridDataAsset->SpawnType == EWS_Grid_SpawnType::CollectionStaticMesh ) )
								{
									if ( float LocalScale = SpawnScale * LocalData.SpawnScale > 0.0f )
									{
										(*Cell).OnPendingLoad += 1;
										int32 LocalSeed = SeedInstance + LocalData.GridDataAsset->SeedOffset;
										LocalSeed += 1;
										const int32 LocalTestSpawn = LocalData.GridDataAsset->Max_TestToSpawn*SpawnScale;
										const int32 LocalMaxSpawn = LocalData.GridDataAsset->Max_Spawn*SpawnScale;
										const FWS_Grid_SpawnConf SpawnConf = FWS_Grid_SpawnConf(LocalData.GridDataAsset->SamplingType,LocalSeed,DerivedCoord,LocalData.GridDataAsset->Collection_ProxyData,LocalData.GridDataAsset->SpawnParams,LocalData.GridDataAsset->Z_Offset,LocalTestSpawn,LocalMaxSpawn,ZoneData,SpawnSize);
										WS_Grid_DirectSpawnDataAsset( SpawnConf , LocalData.GridDataAsset , WSVolume);
									}
								}
							}
						}
						break;
					}
				case :: OnlyData:
					{
						break;
					}
				}
				}
			}
		}
		Grid_Thread_CheckAndMake();
	}
}
// --------------------------------------
void AWorldScapeRoot::Grid_CheckVolumeDataToLoad( const FWS_Grid& GridCoord )
{
	if ( Loaded_Cell.Contains(GridCoord.Hash) )
	{
		if ( FWS_Grid_SpawnerArray* Index = Grid_SpawnerVolumes.Find(GridCoord.Hash) )
		{
			for ( const auto& Spawn : (*Index).Spawners )
			{
				Grid_SpawnByVolume( GridCoord , Spawn.Value );
			}
		}
	}
}
// --------------------------------------
void AWorldScapeRoot::Grid_ShuffleFromStream(int32 ArraySize, TArray<int32>& ShuffledIndexes, FRandomStream& RandomStream)
{
	ShuffledIndexes.Reset();
	for (int32 i = 0; i < ArraySize; i++)
	{
		ShuffledIndexes.Add(i);
	}
	
	for (int32 i = ShuffledIndexes.Num() - 1; i > 0; --i)
	{
		int32 SwapIndex = RandomStream.RandRange(0, i);
		ShuffledIndexes.Swap(i, SwapIndex);
	}
}
// --------------------------------------
bool AWorldScapeRoot::Grid_SerializeActor( AActor* Actor , TArray<uint8>& Serialized )
{
	if ( IsValid(Actor) )
	{
		FMemoryWriter MemoryWriter( Serialized , false);
		MemoryWriter << Actor;
		FString ActorClass = Actor->GetClass()->GetPathName();
		MemoryWriter << ActorClass;
		return true;
	}
	return false;
}
// --------------------------------------
bool AWorldScapeRoot::Grid_UnSerializeActor( AActor*& Actor , const TArray<uint8>& Serialized )
{
	if ( !Serialized.IsEmpty() )
	{
		if ( UWorld* World = GetWorld() )
		{
			FMemoryReader MemoryReader( Serialized , false);
			FString ActorClass;
			MemoryReader << ActorClass;
			if ( UClass* LoadedClassActor = FindObject<UClass>(this, *ActorClass) )
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.ObjectFlags = RF_Transient;
				SpawnParams.Owner = this ;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				AActor* SpawnedActor = World->SpawnActor<AActor>(LoadedClassActor,SpawnParams);
				if ( IsValid(SpawnedActor) )
				{
					MemoryReader << SpawnedActor;
					SpawnedActor->RegisterAllComponents();
					Actor = SpawnedActor;
					return true;
				}
			}
		}
	}
	return false;
}
// --------------------------------------
bool AWorldScapeRoot::Grid_StorageActors( TArray<AActor*>& Actors , TMap<int64,FWS_Grid_SerializedData>& StorageData )
{
	for ( auto& SerActor : Actors )
	{
		if ( IsValid(SerActor) )
		{
			const FWS_Grid GridCoord = FWS_Grid( GetActorLocation() , SerActor->GetActorLocation() , Grid_Size , PlanetScale );
			const FWS_Grid SaveGridCoord = FWS_Grid( GetActorLocation() , SerActor->GetActorLocation() , Grid_Data_Size , PlanetScale );
			TArray<uint8> Serialized;
			if ( Grid_SerializeActor( SerActor , Serialized ) )
			{
				if ( FWS_Grid_SerializedData* LockData = StorageData.Find(SaveGridCoord.Hash) )
				{
					(*LockData).AddData(GridCoord.Hash, Serialized);
				}
				else
				{
					StorageData.Add(SaveGridCoord.Hash,FWS_Grid_SerializedData(GridCoord.Hash, Serialized));
				}
			}
		}
	}
	return !StorageData.IsEmpty();
}
// --------------------------------------
void AWorldScapeRoot::Grid_UpdateStorage( const TArray<FWS_Grid>& Coords )
{
	if ( Grid_CellData )
	{
		if ( !Grid_CellData->Grid_StorageData.IsEmpty() )
		{
			TArray<FWS_Grid> Grids;
			TArray<int64> Loaded;
	
			for ( const auto& Loc : Coords )
			{
				const FWS_Grid SaveGridCoord = FWS_Grid( GetActorLocation() , Loc.CellCenterLocation , Grid_Data_Size , PlanetScale );
				if ( !Grid_LoadedStorage.Contains(SaveGridCoord.Hash) )
				{
					Loaded.Add( SaveGridCoord.Hash );
					if ( TSoftObjectPtr<UWorldScape_GridSerializedData>* Data = Grid_CellData->Grid_StorageData.Find(SaveGridCoord.Hash) )
					{
						UWorldScape_GridSerializedData* LoadedStorage = (*Data).LoadSynchronous();
						if ( LoadedStorage )
						{
							Grid_LoadedStorage.Add(SaveGridCoord.Hash,FWS_Grid_StorageData(LoadedStorage->Storage));
						}
					}
				}
				else
				{
					Grids.Add(SaveGridCoord);
				}
			}
			for ( auto& Cell : Grid_LoadedStorage )
			{
				if ( !Loaded.Contains(Cell.Key) )
				{
					Grid_LoadedStorage.Remove(Cell.Key);
				}
			}
		}
		for ( const auto& Loc : Coords )
		{
			for ( auto& StorageData : Grid_LoadedStorage )
			{
				if ( !StorageData.Value.Loaded.Contains(Loc.Hash) && StorageData.Value.Datas.Datas.Contains(Loc.Hash) )
				{
					if ( FWS_Grid_Container* Cell = Loaded_Cell.Find(Loc.Hash) )
					{
						if ( FWS_Grid_SerializedActor_Array* DatasLoad = StorageData.Value.Datas.Datas.Find(Loc.Hash) )
						{
							for ( const auto& Actor : (*DatasLoad).Datas )
							{
								AActor* Spawned;
								if ( Grid_UnSerializeActor(Spawned,Actor.Data) )
								{
									if ( IsValid(Spawned) )
									{
										(*Cell).Instances.Add(Spawned);
									}
								}
							}
						}
					}
				}
			}
		}
	}
}
// --------------------------------------
void AWorldScapeRoot::Clear_Spawned()
{
	Grid_DestroyAllCell();
}
// --------------------------------------
int32 AWorldScapeRoot::Grid_Compute_ActorMeshIndex( FRandomStream& Random , const TArray<FWS_Grid_Actor_Data>& Actors )
{
	if ( !Actors.IsEmpty() )
	{
		if ( Actors.Num() == 1 )
		{
			if ( UKismetMathLibrary::RandomFloatInRangeFromStream(Random,0.0f,1.0f)  < Actors[0].SpawnableChance )
			{
				return 0;
			}
		}
		else
		{
			for ( int32 y = 0 ; y < Actors.Num() ; y++ )
			{
				const int32 Index = UKismetMathLibrary::RandomIntegerFromStream(Random,Actors.Num()-1);
				if ( UKismetMathLibrary::RandomFloatInRangeFromStream(Random,0.0f,1.0f)  < Actors[Index].SpawnableChance )
				{
					return Index;
				}
			}
		}
	}
	return -1;
}
// --------------------------------------
int32 AWorldScapeRoot::Grid_Compute_StaticMeshIndex( FRandomStream& Random , const TArray<FWS_Grid_StaticMesh_Data>& StaticMesh )
{
	if ( !StaticMesh.IsEmpty() )
	{
		if ( StaticMesh.Num() == 1 )
		{
			if ( UKismetMathLibrary::RandomFloatInRangeFromStream(Random,0.0f,1.0f) < StaticMesh[0].SpawnableChance )
			{
				return 0;
			}
		}
		else
		{
			for ( int32 j = 0 ; j < StaticMesh.Num() ; j++ )
			{
				const int32 Index = UKismetMathLibrary::RandomIntegerFromStream(Random,StaticMesh.Num()-1);
				if ( StaticMesh.IsValidIndex(Index) )
				{
					if ( UKismetMathLibrary::RandomFloatInRangeFromStream(Random,0.0f,1.0f) < StaticMesh[Index].SpawnableChance )
					{
						return Index;
					}
				}
			}
		}
	}
	return -1;
}
// --------------------------------------
void AWorldScapeRoot::Grid_Thread_MakePool()
{
	/*
	if ( Grid_ThreadPool.IsEmpty() )
	{
		Grid_ThreadValidation = true;
		for ( int32 i = 0 ; i < Grid_NumberOfThread ; i++ )
		{
			FAsyncTask<GridGenerationThread>* GridThreadTask = new FAsyncTask<GridGenerationThread>(this,Queue_AsyncCompute,Grid_ThreadValidation,Queue_Spawn);
			Grid_ThreadPool.Add(GridThreadTask);
		}
	}
	*/
	if ( ArrayQueue_AsyncCompute.IsEmpty() )
    	{
    		Grid_ThreadValidation = true;
    		for ( int32 i = 0 ; i < Grid_NumberOfThread ; i++ )
    		{
    			ThreadValidation.AddDefaulted();
    			ArrayQueue_AsyncCompute.AddDefaulted();
    			FAsyncTask<GridGenerationThread>* Thread = new FAsyncTask<GridGenerationThread>(this,ArrayQueue_AsyncCompute.Last(),ThreadValidation.Last(),Queue_Spawn);
    			Thread->SetPriority(Grid_ThreadPriority);
    			Grid_ThreadPool.Add(Thread);
    		}
    	}
	if ( Grid_Debug )
	{
		UE_LOG(LogTemp, Log, TEXT("WS Grid : Thread created ") );
	}
}
// --------------------------------------
void AWorldScapeRoot::Grid_Thread_StopThreadAndClearQueue( const bool Force , const bool Clear )
{
	/*
	Grid_ThreadValidation = false;
	if ( Clear )
	{
		Queue_AsyncCompute.Empty();
	}
	for ( auto& ThreadTask : Grid_ThreadPool )
	{
		if ( ThreadTask != nullptr )
		{
			if (ThreadTask->Cancel() || ThreadTask->IsDone() )
			{
				delete ThreadTask;
			}
			else
			{
				if ( ThreadTask->TryAbandonTask() )
				{
					delete ThreadTask;
				}
				if (ThreadTask->Cancel() || ThreadTask->IsDone() )
				{
					delete ThreadTask;
				}
				else
				{
					if ( Force )
					{
						ThreadTask->EnsureCompletion();
						delete ThreadTask;
					}
					else
					{
						continue;
					}
				}
			}
		}
	}
	Grid_ThreadPool.Empty();
	*/
	Grid_ThreadValidation = false;
	if ( Clear )
	{
		Queue_AsyncCompute.Empty();
	}

	for ( auto& Valid : ThreadValidation )
	{
		Valid = false;
	}
	for ( int32 i = 0 ; i < Grid_ThreadPool.Num() ; i++ )
	{
		if ( Grid_ThreadPool[i] != nullptr )
		{
			if (Grid_ThreadPool[i]->Cancel() || Grid_ThreadPool[i]->IsDone() )
			{
				delete Grid_ThreadPool[i];
			}
			else
			{
				if ( Grid_ThreadPool[i]->TryAbandonTask() )
				{
					delete Grid_ThreadPool[i];
				}
				if (Grid_ThreadPool[i]->Cancel() || Grid_ThreadPool[i]->IsDone() )
				{
					delete Grid_ThreadPool[i];
				}
				else
				{
					if ( Force )
					{
						Grid_ThreadPool[i]->EnsureCompletion();
						delete Grid_ThreadPool[i];
					}
					else
					{
						continue;
					}
				}
			}
		}
	}
	Grid_ThreadPool.Empty();
	ArrayQueue_AsyncCompute.Empty();
	ThreadValidation.Empty();
	if ( Grid_Debug )
	{
		UE_LOG(LogTemp, Log, TEXT("WS Grid : Threads Kill"));
	}
}
// --------------------------------------
bool AWorldScapeRoot::Grid_Thread_CheckIsDone()
{
	/*
	for ( auto& ThreadTask : Grid_ThreadPool )
	{
		if ( ThreadTask != nullptr )
		{
			if ( !ThreadTask->Cancel() || !ThreadTask->IsDone() )
			{
				return false;
			}
		}
	}
	return true;
	*/
	for ( auto& ThreadTask : Grid_ThreadPool )
	{
		if ( ThreadTask != nullptr )
		{
			if ( !ThreadTask->Cancel() || !ThreadTask->IsDone() )
			{
				return false;
			}
		}
	}
	return true;
}
// --------------------------------------
void AWorldScapeRoot::Grid_Thread_AddPending( const FWS_Grid_Queue& Data , const bool Temp )
{
	/*
	Queue_AsyncCompute.Enqueue(Data);
	*/

	if ( Temp )
	{
		TempsQueue.Add( Data );
	}
	else
	{
		if ( Grid_ThreadPool.IsEmpty() )
		{
			Grid_Thread_MakePool();
		}
	
		Grid_FlipNext();
		if ( Grid_ThreadPool.IsValidIndex(Grid_FlipPending) && ArrayQueue_AsyncCompute.IsValidIndex(Grid_FlipPending) && ThreadValidation[Grid_FlipPending] )
		{
			if ( ThreadValidation[Grid_FlipPending] )
			{
				ArrayQueue_AsyncCompute[Grid_FlipPending].Enqueue(Data);
			}
			else
			{
				for ( int32 i = 0 ; i < Grid_ThreadPool.Num() ; i++ )
				{
					Grid_FlipNext();
					if ( ThreadValidation[Grid_FlipPending] )
					{
						ArrayQueue_AsyncCompute[Grid_FlipPending].Enqueue(Data);
						return;
					}
				}
			}
		}
	}
}
// --------------------------------------
void AWorldScapeRoot::Grid_Thread_CheckAndMake()
{
	/*
	if ( Grid_ThreadPool.IsEmpty() && !Queue_AsyncCompute.IsEmpty() && Grid_UseThread )
	{
		Grid_Thread_MakePool();
		for ( auto& ThreadTask : Grid_ThreadPool )
		{
			if ( ThreadTask )
			{
				Grid_ThreadValidation = true;
				ThreadTask->StartBackgroundTask();
				Grid_TimeCounter = 0.0f;
			}
		}
	}
	else if ( !Grid_ThreadPool.IsEmpty() && !Queue_AsyncCompute.IsEmpty() && Grid_UseThread )
	{
		for ( auto& ThreadTask : Grid_ThreadPool )
		{
			if ( ThreadTask )
			{
				if ( ThreadTask->IsDone() )
				{
					Grid_ThreadValidation = true;
					ThreadTask->StartBackgroundTask();
					Grid_TimeCounter = 0.0f;
				}
			}
		}
	}
	*/
	if ( Grid_ThreadPool.IsEmpty() )
	{
		Grid_Thread_MakePool();
		for ( auto& ThreadTask : Grid_ThreadPool )
		{
			if ( ThreadTask )
			{
				Grid_ThreadValidation = true;
				ThreadTask->StartBackgroundTask( GThreadPool, Grid_ThreadPriority );
				Grid_TimeCounter = 0.0f;
			}
		}
	}
}
// --------------------------------------
FWS_VolumeData AWorldScapeRoot::Grid_GetWSVolume( const FWS_Grid& GridCoord )
{
	const FBox CoordBox = FBox(GridCoord.SizeData.A,GridCoord.SizeData.C);
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