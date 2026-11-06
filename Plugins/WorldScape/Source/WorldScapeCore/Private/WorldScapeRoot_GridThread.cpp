// Copyright 2024 IOLACORP STUDIO. All Rights Reserved

#include "WorldScapeRoot.h"
#include "Async/ParallelFor.h"

// ----------------------------------------------------
void GridGenerationThread::DoWork()
{
	if ( !bShouldRun ) {return;}
	FWS_Grid_Queue Local;
	if (TaskQueue.Dequeue(Local))
	{
		if ( IsValid(RootRef) )
		{
			if ( !bShouldRun || !IsValid(RootRef) ) {return;}
			//if ( RootRef->Loaded_Cell.Contains(Local.SpawnConf.GridCoord.Hash) )
			if ( true )
			{
				switch ( Local.SpawnType )
				{
				case :: Actor:
					{
						GridThread_ComputeSpawn_Actor( Local.SpawnConf , Local.ActorData , Local.WS_Volume );
						break;
					}
				case :: StaticMesh:
					{
						GridThread_ComputeSpawn_StaticMesh( Local.SpawnConf , Local.StaticMeshData , Local.WS_Volume);
						break;
					}
				case :: Collection:
					{
						GridThread_ComputeSpawn_Collection( Local.SpawnConf , Local.CollectionActor , Local.StaticMeshData , Local.WS_Volume);
						break;
					}
				case :: CollectionStaticMesh:
					{
						GridThread_ComputeSpawn_CollectionStaticMesh( Local.SpawnConf , Local.StaticMeshData , Local.WS_Volume);
						break;
					}
				case :: Foliage:
					{
						GridThread_ComputeSpawn_Foliage( Local.SpawnConf  , Local.StaticMeshData , Local.FoliageSectorSize , Local.WS_Volume);
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
	}
}
// ----------------------------------------------------
void GridGenerationThread::GridThread_ComputeSpawn_Actor( const FWS_Grid_SpawnConf& SpawnConf , const TArray<FWS_Grid_Spawner_Actor>& Actors , const FWS_VolumeData& WSVolume)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(GridThread_ComputeSpawn_Actor);
	FRandomStream Random = FRandomStream(SpawnConf.Seed);
		int32 SpawnCount = 0;
		TArray<FVector> SpawnLocations;
		if ( IsValid(RootRef) )
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
						if ( !bShouldRun || !IsValid(RootRef) ) {return;}
						FVector TestLocation;
						if ( RootRef->WS_Grid_SearchPointOnDisk( SpawnConf.ZoneData , TestLocation , Random , SpawnConf.SpawnParams , SpawnLocations , SpawnConf.GridCoord.CellCenterLocation , SpawnConf.GridCoord.CellCenterNormal , SpawnConf.ZoneSize ,WSVolume ) )
						{
							FTransform Local;
							if ( !bShouldRun || !IsValid(RootRef) ) {return;}
							if ( RootRef->WS_Grid_SnapPoint( Local ,Random,SpawnConf.SpawnParams, TestLocation , SpawnConf.GridCoord.CellCenterNormal , SpawnConf.Z_Offset , WSVolume) )
							{
								SpawnLocations.Add(Local.GetLocation());
						
								for ( int32 y = 0 ; y < Actors.Num() ; y++ )
								{
									const int32 Index = UKismetMathLibrary::RandomIntegerFromStream(Random,Actors.Num()-1);
									if ( UKismetMathLibrary::RandomFloatInRangeFromStream(Random,0.0f,1.0f)  < Actors[Index].SpawnableChance )
									{
										if ( Actors.IsValidIndex(Index) )
										{
											SpawnQueue.Enqueue(FWS_Grid_SpawnPending( SpawnConf.GridCoord,Local,Actors[Index].ProxyData,{FWS_Grid_Actor_SpawnData(Actors[Index].Class,Local,Actors[Index])},TArray<FWS_Grid_Static_SpawnData>(),false,SpawnConf.Seed+(SpawnCount*1000000)));
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
					const FTransform Base = FTransform(FRotationMatrix::MakeFromZX(SpawnConf.GridCoord.CellCenterNormal,SpawnConf.GridCoord.SizeData.XAxis).ToQuat() ,SpawnConf.GridCoord.CellCenterLocation);
					const int32 SubSpawnCount = SpawnConf.Max_Spawn/9;
					const TArray<FVector> SubLocationSpawnGrid =  RootRef->WS_Grid_GenerateZoneGrid(Base,SpawnConf.GridCoord.SizeData.CircleInternalSize,3);
					const double SubSize = SpawnConf.GridCoord.SizeData.CircleInternalSize/3;
					ParallelFor(SubLocationSpawnGrid.Num(), [&](int32 Index)
					{
						const TArray<FVector> PosPoisson = RootRef->WS_Grid_SearchPoissonDisk( SpawnConf.ZoneData , SubSpawnCount , Random , SpawnConf.SpawnParams , SubLocationSpawnGrid[Index] , SubSize , WSVolume);
						for ( const auto& TestLocation : PosPoisson )
						{
							FTransform Local;
							if ( !bShouldRun || !IsValid(RootRef) ) {return;}
							if ( RootRef->WS_Grid_SnapPoint( Local ,Random,SpawnConf.SpawnParams, TestLocation , SpawnConf.GridCoord.CellCenterNormal , SpawnConf.Z_Offset , WSVolume) )
							{
								for ( int32 y = 0 ; y < Actors.Num() ; y++ )
								{
									const int32 IndexSM = UKismetMathLibrary::RandomIntegerFromStream(Random,Actors.Num()-1);
									if ( UKismetMathLibrary::RandomFloatInRangeFromStream(Random,0.0f,1.0f)  < Actors[IndexSM].SpawnableChance )
									{
										if ( Actors.IsValidIndex(IndexSM) )
										{
											SpawnQueue.Enqueue(FWS_Grid_SpawnPending( SpawnConf.GridCoord,Local,Actors[IndexSM].ProxyData,{FWS_Grid_Actor_SpawnData(Actors[IndexSM].Class,Local,Actors[IndexSM])},TArray<FWS_Grid_Static_SpawnData>(),false,SpawnConf.Seed+(SpawnCount*1000000)));
											SpawnCount += 1;
											if ( SpawnCount >= SubSpawnCount )
											{
												return;
											}
											break;
										}
									}
								}
							}
						}
					});
					break;
				}
			case ::Grid:
				{
					const FTransform Base = FTransform(FRotationMatrix::MakeFromZX(SpawnConf.GridCoord.CellCenterNormal,SpawnConf.GridCoord.SizeData.XAxis).ToQuat() ,SpawnConf.GridCoord.CellCenterLocation);
					const int32 SubSpawnCount = SpawnConf.Max_Spawn/9;
					const TArray<FVector> SubLocationSpawnGrid =  RootRef->WS_Grid_GenerateZoneGrid(Base,SpawnConf.GridCoord.SizeData.CircleInternalSize,3);
					const double SubSize = SpawnConf.GridCoord.SizeData.CircleInternalSize/3;
					ParallelFor(SubLocationSpawnGrid.Num(), [&](int32 Index)
					{
						const TArray<FVector> PosGrid = RootRef->WS_Grid_SearchGridSampling(SpawnConf.ZoneData , SubSpawnCount , Random , SpawnConf.SpawnParams , FTransform(Base.Rotator().Quaternion(),SubLocationSpawnGrid[Index]) , SubSize, WSVolume);
						for ( const auto& TestLocation : PosGrid )
						{
							FTransform Local;
							if ( !bShouldRun || !IsValid(RootRef) ) {return;}
							if ( RootRef->WS_Grid_SnapPoint( Local ,Random,SpawnConf.SpawnParams, TestLocation , SpawnConf.GridCoord.CellCenterNormal , SpawnConf.Z_Offset, WSVolume ) )
							{
								for ( int32 y = 0 ; y < Actors.Num() ; y++ )
								{
									const int32 IndexSM = UKismetMathLibrary::RandomIntegerFromStream(Random,Actors.Num()-1);
									if ( UKismetMathLibrary::RandomFloatInRangeFromStream(Random,0.0f,1.0f)  < Actors[IndexSM].SpawnableChance )
									{
										if ( Actors.IsValidIndex(IndexSM) )
										{
											SpawnQueue.Enqueue(FWS_Grid_SpawnPending( SpawnConf.GridCoord,Local,Actors[IndexSM].ProxyData,{FWS_Grid_Actor_SpawnData(Actors[IndexSM].Class,Local,Actors[IndexSM])},TArray<FWS_Grid_Static_SpawnData>(),false,SpawnConf.Seed+(SpawnCount*1000000)));
											SpawnCount += 1;
											if ( SpawnCount >= SubSpawnCount )
											{
												return;
											}
											break;
										}
									}
								}
							}
						}
					});
					break;
				}
			}
		}
}
// ----------------------------------------------------
void GridGenerationThread::GridThread_ComputeSpawn_StaticMesh( const FWS_Grid_SpawnConf& SpawnConf  , const TArray<FWS_Grid_StaticMesh_Data>& StaticMesh , const FWS_VolumeData& WSVolume)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(GridThread_ComputeSpawn_StaticMesh);
	FRandomStream Random = FRandomStream(SpawnConf.Seed);
		int32 SpawnCount = 0;
		TArray<FVector> SpawnLocations;
		if ( IsValid(RootRef) )
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
						if ( !bShouldRun || !IsValid(RootRef) ) {return;}
						FVector TestLocation;
						if ( RootRef->WS_Grid_SearchPointOnDisk( SpawnConf.ZoneData , TestLocation , Random , SpawnConf.SpawnParams , SpawnLocations , SpawnConf.GridCoord.CellCenterLocation , SpawnConf.GridCoord.CellCenterNormal , SpawnConf.ZoneSize , WSVolume) )
						{
							if ( !bShouldRun || !IsValid(RootRef) ) {return;}
							FTransform Local;
							if ( RootRef->WS_Grid_SnapPoint( Local ,Random,SpawnConf.SpawnParams, TestLocation , SpawnConf.GridCoord.CellCenterNormal , SpawnConf.Z_Offset, WSVolume ) )
							{
								SpawnLocations.Add(Local.GetLocation());
								for ( int32 j = 0 ; j < StaticMesh.Num() ; j++ )
								{
									const int32 Index = UKismetMathLibrary::RandomIntegerFromStream(Random,StaticMesh.Num()-1);
									if ( UKismetMathLibrary::RandomFloatInRangeFromStream(Random,0.0f,1.0f) < StaticMesh[Index].SpawnableChance )
									{
										if ( !bShouldRun || !IsValid(RootRef) ) {return;}
										SpawnQueue.Enqueue(FWS_Grid_SpawnPending(SpawnConf.GridCoord,Local,SpawnConf.Proxy,TArray<FWS_Grid_Actor_SpawnData>(), {FWS_Grid_Static_SpawnData(StaticMesh[Index].StaticMesh,RootRef->WS_Grid_ComputePlacement(Random,Local,StaticMesh[Index].MeshBounds,StaticMesh[Index].Placement, WSVolume),StaticMesh[Index].Collision,StaticMesh[Index].CastShadow)} ,false,SpawnConf.Seed+(SpawnCount*1000000)));
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
					break;
				}
			case ::PoissonDisk:
				{
					const FTransform Base = FTransform(FRotationMatrix::MakeFromZX(SpawnConf.GridCoord.CellCenterNormal,SpawnConf.GridCoord.SizeData.XAxis).ToQuat() ,SpawnConf.GridCoord.CellCenterLocation);
					const int32 SubSpawnCount = SpawnConf.Max_Spawn/9;
					const TArray<FVector> SubLocationSpawnGrid =  RootRef->WS_Grid_GenerateZoneGrid(Base,SpawnConf.GridCoord.SizeData.CircleInternalSize,3);
					const double SubSize = SpawnConf.GridCoord.SizeData.CircleInternalSize/3;
					ParallelFor(SubLocationSpawnGrid.Num(), [&](int32 Index)
					{
						int32 LocalSpawnCount = 0;
						const TArray<FVector> PosPoisson = RootRef->WS_Grid_SearchPoissonDisk( SpawnConf.ZoneData , SubSpawnCount , Random , SpawnConf.SpawnParams , SubLocationSpawnGrid[Index] , SubSize , WSVolume);
						for ( const auto& TestLocation : PosPoisson )
						{
							if ( !bShouldRun || !IsValid(RootRef) ) {return;}
							FTransform Local;
							if ( RootRef->WS_Grid_SnapPoint( Local ,Random,SpawnConf.SpawnParams, TestLocation , SpawnConf.GridCoord.CellCenterNormal , SpawnConf.Z_Offset , WSVolume) )
							{
								for ( int32 j = 0 ; j < StaticMesh.Num() ; j++ )
								{
									const int32 IndexSM = UKismetMathLibrary::RandomIntegerFromStream(Random,StaticMesh.Num()-1);
									if ( UKismetMathLibrary::RandomFloatInRangeFromStream(Random,0.0f,1.0f) < StaticMesh[IndexSM].SpawnableChance )
									{
										if ( !bShouldRun || !IsValid(RootRef) ) {return;}
										SpawnQueue.Enqueue(FWS_Grid_SpawnPending(SpawnConf.GridCoord,Local,SpawnConf.Proxy,TArray<FWS_Grid_Actor_SpawnData>(), {FWS_Grid_Static_SpawnData(StaticMesh[IndexSM].StaticMesh,RootRef->WS_Grid_ComputePlacement(Random,Local,StaticMesh[IndexSM].MeshBounds,StaticMesh[IndexSM].Placement, WSVolume),StaticMesh[IndexSM].Collision,StaticMesh[Index].CastShadow)} ,false,SpawnConf.Seed+(SpawnCount*1000000)));
										LocalSpawnCount += 1;
										if ( LocalSpawnCount >= SubSpawnCount )
										{
											return;
										}
										break;
									}
								}
							}
						}
					});
					break;
				}
			case ::Grid:
				{
					const FTransform Base = FTransform(FRotationMatrix::MakeFromZX(SpawnConf.GridCoord.CellCenterNormal,SpawnConf.GridCoord.SizeData.XAxis).ToQuat() ,SpawnConf.GridCoord.CellCenterLocation);
					const int32 SubSpawnCount = SpawnConf.Max_Spawn/9;
					const TArray<FVector> SubLocationSpawnGrid =  RootRef->WS_Grid_GenerateZoneGrid(Base,SpawnConf.GridCoord.SizeData.CircleInternalSize,3);
					const double SubSize = SpawnConf.GridCoord.SizeData.CircleInternalSize/3;
					
					ParallelFor(SubLocationSpawnGrid.Num(), [&](int32 Index)
					{
						int32 LocalSpawnCount = 0;
						const TArray<FVector> PosGrid = RootRef->WS_Grid_SearchGridSampling(SpawnConf.ZoneData , SubSpawnCount , Random , SpawnConf.SpawnParams , FTransform(Base.Rotator().Quaternion(),SubLocationSpawnGrid[Index]) , SubSize*2, WSVolume);
						for ( const auto& TestLocation : PosGrid )
						{
							if ( !bShouldRun || !IsValid(RootRef) ) {return;}
							FTransform Local;
							if ( RootRef->WS_Grid_SnapPoint( Local ,Random,SpawnConf.SpawnParams, TestLocation , SpawnConf.GridCoord.CellCenterNormal , SpawnConf.Z_Offset , WSVolume) )
							{
								for ( int32 j = 0 ; j < StaticMesh.Num() ; j++ )
								{
									const int32 IndexSM = UKismetMathLibrary::RandomIntegerFromStream(Random,StaticMesh.Num()-1);
									if ( UKismetMathLibrary::RandomFloatInRangeFromStream(Random,0.0f,1.0f) < StaticMesh[IndexSM].SpawnableChance )
									{
										if ( !bShouldRun || !IsValid(RootRef) ) {return;}
										SpawnQueue.Enqueue(FWS_Grid_SpawnPending(SpawnConf.GridCoord,Local,SpawnConf.Proxy,TArray<FWS_Grid_Actor_SpawnData>(), {FWS_Grid_Static_SpawnData(StaticMesh[IndexSM].StaticMesh,RootRef->WS_Grid_ComputePlacement(Random,Local,StaticMesh[IndexSM].MeshBounds,StaticMesh[IndexSM].Placement, WSVolume),StaticMesh[IndexSM].Collision,StaticMesh[Index].CastShadow)} ,false,SpawnConf.Seed+(SpawnCount*1000000)));
										LocalSpawnCount += 1;
										if ( LocalSpawnCount >= SubSpawnCount )
										{
											return;
											break;
										}
										break;
									}
								}
							}
						}
					});
					break;
				}
			}
		}
}
// ----------------------------------------------------
void GridGenerationThread::GridThread_ComputeSpawn_Collection( const FWS_Grid_SpawnConf& SpawnConf  , const TArray<FWS_Grid_Actor_Data>& Actors , const TArray<FWS_Grid_StaticMesh_Data>& StaticMesh , const FWS_VolumeData& WSVolume)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(GridThread_ComputeSpawn_Collection);
	FRandomStream Random = FRandomStream(SpawnConf.Seed);
	int32 SpawnCount = 0;
	TArray<FVector> SpawnLocations;
	if ( IsValid(RootRef) )
	{
		switch (SpawnConf.SamplingType)
		{
		case ::CircleRND:
			{
				for ( int32 i = 0; i < SpawnConf.Max_TestToSpawn; i++ )
				{
					if ( !bShouldRun || !IsValid(RootRef) ) {return;}
					FVector TestLocation;
					if ( RootRef->WS_Grid_SearchPointOnDisk( SpawnConf.ZoneData , TestLocation , Random , SpawnConf.SpawnParams , SpawnLocations , SpawnConf.GridCoord.CellCenterLocation , SpawnConf.GridCoord.CellCenterNormal , SpawnConf.ZoneSize , WSVolume) )
					{
						if ( !bShouldRun || !IsValid(RootRef) ) {return;}
						FTransform Local;
						if ( RootRef->WS_Grid_SnapPoint( Local ,Random,SpawnConf.SpawnParams, TestLocation , SpawnConf.GridCoord.CellCenterNormal , SpawnConf.Z_Offset , WSVolume) )
						{
							SpawnLocations.Add(Local.GetLocation());
							if ( !bShouldRun || !IsValid(RootRef) ) {return;}
							TArray<FWS_Grid_Actor_SpawnData> LActor = RootRef->Grid_Compute_CollectionActor( Random , Local , Actors , WSVolume);
							if ( !bShouldRun || !IsValid(RootRef) ) {return;}
							TArray<FWS_Grid_Static_SpawnData> LStaticMesh =  RootRef->Grid_Compute_CollectionStaticMesh( Random , Local , StaticMesh , WSVolume);

							SpawnQueue.Enqueue(FWS_Grid_SpawnPending(SpawnConf.GridCoord,Local,SpawnConf.Proxy,MoveTemp(LActor),MoveTemp(LStaticMesh),false,SpawnConf.Seed+(SpawnCount*1000000)));
							
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
				const FTransform Base = FTransform(FRotationMatrix::MakeFromZX(SpawnConf.GridCoord.CellCenterNormal,SpawnConf.GridCoord.SizeData.XAxis).ToQuat() ,SpawnConf.GridCoord.CellCenterLocation);
				const int32 SubSpawnCount = SpawnConf.Max_Spawn/9;
				const TArray<FVector> SubLocationSpawnGrid =  RootRef->WS_Grid_GenerateZoneGrid(Base,SpawnConf.GridCoord.SizeData.CircleInternalSize,3);
				const double SubSize = SpawnConf.GridCoord.SizeData.CircleInternalSize/3;
				ParallelFor(SubLocationSpawnGrid.Num(), [&](int32 Index)
				{
					const TArray<FVector> PosPoisson = RootRef->WS_Grid_SearchPoissonDisk( SpawnConf.ZoneData , SubSpawnCount , Random , SpawnConf.SpawnParams , SubLocationSpawnGrid[Index] , SubSize, WSVolume );
					for ( const auto& TestLocation : PosPoisson )
					{
						if ( !bShouldRun || !IsValid(RootRef) ) {return;}
								FTransform Local;
								if ( RootRef->WS_Grid_SnapPoint( Local ,Random,SpawnConf.SpawnParams, TestLocation , SpawnConf.GridCoord.CellCenterNormal , SpawnConf.Z_Offset, WSVolume ) )
								{
									if ( !bShouldRun || !IsValid(RootRef) ) {return;}
									TArray<FWS_Grid_Actor_SpawnData> LActor = RootRef->Grid_Compute_CollectionActor( Random , Local , Actors , WSVolume);
									if ( !bShouldRun || !IsValid(RootRef) ) {return;}
									TArray<FWS_Grid_Static_SpawnData> LStaticMesh =  RootRef->Grid_Compute_CollectionStaticMesh( Random , Local , StaticMesh , WSVolume);

									SpawnQueue.Enqueue(FWS_Grid_SpawnPending(SpawnConf.GridCoord,Local,SpawnConf.Proxy,MoveTemp(LActor),MoveTemp(LStaticMesh),false,SpawnConf.Seed+(SpawnCount*1000000)));
							
									SpawnCount += 1;
									if ( SpawnCount >= SpawnConf.Max_Spawn )
									{
										return;
									}
								}
					}
				});
				break;
			}
		case ::Grid:
			{
				const FTransform Base = FTransform(FRotationMatrix::MakeFromZX(SpawnConf.GridCoord.CellCenterNormal,SpawnConf.GridCoord.SizeData.XAxis).ToQuat() ,SpawnConf.GridCoord.CellCenterLocation);
				const int32 SubSpawnCount = SpawnConf.Max_Spawn/9;
				const TArray<FVector> SubLocationSpawnGrid =  RootRef->WS_Grid_GenerateZoneGrid(Base,SpawnConf.GridCoord.SizeData.CircleInternalSize,3);
				const double SubSize = SpawnConf.GridCoord.SizeData.CircleInternalSize/3;
				ParallelFor(SubLocationSpawnGrid.Num(), [&](int32 Index)
				{
					const TArray<FVector> PosGrid = RootRef->WS_Grid_SearchGridSampling(SpawnConf.ZoneData , SubSpawnCount , Random , SpawnConf.SpawnParams , FTransform(Base.Rotator().Quaternion(),SubLocationSpawnGrid[Index]) , SubSize, WSVolume);
					for ( const auto& TestLocation : PosGrid )
					{
						if ( !bShouldRun || !IsValid(RootRef) ) {return;}
								FTransform Local;
								if ( RootRef->WS_Grid_SnapPoint( Local ,Random,SpawnConf.SpawnParams, TestLocation , SpawnConf.GridCoord.CellCenterNormal , SpawnConf.Z_Offset , WSVolume) )
								{
									if ( !bShouldRun || !IsValid(RootRef) ) {return;}
									TArray<FWS_Grid_Actor_SpawnData> LActor = RootRef->Grid_Compute_CollectionActor( Random , Local , Actors , WSVolume);
									if ( !bShouldRun || !IsValid(RootRef) ) {return;}
									TArray<FWS_Grid_Static_SpawnData> LStaticMesh =  RootRef->Grid_Compute_CollectionStaticMesh( Random , Local , StaticMesh , WSVolume);

									SpawnQueue.Enqueue(FWS_Grid_SpawnPending(SpawnConf.GridCoord,Local,SpawnConf.Proxy,MoveTemp(LActor),MoveTemp(LStaticMesh),false,SpawnConf.Seed+(SpawnCount*1000000)));
							
									SpawnCount += 1;
									if ( SpawnCount >= SpawnConf.Max_Spawn )
									{
										return;
									}
								}
					}
				});
				break;
			}
		}
	}
}
// ----------------------------------------------------
void GridGenerationThread::GridThread_ComputeSpawn_CollectionStaticMesh( const FWS_Grid_SpawnConf& SpawnConf  , const TArray<FWS_Grid_StaticMesh_Data>& StaticMesh , const FWS_VolumeData& WSVolume)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(GridThread_ComputeSpawn_CollectionStaticMesh);
	FRandomStream Random = FRandomStream(SpawnConf.Seed);
	int32 SpawnCount = 0;
	TArray<FVector> SpawnLocations;
	
	if ( IsValid(RootRef) )
	{
		switch (SpawnConf.SamplingType)
		{
		case ::CircleRND:
			{
				for ( int32 i = 0; i < SpawnConf.Max_TestToSpawn; i++ )
				{
					if ( !bShouldRun || !IsValid(RootRef) ) {return;}
					FVector TestLocation;
					if ( RootRef->WS_Grid_SearchPointOnDisk( SpawnConf.ZoneData , TestLocation , Random , SpawnConf.SpawnParams , SpawnLocations , SpawnConf.GridCoord.CellCenterLocation , SpawnConf.GridCoord.CellCenterNormal , SpawnConf.ZoneSize , WSVolume) )
					{
						if ( !bShouldRun || !IsValid(RootRef) ) {return;}
						FTransform Local;
						if ( RootRef->WS_Grid_SnapPoint( Local ,Random,SpawnConf.SpawnParams, TestLocation , SpawnConf.GridCoord.CellCenterNormal , SpawnConf.Z_Offset , WSVolume) )
						{
							SpawnLocations.Add(Local.GetLocation());
							if ( !bShouldRun || !IsValid(RootRef) ) {return;}
							TArray<FWS_Grid_Static_SpawnData> LStaticMesh = RootRef->Grid_Compute_CollectionStaticMesh( Random , Local , StaticMesh , WSVolume);
							SpawnQueue.Enqueue(FWS_Grid_SpawnPending(SpawnConf.GridCoord,Local,SpawnConf.Proxy,TArray<FWS_Grid_Actor_SpawnData>(),MoveTemp(LStaticMesh),false,SpawnConf.Seed+(SpawnCount*1000000)));
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
				const FTransform Base = FTransform(FRotationMatrix::MakeFromZX(SpawnConf.GridCoord.CellCenterNormal,SpawnConf.GridCoord.SizeData.XAxis).ToQuat() ,SpawnConf.GridCoord.CellCenterLocation);
				const int32 SubSpawnCount = SpawnConf.Max_Spawn/9;
				const TArray<FVector> SubLocationSpawnGrid =  RootRef->WS_Grid_GenerateZoneGrid(Base,SpawnConf.GridCoord.SizeData.CircleInternalSize,3);
				const double SubSize = SpawnConf.GridCoord.SizeData.CircleInternalSize/3;
				ParallelFor(SubLocationSpawnGrid.Num(), [&](int32 Index)
				{
					const TArray<FVector> PosPoisson = RootRef->WS_Grid_SearchPoissonDisk( SpawnConf.ZoneData , SubSpawnCount , Random , SpawnConf.SpawnParams , SubLocationSpawnGrid[Index] , SubSize , WSVolume);
					for ( const auto& TestLocation : PosPoisson )
					{
						if ( !bShouldRun || !IsValid(RootRef) ) {return;}
						FTransform Local;
						if ( RootRef->WS_Grid_SnapPoint( Local ,Random,SpawnConf.SpawnParams, TestLocation , SpawnConf.GridCoord.CellCenterNormal , SpawnConf.Z_Offset , WSVolume) )
						{
							if ( !bShouldRun || !IsValid(RootRef) ) {return;}
							TArray<FWS_Grid_Static_SpawnData> LStaticMesh = RootRef->Grid_Compute_CollectionStaticMesh( Random , Local , StaticMesh , WSVolume);
							SpawnQueue.Enqueue(FWS_Grid_SpawnPending(SpawnConf.GridCoord,Local,SpawnConf.Proxy,TArray<FWS_Grid_Actor_SpawnData>(),MoveTemp(LStaticMesh)));
							SpawnCount += 1;
							if ( SpawnCount >= SpawnConf.Max_Spawn )
							{
								return;
							}
						}
					}
				});
				break;
			}
		case ::Grid:
			{
				const FTransform Base = FTransform(FRotationMatrix::MakeFromZX(SpawnConf.GridCoord.CellCenterNormal,SpawnConf.GridCoord.SizeData.XAxis).ToQuat() ,SpawnConf.GridCoord.CellCenterLocation);
				const int32 SubSpawnCount = SpawnConf.Max_Spawn/9;
				const TArray<FVector> SubLocationSpawnGrid =  RootRef->WS_Grid_GenerateZoneGrid(Base,SpawnConf.GridCoord.SizeData.CircleInternalSize,3);
				const double SubSize = SpawnConf.GridCoord.SizeData.CircleInternalSize/3;
				ParallelFor(SubLocationSpawnGrid.Num(), [&](int32 Index)
				{
					const TArray<FVector> PosGrid = RootRef->WS_Grid_SearchGridSampling(SpawnConf.ZoneData , SubSpawnCount , Random , SpawnConf.SpawnParams , FTransform(Base.Rotator().Quaternion(),SubLocationSpawnGrid[Index]) , SubSize, WSVolume);
					for ( const auto& TestLocation : PosGrid )
					{
						if ( !bShouldRun || !IsValid(RootRef) ) {return;}
						FTransform Local;
						if ( RootRef->WS_Grid_SnapPoint( Local ,Random,SpawnConf.SpawnParams, TestLocation , SpawnConf.GridCoord.CellCenterNormal , SpawnConf.Z_Offset , WSVolume) )
						{
							if ( !bShouldRun || !IsValid(RootRef) ) {return;}
							TArray<FWS_Grid_Static_SpawnData> LStaticMesh = RootRef->Grid_Compute_CollectionStaticMesh( Random , Local , StaticMesh , WSVolume);
							SpawnQueue.Enqueue(FWS_Grid_SpawnPending(SpawnConf.GridCoord,Local,SpawnConf.Proxy,TArray<FWS_Grid_Actor_SpawnData>(),MoveTemp(LStaticMesh)));
							SpawnCount += 1;
							if ( SpawnCount >= SpawnConf.Max_Spawn )
							{
								return;
							}
						}
					}
				});
				break;
			}
		}
	}
}
// ----------------------------------------------------
void GridGenerationThread::GridThread_ComputeSpawn_Foliage( const FWS_Grid_SpawnConf& SpawnConf  , const TArray<FWS_Grid_StaticMesh_Data>& StaticMesh , const double& FoliageSectorSize , const FWS_VolumeData& WSVolume)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(GridThread_ComputeSpawn_Foliage);
	FRandomStream Random = FRandomStream(SpawnConf.Seed);
		
		// Compute Grid for FoliageSector
		//const FTransform Base = FTransform(FRotationMatrix::MakeFromZ(SpawnConf.GridCoord.CellCenterNormal).ToQuat() ,SpawnConf.GridCoord.CellCenterLocation);
		const FTransform Base = FTransform(FRotationMatrix::MakeFromZX(SpawnConf.GridCoord.CellCenterNormal,SpawnConf.GridCoord.SizeData.XAxis).ToQuat() ,SpawnConf.GridCoord.CellCenterLocation);
	
		double FSectorSize = FoliageSectorSize;
		const int32 Step = FMath::Max((SpawnConf.ZoneSize*2.2)/FoliageSectorSize,1);
		if ( (SpawnConf.ZoneSize*2.2)/FoliageSectorSize < 1.0f )
		{
			FSectorSize = SpawnConf.ZoneSize;
		}
		if ( IsValid(RootRef) )
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
								if ( !bShouldRun || !IsValid(RootRef) ) {return;}
								FVector TestLocation;
								if ( RootRef->WS_Grid_SearchPointOnDisk( SpawnConf.ZoneData , TestLocation , Random , SpawnConf.SpawnParams , SpawnLocations , NewCenter , SpawnConf.GridCoord.CellCenterNormal , FSectorSize*1.0 , WSVolume, 0.25 ) )
								{
									if ( !bShouldRun || !IsValid(RootRef) ) {return;}
									FTransform Local;
									if ( RootRef->WS_Grid_SnapPoint( Local ,Random,SpawnConf.SpawnParams, TestLocation , SpawnConf.GridCoord.CellCenterNormal , SpawnConf.Z_Offset , WSVolume) )
									{
										SpawnLocations.Add(Local.GetLocation());
										for ( int32 j = 0 ; j < StaticMesh.Num() ; j++ )
										{
											const int32 Index = UKismetMathLibrary::RandomIntegerFromStream(Random,StaticMesh.Num()-1);
											if ( UKismetMathLibrary::RandomFloatInRangeFromStream(Random,0.0f,1.0f) < StaticMesh[Index].SpawnableChance )
											{
												if ( !bShouldRun || !IsValid(RootRef) ) {return;}
												SpawnDataOut.Add(FWS_Grid_Static_SpawnData(StaticMesh[Index].StaticMesh,RootRef->WS_Grid_ComputePlacement(Random,Local,StaticMesh[Index].MeshBounds,StaticMesh[Index].Placement, WSVolume),StaticMesh[Index].Collision,StaticMesh[Index].CastShadow));
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
								if ( !bShouldRun || !IsValid(RootRef) ) {return;}
								SpawnQueue.Enqueue(FWS_Grid_SpawnPending(SpawnConf.GridCoord,BaseTransform,SpawnConf.Proxy,TArray<FWS_Grid_Actor_SpawnData>(),MoveTemp(SpawnDataOut),true,SpawnConf.Seed+(SpawnCount*1000000)));
								SpawnCount += 1;
							}
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
}
// ----------------------------------------------------
void GridGenerationThread::GridThread_ComputeSpawn_Foliage_Para( const FWS_Grid_SpawnConf& SpawnConf  , const TArray<FWS_Grid_StaticMesh_Data>& StaticMesh , const double& FoliageSectorSize , const FWS_VolumeData& WSVolume)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(GridThread_ComputeSpawn_Foliage_Para);
	FRandomStream Random = FRandomStream(SpawnConf.Seed);
		
		// Compute Grid for FoliageSector
		//const FTransform Base = FTransform(FRotationMatrix::MakeFromZ(SpawnConf.GridCoord.CellCenterNormal).ToQuat() ,SpawnConf.GridCoord.CellCenterLocation);
		const FTransform Base = FTransform(FRotationMatrix::MakeFromZX(SpawnConf.GridCoord.CellCenterNormal,SpawnConf.GridCoord.SizeData.XAxis).ToQuat() ,SpawnConf.GridCoord.CellCenterLocation);
	
		double FSectorSize = FoliageSectorSize;
		const int32 Step = FMath::Max((SpawnConf.ZoneSize*2.0)/FoliageSectorSize,1);
		if ( (SpawnConf.ZoneSize*2.0)/FoliageSectorSize < 1.0f )
		{
			FSectorSize = SpawnConf.ZoneSize;
		}
		if ( IsValid(RootRef) )
		{
			switch (SpawnConf.SamplingType)
			{
			case ::CircleRND:
				{
					const FVector Forward = Base.GetRotation().GetForwardVector();
					const FVector Right = Base.GetRotation().GetRightVector();
					const FVector OffsetStart = ((((Step / 2) * Forward * FSectorSize) * -1) + (((Step / 2) * Right * FSectorSize) * -1)) + SpawnConf.GridCoord.CellCenterLocation;

					TArray<FVector> ForLocation;
					
					for (int32 x = 0; x < Step; x++)
					{
						for (int32 y = 0; y < Step; y++)
						{
							ForLocation.Add(OffsetStart + (x * Forward * FSectorSize) + (y * Right * FSectorSize));
						}
					}
					ParallelFor(ForLocation.Num(), [&](int32 Index)
					{
						if ( !bShouldRun ) {return;}
						const FVector NewCenter = ForLocation[Index];
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
								if ( !bShouldRun || !IsValid(RootRef) ) {return;}
								FVector TestLocation;
								if ( RootRef->WS_Grid_SearchPointOnDisk( SpawnConf.ZoneData , TestLocation , Random , SpawnConf.SpawnParams , SpawnLocations , NewCenter , SpawnConf.GridCoord.CellCenterNormal , FSectorSize*1.0 , WSVolume , 0.25 ) )
								{
									if ( !bShouldRun || !IsValid(RootRef) ) {return;}
									FTransform Local;
									if ( RootRef->WS_Grid_SnapPoint( Local ,Random,SpawnConf.SpawnParams, TestLocation , SpawnConf.GridCoord.CellCenterNormal , SpawnConf.Z_Offset , WSVolume) )
									{
										SpawnLocations.Add(Local.GetLocation());
										for ( int32 j = 0 ; j < StaticMesh.Num() ; j++ )
										{
											const int32 IndexSM = UKismetMathLibrary::RandomIntegerFromStream(Random,StaticMesh.Num()-1);
											if ( UKismetMathLibrary::RandomFloatInRangeFromStream(Random,0.0f,1.0f) < StaticMesh[IndexSM].SpawnableChance )
											{
												if ( !bShouldRun || !IsValid(RootRef) ) {return;}
												SpawnDataOut.Add(FWS_Grid_Static_SpawnData(StaticMesh[IndexSM].StaticMesh,RootRef->WS_Grid_ComputePlacement(Random,Local,StaticMesh[IndexSM].MeshBounds,StaticMesh[IndexSM].Placement, WSVolume),StaticMesh[IndexSM].Collision,StaticMesh[Index].CastShadow));
												SpawnCount += 1;
												if ( SpawnCount >= SpawnConf.Max_Spawn )
												{
													if ( !SpawnDataOut.IsEmpty() )
													{
														const FTransform BaseTransform = FTransform( Base.Rotator(),NewCenter);
														if ( !bShouldRun || !IsValid(RootRef) ) {return;}
														SpawnQueue.Enqueue(FWS_Grid_SpawnPending(SpawnConf.GridCoord,BaseTransform,SpawnConf.Proxy,TArray<FWS_Grid_Actor_SpawnData>(),MoveTemp(SpawnDataOut),true,SpawnConf.Seed+(SpawnCount*1000000)));
													}
													return;
													//Over = true;
													//break;
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
								if ( !bShouldRun || !IsValid(RootRef) ) {return;}
								SpawnQueue.Enqueue(FWS_Grid_SpawnPending(SpawnConf.GridCoord,BaseTransform,SpawnConf.Proxy,TArray<FWS_Grid_Actor_SpawnData>(),MoveTemp(SpawnDataOut),true,SpawnConf.Seed+(SpawnCount*1000000)));
							}
						return;
						});
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
}