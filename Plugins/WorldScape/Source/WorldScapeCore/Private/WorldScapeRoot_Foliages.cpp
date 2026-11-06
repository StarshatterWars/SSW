// Copyright 2021 IOLACORP STUDIO. All Rights Reserved
//#include "AsyncTreeDifferences.h"
//#include "GenerateMips.h"
#include "WorldScapeRoot.h"
#include "PoissonDiscSampling.h"
#include "Async/ParallelFor.h"
//ChunkGeneration Thread Functions

FoliageGenerationThread::~FoliageGenerationThread()
{

}

void FoliageGenerationThread::DoWorkMain()
{
	DoWork();
	if (this)
		delete this;
}





bool FoliageGenerationThread::CheckPositionValidity(UWorldScapeFoliagesInterface* Foliage, UWorldScapeFoliagesCollection* FoliageCollection,
	const DVector& Position, const TArray<FHeightMapVolumeDataCopy>& VolumeListVar, const TArray<FNoiseVolumeData>& NoiseVolumeListVar, 
	const TArray<FTerrainHoleVolumeData>& TerrainHoleVolumeListVar, const TArray<FTransform>& VolumeTransformArray
)
{
	float Contraint = false;
	FNoiseData NoiseData = RootRef->GetGroundNoise(Position, NoiseVolumeListVar, VolumeListVar, TerrainHoleVolumeListVar, VolumeTransformArray);
	
	if (Foliage->bOverrideCollectionConstraint)
	{
		Contraint = UWorldScapeFoliagesContraintHelper::GetCombinedFactor(NoiseData.Temperature, NoiseData.Humidity, Foliage->Temperature, Foliage->Humidity);
	}
	else
	{
		Contraint = UWorldScapeFoliagesContraintHelper::GetCombinedFactor(NoiseData.Temperature, NoiseData.Humidity, FoliageCollection->Temperature, FoliageCollection->Humidity);
	}
	//Ignore the foliage if the points has less than 1% chance to spawn foliage to sace performance
	return Contraint > RootRef->FoliageMinimuMSpawnChanceToNotIgnore;
}
// --------------------------------------------------------------
bool FoliageGenerationThread::CheckIfAllFoliageCanBeSpawned(UWorldScapeFoliagesInterface*  ActiveFoliage)
{
	if (ActiveFoliage->GetClass() == UWorldScapeFoliagesCluster::StaticClass())
	{
		//If the cluster has no foliage to spawn, simply ignore it
		if (((UWorldScapeFoliagesCluster*)ActiveFoliage)->FoliagesClusterUnitList.Num() == 0)
			return false;
		
		UWorldScapeFoliagesCluster* FoliageCluster = (UWorldScapeFoliagesCluster*)ActiveFoliage;

		//is one of the foliage cluster has an null static mesh, ignore the whole cluster
		for (int i = 0; i < FoliageCluster->FoliagesClusterUnitList.Num(); i++)
		{
			if (FoliageCluster->FoliagesClusterUnitList[i].StaticMesh == nullptr)
				return false;
		}
	}

	else if (ActiveFoliage->GetClass() == UWorldScapeFoliagesAsset::StaticClass())
	{
		UWorldScapeFoliagesAsset* FoliageAsset = (UWorldScapeFoliagesAsset*)ActiveFoliage;
		//Ignore the foliage if the object to spawn is not valid
		if (FoliageAsset->bSpawnActorInstead && FoliageAsset->ObjectToSpawn == nullptr)
			return false;
		if (!FoliageAsset->bSpawnActorInstead && FoliageAsset->StaticMesh == nullptr)
			return false;
	}

	else if (ActiveFoliage->GetClass() == UWorldScapeFoliagesBlueprint::StaticClass())
	{
		if (((UWorldScapeFoliagesBlueprint*)ActiveFoliage)->ObjectToSpawn == nullptr)
			return false;
	}
	return true;
}
// ------------------------------------------------------------------------
bool FoliageGenerationThread::ShouldSectorBeSampled(FFoliagesSector& Sector, int32 FoliageCollectionID, int32 FoliageID, int32 FLID)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WS_ShouldSectorBeSampled);
	
	TArray<FHeightMapVolumeDataCopy> VolumeListVar;
	TArray<FNoiseVolumeData> NoiseVolumeListVar;
	TArray<FFoliageMaskVolumeData> FoliageMaskListVar;
	TArray<FTerrainHoleVolumeData> TerrainHoleVolumeListVar;

	// If root is not valid, turn back, that mean the root as been destroyed
	if (!IsValid(RootRef))
	{
		return false;
	}
	
	RootRef->GetVolumesData(VolumeListVar, NoiseVolumeListVar, FoliageMaskListVar, TerrainHoleVolumeListVar, Sector.GetFBox());
	
	TArray<FTransform> VolumeTransformArray;
	for (int32 i = 0; i < VolumeListVar.Num(); i++)
	{
		if (VolumeListVar.IsValidIndex(i))
		{
			VolumeTransformArray.Add(VolumeListVar[i].VolumeLocation);
		}
	}
	if (FoliageCollectionID >= RootRef->Foliages.Num())
		return false;
	
	FBox ChunkBox = Sector.GetFBox();
	UWorldScapeFoliagesCollection* ActiveFoliageCollection = RootRef->Foliages[FoliageCollectionID];
	if (FoliageID >= ActiveFoliageCollection->FoliageList.Num())
		return false;
	UWorldScapeFoliagesInterface* ActiveFoliage = ActiveFoliageCollection->FoliageList[FoliageID];
	if (!CheckIfAllFoliageCanBeSpawned(ActiveFoliage)) 
	{
		return false;
	}
	if (CheckPositionValidity(ActiveFoliage, ActiveFoliageCollection, ChunkBox.GetCenter(), VolumeListVar, NoiseVolumeListVar, TerrainHoleVolumeListVar, VolumeTransformArray))
	{
		return true;
	}
	bool IsValidChunk = false;
	for (int x = 0; x < 2; x++)
	{
		for (int y = 0; y < 2; y++)
		{
			for (int z = 0; z < 2; z++)
			{

				DVector Position = DVector(
					ChunkBox.Min.X + ChunkBox.GetSize().X * 0.3333333333 * x,
					ChunkBox.Min.Y + ChunkBox.GetSize().Y * 0.3333333333 * y,
					ChunkBox.Min.Z + ChunkBox.GetSize().Z * 0.3333333333 * z
				);

				if (CheckPositionValidity(ActiveFoliage, ActiveFoliageCollection, Position, VolumeListVar, NoiseVolumeListVar, TerrainHoleVolumeListVar, VolumeTransformArray))
				{
					return true;
				}

			}

		}
	}

	return false;
}

void FoliageGenerationThread::GenerateVegetationForSector(FFoliagesSector& Sector, int32 FoliageCollectionID, int32 FoliageID, int32 FLID)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WS_GenerateVegetationForSector);
	//Chunk.

	/* Old method :
	
	float CoordinateSeed = (Sector.Position.X / PlanetScaleCode) * (Sector.Size * 35000) - (Sector.Position.Y / PlanetScaleCode) * (Sector.Size * 25000) + (Sector.Size * 5000) * (Sector.Position.Z / PlanetScaleCode) + 100 * FoliageCollectionID + 2000 * FoliageID + 50000 * FLID;
	int64 SectorSeed = RootRef->Seed + CoordinateSeed;

	if (abs(SectorSeed) > 2000000000) 
	{
		int32 sign = 1;
		if (SectorSeed < 0)sign = -1;
		int64 SectorRepeat = (abs(SectorSeed) / 2000000000);
		SectorSeed = (abs(SectorSeed) - (static_cast<int64>(2000000000) * SectorRepeat)) * sign;
	}
	*/
	int32 seed = FMath::TruncToInt(FMath::Fmod(FVector::DotProduct(Sector.Position.ToFVector(), FVector(Sector.Size + 100 * FoliageCollectionID, 2000 * FoliageID, 50000 * FLID)), 2000000000));

	FRandomStream Stream = FRandomStream(seed);
	
	TArray<FHeightMapVolumeDataCopy> VolumeListVar;
	TArray<FNoiseVolumeData> NoiseVolumeListVar;
	TArray<FFoliageMaskVolumeData> FoliageMaskListVar;
	TArray<FTerrainHoleVolumeData> TerrainHoleVolumeListVar;
	//Make sure only Volume touching the chunk are processed to reduce CPU Cost
	if (IsValid(RootRef))
	{
		RootRef->GetVolumesData(VolumeListVar, NoiseVolumeListVar,FoliageMaskListVar, TerrainHoleVolumeListVar, Sector.GetFBox());
	}
	else
		return;

	TArray<FTransform> VolumeTransformArray;
	for (int32 i = 0; i < VolumeListVar.Num(); i++)
	{
		if (VolumeListVar.IsValidIndex(i))
		{
			VolumeTransformArray.Add(VolumeListVar[i].VolumeLocation);
		}
	}
	//UE_LOG(LogWorldScapeCore, Log, TEXT("Seed : %i"), (int)SectorSeed);
	if (IsValid(RootRef) &&
		RootRef->Foliages.IsValidIndex(FoliageCollectionID) && IsValid(RootRef->Foliages[FoliageCollectionID]) &&
		RootRef->Foliages[FoliageCollectionID]->FoliageList.IsValidIndex(FoliageID) && IsValid(RootRef->Foliages[FoliageCollectionID]->FoliageList[FoliageID]))
	{
		// TODO add filter by grid zone 
		UWorldScapeFoliagesCollection* ActiveFoliageCollection = RootRef->Foliages[FoliageCollectionID];
		UWorldScapeFoliagesInterface* ActiveFoliage = ActiveFoliageCollection->FoliageList[FoliageID];
		double OceanHeight = RootRef->OceanHeight;

		DVector Tangent, BiTangent;
		Tangent = RootRef->GetTangentFromECEFPoint(Sector.Position.ToFVector());
		BiTangent = RootRef->GetBiTangentFromECEFPoint(Sector.Position.ToFVector());
		// Get a cluster of points
		TArray<DVector> PointsList;
		if (ActiveFoliage->bUsePoissonDisc) 
		{
			TArray< FPoissonObject> GeneratedPoints;
			float averageRadius = 1/ActiveFoliage->PoissonDiscDensity * ActiveFoliage->FoliageSectorSize;
			float minRadius = averageRadius - averageRadius * ActiveFoliage->PoissonDiscDensityVariation*0.01f;
			float maxRadius = averageRadius + averageRadius * ActiveFoliage->PoissonDiscDensityVariation*0.01f;
			UE_LOG(LogWorldScapeCore, Log, TEXT("minRadius : %f"), minRadius);
			UE_LOG(LogWorldScapeCore, Log, TEXT("maxRadius : %f"), maxRadius);
			UE_LOG(LogWorldScapeCore, Log, TEXT("Size : %f"), ActiveFoliage->FoliageSectorSize);
			GeneratedPoints = FPoissonDiscSampling::GeneratePoints(minRadius, maxRadius, FVector2D(ActiveFoliage->FoliageSectorSize), Stream, 30);
			UE_LOG(LogWorldScapeCore, Log, TEXT("GeneratedPoints : %i"), GeneratedPoints.Num());
			for (int32 j = 0; j < GeneratedPoints.Num(); j++)
			{
				DVector FinalPosition = Sector.Position + Tangent * GeneratedPoints[j].Position.X + BiTangent * GeneratedPoints[j].Position.Y - (Tangent* ActiveFoliage->FoliageSectorSize*0.5f + BiTangent * ActiveFoliage->FoliageSectorSize * 0.5f);
				PointsList.Add(FinalPosition);
			}
		}
		else 
		{
			FBox ChunkBox = Sector.GetFBox();
			for (int32 j = 0; j < ActiveFoliage->FoliagesCount; j++)
			{
				//FVector RandomPoint = ;
				PointsList.Add(DVector(WorldScapeHelper::RandPointInBox(ChunkBox, Stream)));
			}
		}
		//FVector VertexWorldTransformedPosition;
		//FVector VertexTransformedPosition;
		int32 ChunkPosition = 0;
		DVector PNA, PNB;
		
		DVector PointPosition;

		if (ActiveFoliage->GetClass() == UWorldScapeFoliagesCluster::StaticClass())
		{
			int FoliageUnitCount = ((UWorldScapeFoliagesCluster*)ActiveFoliage)->FoliagesClusterUnitList.Num();
			for (int FUID = 0; FUID < FoliageUnitCount; FUID++) 
			{
				Sector.VegetationTransformArray.Add(FVegetationTransformArray());
			}
		}
		else
		{
			Sector.VegetationTransformArray.Add(FVegetationTransformArray());
		}
		//ParallelFor(PointsList.Num(), [&](int32 Index)
		for (int32 p = 0; p < PointsList.Num(); p++) 
		{
			RootRef->FoliageCountLeft = PointsList.Num() - p;
			PointPosition = PointsList[p];
			DVector Normal;
			DVector VirtualPos;
			
			Normal = WorldScapeWorld::GetUpVector(PointPosition, bFlatWorld);
			Tangent = RootRef->GetTangentFromECEFPoint(PointPosition.ToFVector());
			BiTangent = RootRef->GetBiTangentFromECEFPoint(PointPosition.ToFVector());
			FNoiseData NoiseData = RootRef->GetGroundNoise(PointPosition, NoiseVolumeListVar, VolumeListVar, TerrainHoleVolumeListVar, VolumeTransformArray);
			
			VirtualPos = WorldScapeWorld::GetVirtualPosition(PointPosition, PlanetScaleCode, bFlatWorld);
			
			if (NoiseData.Hole)
			{
				continue;
			}

			float randomvalue = Stream.FRand();
			
			int foliageLayer = ActiveFoliage->FoliageLayer;
			bool OnlySpawnInVolume = ActiveFoliage->SpawnOnlyInVolume;
			if (foliageLayer < 0) {
				foliageLayer = ActiveFoliageCollection->FoliageLayer;
				OnlySpawnInVolume = ActiveFoliageCollection->SpawnOnlyInVolume;
			}

			PointPosition = Normal * NoiseData.Height + VirtualPos;
			if (WorldScapeHelper::IsPointInCube(PointPosition, Sector.Position, Sector.Size) || bFlatWorld)
			{
				bool IsFoliageInForbidenZone = false;
				DVector PositionTransformedPosition;
				DVector PositionWorldTransformedPosition = PointPosition;
				PositionWorldTransformedPosition = RootRef->GetActorTransform().TransformPositionNoScale(PositionWorldTransformedPosition.ToFVector());

				for (int32 h = 0; h < FoliageMaskListVar.Num(); h++) 
				{
					if (!FoliageMaskListVar.IsValidIndex(h))
					{
						continue;
					}
					// Mask layer or ALL
					if ( !FoliageMaskListVar[h].MaskAll && !FoliageMaskListVar[h].FoliageLayerMask.Contains(foliageLayer))
					{
						continue;
					}

					FFoliageMaskVolumeData ActualVolume = FoliageMaskListVar[h];
					float VolumeSize = ActualVolume.VolumeLocation.GetScale3D().X * 100;

					//if ((PositionWorldTransformedPosition - ActualVolume.VolumeLocation.GetLocation()).Lenght() > VolumeSize * 1.74)
					if ((PositionWorldTransformedPosition - ActualVolume.VolumeLocation.GetLocation()).SizeSquared() > FMath::Square(VolumeSize * 1.74) ) // Set Square
					{
						continue;
					}
					
					//TODO : Make it 64 bit
					PositionTransformedPosition = ActualVolume.VolumeLocation.InverseTransformPositionNoScale(PositionWorldTransformedPosition.ToFVector());
					PositionTransformedPosition = PositionTransformedPosition / VolumeSize * 2;
					if (PositionTransformedPosition.X >= -1 && PositionTransformedPosition.X <=1 && PositionTransformedPosition.Y >= -1 && PositionTransformedPosition.Y <= 1 && PositionTransformedPosition.Z >= -1 && PositionTransformedPosition.Z <= 1)
					{
						// Blending Foliage
						// Falloff
						if ( ActualVolume.UseBlending )
						{
							const float XBlendIntensity = FMath::GetMappedRangeValueClamped(FVector2D(ActualVolume.BlendFalloff,1),FVector2D(0,1),FMath::Abs(PositionTransformedPosition.X));
							const float YBlendIntensity = FMath::GetMappedRangeValueClamped(FVector2D(ActualVolume.BlendFalloff,1),FVector2D(0,1),FMath::Abs(PositionTransformedPosition.Y));
							const float BlendCombinate = XBlendIntensity*YBlendIntensity;
							if ( BlendCombinate > 0.0f )
							{
								const float BlendRNDValue = Stream.FRandRange(0,1);
								if (BlendRNDValue > BlendCombinate)
								{
									IsFoliageInForbidenZone = true;
									break;
								}
							}
							else
							{
								IsFoliageInForbidenZone = true;
								break;
							}
						}
						else
						{
							IsFoliageInForbidenZone = true;
							break;
						}
					}
				}
				if (IsFoliageInForbidenZone != OnlySpawnInVolume)
				{
					continue;
				}
				bool Contraint = false;
				float Noise = NoiseData.FoliageMask;
				if (!ActiveFoliage->bUseFoliageNoiseMask) {
					Noise = 1;
				}
				if (ActiveFoliage->bOverrideCollectionConstraint) 
				{
					Contraint = randomvalue <= Noise * UWorldScapeFoliagesContraintHelper::GetCombinedFactor(
						NoiseData.Height, NoiseData.Temperature, NoiseData.Humidity, ActiveFoliage->Elevation, ActiveFoliage->Temperature, ActiveFoliage->Humidity
					);
					if (Contraint)
					{
						switch (ActiveFoliage->SpawnInWater)
						{
						case EFoliageWaterSpawn::InWater:
							Contraint = (NoiseData.Height < OceanHeight || NoiseData.WaterMask>0.85);
							break;
						case EFoliageWaterSpawn::OutsideWater:
							Contraint = (NoiseData.Height > OceanHeight && NoiseData.WaterMask < 0.85);
						default:
							break;
						}
					}
				}
				else 
				{
					Contraint = randomvalue <= Noise * UWorldScapeFoliagesContraintHelper::GetCombinedFactor(
						NoiseData.Height, NoiseData.Temperature, NoiseData.Humidity, ActiveFoliageCollection->Elevation, ActiveFoliageCollection->Temperature, ActiveFoliageCollection->Humidity
					);
					if (Contraint)
					{
						switch (ActiveFoliageCollection->SpawnInWater)
						{
						case EFoliageWaterSpawn::InWater:
							Contraint = (NoiseData.Height < OceanHeight || NoiseData.WaterMask>0.85);
							break;
						case EFoliageWaterSpawn::OutsideWater:
							Contraint = (NoiseData.Height > OceanHeight && NoiseData.WaterMask < 0.85);
						default:
							break;
						}
					}
				}
				if (!Contraint)
				{
					continue;
				}
				FVector FoliageNormal;
				float NoiseDataAHeight = RootRef->GetGroundNoise((PointPosition + Tangent * 100), NoiseVolumeListVar, VolumeListVar, TerrainHoleVolumeListVar, VolumeTransformArray).Height;
				float NoiseDataBHeight = RootRef->GetGroundNoise((PointPosition + BiTangent * 100), NoiseVolumeListVar, VolumeListVar, TerrainHoleVolumeListVar, VolumeTransformArray).Height;
				PNA = PointPosition + Tangent * 100;
				PNB = PointPosition + BiTangent * 100;
				if (!bFlatWorld) 
				{
					PNB.Normalize();
					PNA.Normalize();
					PNA = PNA * (PlanetScaleCode + NoiseDataAHeight);
					PNB = PNB * (PlanetScaleCode + NoiseDataBHeight);
				}
				else 
				{
					PNA = DVector(PNA.X, PNA.Y, NoiseDataAHeight);
					PNB = DVector(PNB.X, PNB.Y, NoiseDataBHeight);
				}
				DVector GroundNormal = DVector::CrossProduct(PNA - PointPosition, PNB - PointPosition);
				GroundNormal.Normalize();
				float Slope = 1 - FMath::Clamp(FVector::DotProduct(Normal.ToFVector(), GroundNormal.ToFVector()), 0.f, 1.f);
				//Slope Contraint
				if (ActiveFoliage->bOverrideCollectionConstraint)
					Contraint = randomvalue <= UWorldScapeFoliagesContraintHelper::GetCombinedFactor(Slope, ActiveFoliage->Slope);
				else
					Contraint = randomvalue <= UWorldScapeFoliagesContraintHelper::GetCombinedFactor(Slope, ActiveFoliageCollection->Slope);
				if (!Contraint)
				{
					continue;
				}
				// Cluster Case
				if (ActiveFoliage->GetClass() == UWorldScapeFoliagesCluster::StaticClass())
				{
					UWorldScapeFoliagesCluster* FoliageCluster = (UWorldScapeFoliagesCluster*)ActiveFoliage;
					
					for (int32 foliagesClusterId = 0; foliagesClusterId < FoliageCluster->FoliagesClusterUnitList.Num(); foliagesClusterId++)
					{
						FWorldScapeFoliagesClusterUnit foliagesClusterUnit = FoliageCluster->FoliagesClusterUnitList[foliagesClusterId];
						int subFoliagesCount = foliagesClusterUnit.ClusterMax;
						if (foliagesClusterUnit.ClusterMax != foliagesClusterUnit.ClusterMin) 
						{
							subFoliagesCount = FMath::RoundToInt(Stream.FRandRange(foliagesClusterUnit.ClusterMin, foliagesClusterUnit.ClusterMax));
						}
						DVector subFoliagePosition;
						for (int subFoliages = 0; subFoliages < subFoliagesCount; subFoliages++)
						{
							double randomAngle = Stream.FRand() * 2 * PI;
							double r = pow(Stream.FRand(), foliagesClusterUnit.FoliagesConcentration*0.5f);
							double u = sin(randomAngle) * foliagesClusterUnit.ClusterRadius * r;
							double v = cos(randomAngle) * foliagesClusterUnit.ClusterRadius * r;
							
							subFoliagePosition = Tangent * u + BiTangent*v;

							DVector SubPointPosition = PointPosition + subFoliagePosition;
							FNoiseData SubNoiseData = RootRef->GetGroundNoise(SubPointPosition, NoiseVolumeListVar, VolumeListVar, TerrainHoleVolumeListVar, VolumeTransformArray);

							DVector SubPointPositionNormal = WorldScapeWorld::GetUpVector(SubPointPosition, bFlatWorld);

							DVector subVirtualPos = WorldScapeWorld::GetVirtualPosition(SubPointPosition, PlanetScaleCode, bFlatWorld);
							SubPointPosition = SubPointPositionNormal * (SubNoiseData.Height) + subVirtualPos;
							FTransform transform;
							FQuat Rotation;
							FoliageNormal.Normalize();
							float angle = 0;
							
							float SubNoiseDataAHeight = RootRef->GetGroundNoise((SubPointPosition + Tangent * 100), NoiseVolumeListVar, VolumeListVar, TerrainHoleVolumeListVar, VolumeTransformArray).Height;
							float SubNoiseDataBHeight = RootRef->GetGroundNoise((SubPointPosition + BiTangent * 100), NoiseVolumeListVar, VolumeListVar, TerrainHoleVolumeListVar, VolumeTransformArray).Height;
							float LerpValue = (foliagesClusterUnit.GroundRotationInfluenceMax - foliagesClusterUnit.GroundRotationInfluenceMin) * Stream.FRand() + foliagesClusterUnit.GroundRotationInfluenceMin;
							
							PNA = SubPointPosition + Tangent * 100;
							PNB = SubPointPosition + BiTangent * 100;
							if (!bFlatWorld)
							{
								PNB.Normalize();
								PNA.Normalize();
								PNA = PNA * (PlanetScaleCode + SubNoiseDataAHeight);
								PNB = PNB * (PlanetScaleCode + SubNoiseDataBHeight);
							}
							else
							{
								PNA = DVector(PNA.X, PNA.Y, SubNoiseDataAHeight);
								PNB = DVector(PNB.X, PNB.Y, SubNoiseDataBHeight);
							}
							DVector SubGroundNormal = DVector::CrossProduct(PNA - SubPointPosition, PNB - SubPointPosition);
							SubGroundNormal.Normalize();
							if (FoliageCluster->ConstraintAffectFoliages)
							{
								float subRandomValue = Stream.FRand();
								//Slope Contraint
								if (ActiveFoliage->bOverrideCollectionConstraint)
								{
									Contraint = subRandomValue <= UWorldScapeFoliagesContraintHelper::GetCombinedFactor(
										SubNoiseData.Height, SubNoiseData.Temperature, SubNoiseData.Humidity, ActiveFoliage->Elevation, ActiveFoliage->Temperature, ActiveFoliage->Humidity
									);
									if (Contraint)
									{
										switch (ActiveFoliage->SpawnInWater)
										{
										case EFoliageWaterSpawn::InWater:
											Contraint = (NoiseData.Height < OceanHeight || NoiseData.WaterMask>0.85);
											break;
										case EFoliageWaterSpawn::OutsideWater:
											Contraint = (NoiseData.Height > OceanHeight && NoiseData.WaterMask < 0.85);
										default:
											break;
										}
									}
								}
								else
								{
									Contraint = subRandomValue <= UWorldScapeFoliagesContraintHelper::GetCombinedFactor(
										SubNoiseData.Height, SubNoiseData.Temperature, SubNoiseData.Humidity, ActiveFoliageCollection->Elevation, ActiveFoliageCollection->Temperature, ActiveFoliageCollection->Humidity
									);
									if (Contraint)
									{
										switch (ActiveFoliageCollection->SpawnInWater)
										{
										case EFoliageWaterSpawn::InWater:
											Contraint = (NoiseData.Height < OceanHeight || NoiseData.WaterMask>0.85);
											break;
										case EFoliageWaterSpawn::OutsideWater:
											Contraint = (NoiseData.Height > OceanHeight && NoiseData.WaterMask < 0.85);
										default:
											break;
										}
									}
								}
								if (Contraint) 
								{
									float SubSlope = 1 - FMath::Clamp(FVector::DotProduct(Normal.ToFVector(), SubGroundNormal.ToFVector()), 0.f, 1.f);
									//Slope Contraint
									if (ActiveFoliage->bOverrideCollectionConstraint)
										Contraint = randomvalue <= UWorldScapeFoliagesContraintHelper::GetCombinedFactor(SubSlope, ActiveFoliage->Slope);
									else
										Contraint = randomvalue <= UWorldScapeFoliagesContraintHelper::GetCombinedFactor(SubSlope, ActiveFoliageCollection->Slope);
								}
								if (!Contraint)
								{
									continue;
								}
							}
							if (foliagesClusterUnit.bAlignedToGround)
							{
								FoliageNormal = FMath::Lerp(Normal.ToFVector(), SubGroundNormal.ToFVector(), LerpValue);
								Rotation = UKismetMathLibrary::MakeRotFromZ(FoliageNormal).Quaternion();
								if (foliagesClusterUnit.bRandomRotation) 
									Rotation *= FRotator(0, Stream.FRand() * 360.f, 0).Quaternion();
							}
							else if (foliagesClusterUnit.bRandomRotation) {
								FoliageNormal = Normal.ToFVector();
								Rotation = UKismetMathLibrary::RandomRotatorFromStream(Stream,true ).Quaternion();
							}
							else 
								FoliageNormal = Normal.ToFVector();

							float scale = (foliagesClusterUnit.MaxScale - foliagesClusterUnit.MinScale) * Stream.FRand() + foliagesClusterUnit.MinScale;
							Rotation.Normalize();
							FVector Offset = foliagesClusterUnit.Offset;
							Offset = Rotation.RotateVector(Offset);
#if ENGINE_MAJOR_VERSION == 5
							transform.SetLocation((SubPointPosition + DVector(Offset) - DVector(Sector.Position)).ToFVector());
#else
							transform.SetLocation((SubPointPosition + DVector(Offset)).ToFVector());
#endif
							transform.SetRotation(Rotation);
							transform.SetScale3D(FVector(scale));
							if (!transform.IsRotationNormalized()) {

								UE_LOG(LogWorldScapeCore, Log, TEXT(" Threaded Rotation is not normalized and would crash\nRotation Value : %s\nTransform removed from the spawning pool"), *transform.GetRotation().ToString());
							}
							Sector.VegetationTransformArray[foliagesClusterId].VegetationTransform.Add(transform);
#if ENGINE_MAJOR_VERSION == 4
							Sector.VegetationTransformArray[foliagesClusterId].VegetationPosition.Add(SubPointPosition + DVector(Offset));
#endif
						}
					}
				}
				//FOLIAGES BLUEPRINT CASE :
				else if (ActiveFoliage->GetClass() == UWorldScapeFoliagesBlueprint::StaticClass())
				{
					UWorldScapeFoliagesBlueprint* FoliagesToUse = (UWorldScapeFoliagesBlueprint*)ActiveFoliage;
					if (FoliagesToUse->bAlignedToGround)
					{
						float LerpValue = (FoliagesToUse->GroundRotationInfluenceMax - FoliagesToUse->GroundRotationInfluenceMin) * Stream.FRand() + FoliagesToUse->GroundRotationInfluenceMin;
						FoliageNormal = FMath::Lerp(Normal.ToFVector(), GroundNormal.ToFVector(), LerpValue);
					}
					else
						FoliageNormal = Normal.ToFVector();

					float scale = (FoliagesToUse->MaxScale - FoliagesToUse->MinScale) * Stream.FRand() + FoliagesToUse->MinScale;
					FTransform transform;
					FRotator LocalRandomRotation = RootRef->GetActorRotation();		
					if (FoliagesToUse->bRandomRotation) {
						LocalRandomRotation = UKismetMathLibrary::RandomRotatorFromStream(Stream, true);
					}
					FQuat Rotation = FQuat(LocalRandomRotation);
					FoliageNormal.Normalize();		
					if (FoliagesToUse->bAlignedToGround){
						Rotation = UKismetMathLibrary::MakeRotFromZX(FoliageNormal, LocalRandomRotation.Vector()).Quaternion();
					}
					Rotation.Normalize();
					FVector Offset = FoliagesToUse->Offset;
					Offset = Rotation.RotateVector(Offset);
#if ENGINE_MAJOR_VERSION == 5
					transform.SetLocation((PointPosition + DVector(Offset * scale) - DVector(Sector.Position)).ToFVector());
#else
					transform.SetLocation((PointPosition + DVector(Offset * scale)).ToFVector());
#endif
					transform.SetRotation(Rotation);
					transform.SetScale3D(FVector(scale));
					if (!transform.IsRotationNormalized()) {

						UE_LOG(LogWorldScapeCore, Log, TEXT(" Threaded Rotation is not normalized and would crash\nRotation Value : %s\nTransform removed from the spawning pool"), *transform.GetRotation().ToString());
					}
					Sector.VegetationTransformArray[0].VegetationTransform.Add(transform);
#if ENGINE_MAJOR_VERSION == 4
					Sector.VegetationTransformArray[0].VegetationPosition.Add(PointPosition + DVector(Offset));
#endif
				}
				//FOLIAGES ASSETS CASE :
				else if (ActiveFoliage->GetClass() == UWorldScapeFoliagesAsset::StaticClass())
				{
					UWorldScapeFoliagesAsset* FoliagesToUse = (UWorldScapeFoliagesAsset*)ActiveFoliage;
					if (FoliagesToUse->bAlignedToGround)
					{
						float LerpValue = (FoliagesToUse->GroundRotationInfluenceMax - FoliagesToUse->GroundRotationInfluenceMin) * Stream.FRand() + FoliagesToUse->GroundRotationInfluenceMin;
						FoliageNormal = FMath::Lerp(Normal.ToFVector(), GroundNormal.ToFVector(), LerpValue);
					}
					else
						FoliageNormal = Normal.ToFVector();

					float scale = (FoliagesToUse->MaxScale - FoliagesToUse->MinScale) * Stream.FRand() + FoliagesToUse->MinScale;
					FTransform transform;
					FQuat Rotation;
					FoliageNormal.Normalize();
					float angle = 0;
					if (FoliagesToUse->bAlignedToGround)
					{
						Rotation = UKismetMathLibrary::MakeRotFromZ(FoliageNormal).Quaternion();
						if (FoliagesToUse->bRandomRotation) {
							Rotation *= FRotator(0, Stream.FRand() * 360.f, 0).Quaternion();
						}
					}
					else if (FoliagesToUse->bRandomRotation) {
						//angle = Stream.FRand() * 360.f;
						Rotation = UKismetMathLibrary::RandomRotatorFromStream(Stream, true).Quaternion();
					}
					Rotation.Normalize();
					FVector Offset = FoliagesToUse->Offset;
					Offset = Rotation.RotateVector(Offset);
#if ENGINE_MAJOR_VERSION == 5
					transform.SetLocation((PointPosition + DVector(Offset) - DVector(Sector.Position)).ToFVector());
#else
					transform.SetLocation((PointPosition + DVector(Offset)).ToFVector());
#endif
					transform.SetRotation(Rotation);
					transform.SetScale3D(FVector(scale));
					if (!transform.IsRotationNormalized()) {

						UE_LOG(LogWorldScapeCore, Log, TEXT(" Threaded Rotation is not normalized and would crash\nRotation Value : %s\nTransform removed from the spawning pool"), *transform.GetRotation().ToString());
					}
					Sector.VegetationTransformArray[0].VegetationTransform.Add(transform);
#if ENGINE_MAJOR_VERSION == 4
					Sector.VegetationTransformArray[0].VegetationPosition.Add(PointPosition + DVector(Offset));
#endif
				}
			}
		}
		PointsList.Empty();
	}
	//delete Stream;
}

// ParallelFor Version
void FoliageGenerationThread::GenerateVegetationForSector_Para(FFoliagesSector& Sector, int32 FoliageCollectionID, int32 FoliageID, int32 FLID)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WS_GenerateVegetationForSector_Para);
	int32 seed = FMath::TruncToInt(FMath::Fmod(FVector::DotProduct(Sector.Position.ToFVector(), FVector(Sector.Size + 100 * FoliageCollectionID, 2000 * FoliageID, 50000 * FLID)), 2000000000));

	FRandomStream Stream = FRandomStream(seed);
	
	TArray<FHeightMapVolumeDataCopy> VolumeListVar;
	TArray<FNoiseVolumeData> NoiseVolumeListVar;
	TArray<FFoliageMaskVolumeData> FoliageMaskListVar;
	TArray<FTerrainHoleVolumeData> TerrainHoleVolumeListVar;
	//Make sure only Volume touching the chunk are processed to reduce CPU Cost

	if (IsValid(RootRef))
	{
		RootRef->GetVolumesData(VolumeListVar, NoiseVolumeListVar,FoliageMaskListVar, TerrainHoleVolumeListVar, Sector.GetFBox());
	}
	else
		return;

	TArray<FTransform> VolumeTransformArray;
	for (int32 i = 0; i < VolumeListVar.Num(); i++)
	{
		if (VolumeListVar.IsValidIndex(i))
		{
			VolumeTransformArray.Add(VolumeListVar[i].VolumeLocation);
		}
			
	}
	//UE_LOG(LogWorldScapeCore, Log, TEXT("Seed : %i"), (int)SectorSeed);
	if (IsValid(RootRef) &&
		RootRef->Foliages.IsValidIndex(FoliageCollectionID) && IsValid(RootRef->Foliages[FoliageCollectionID]) &&
		RootRef->Foliages[FoliageCollectionID]->FoliageList.IsValidIndex(FoliageID) && IsValid(RootRef->Foliages[FoliageCollectionID]->FoliageList[FoliageID]))
	{
		// TODO add filter by grid zone 
		UWorldScapeFoliagesCollection* ActiveFoliageCollection = RootRef->Foliages[FoliageCollectionID];
		UWorldScapeFoliagesInterface* ActiveFoliage = ActiveFoliageCollection->FoliageList[FoliageID];
		double OceanHeight = RootRef->OceanHeight;

		DVector Tangent, BiTangent;
		Tangent = RootRef->GetTangentFromECEFPoint(Sector.Position.ToFVector());
		BiTangent = RootRef->GetBiTangentFromECEFPoint(Sector.Position.ToFVector());
		// Get a cluster of points
		TArray<DVector> PointsList;
		if (ActiveFoliage->bUsePoissonDisc) 
		{
			TArray< FPoissonObject> GeneratedPoints;
			float averageRadius = 1/ActiveFoliage->PoissonDiscDensity * ActiveFoliage->FoliageSectorSize;
			float minRadius = averageRadius - averageRadius * ActiveFoliage->PoissonDiscDensityVariation*0.01f;
			float maxRadius = averageRadius + averageRadius * ActiveFoliage->PoissonDiscDensityVariation*0.01f;
			UE_LOG(LogWorldScapeCore, Log, TEXT("minRadius : %f"), minRadius);
			UE_LOG(LogWorldScapeCore, Log, TEXT("maxRadius : %f"), maxRadius);
			UE_LOG(LogWorldScapeCore, Log, TEXT("Size : %f"), ActiveFoliage->FoliageSectorSize);
			GeneratedPoints = FPoissonDiscSampling::GeneratePoints(minRadius, maxRadius, FVector2D(ActiveFoliage->FoliageSectorSize), Stream, 30);
			UE_LOG(LogWorldScapeCore, Log, TEXT("GeneratedPoints : %i"), GeneratedPoints.Num());
			for (int32 j = 0; j < GeneratedPoints.Num(); j++)
			{
				DVector FinalPosition = Sector.Position + Tangent * GeneratedPoints[j].Position.X + BiTangent * GeneratedPoints[j].Position.Y - (Tangent* ActiveFoliage->FoliageSectorSize*0.5f + BiTangent * ActiveFoliage->FoliageSectorSize * 0.5f);
				PointsList.Add(FinalPosition);
			}
		}
		else 
		{
			FBox ChunkBox = Sector.GetFBox();
			for (int32 j = 0; j < ActiveFoliage->FoliagesCount; j++)
			{
				//FVector RandomPoint = ;
				PointsList.Add(DVector(WorldScapeHelper::RandPointInBox(ChunkBox, Stream)));
			}
		}
		//FVector VertexWorldTransformedPosition;
		//FVector VertexTransformedPosition;
		int32 ChunkPosition = 0;
		DVector PNA, PNB;
		
		DVector PointPosition;

		if (ActiveFoliage->GetClass() == UWorldScapeFoliagesCluster::StaticClass())
		{
			int FoliageUnitCount = ((UWorldScapeFoliagesCluster*)ActiveFoliage)->FoliagesClusterUnitList.Num();
			for (int FUID = 0; FUID < FoliageUnitCount; FUID++) 
			{
				Sector.VegetationTransformArray.Add(FVegetationTransformArray());
			}
		}
		else
		{
			Sector.VegetationTransformArray.Add(FVegetationTransformArray());
		}

		// ------------------------------------------
		// ParallelFor Exec
		// ------------------------------------------
		TArray<FWS_FoliageAsyncData_Array> ComputeTransform;
		for ( int32 j = 0; j < PointsList.Num(); j++)
		{
			ComputeTransform.Add(FWS_FoliageAsyncData_Array(Sector.VegetationTransformArray.Num()));
		}
		if ( !PointsList.IsEmpty() )
		{
			return;
		}
		ParallelFor(PointsList.Num(), [&](int32 p)
		{
			PointPosition = PointsList[p];
			DVector Normal;
			DVector VirtualPos;
			
			Normal = WorldScapeWorld::GetUpVector(PointPosition, bFlatWorld);
			Tangent = RootRef->GetTangentFromECEFPoint(PointPosition.ToFVector());
			BiTangent = RootRef->GetBiTangentFromECEFPoint(PointPosition.ToFVector());
			FNoiseData NoiseData = RootRef->GetGroundNoise(PointPosition, NoiseVolumeListVar, VolumeListVar, TerrainHoleVolumeListVar, VolumeTransformArray);
			
			VirtualPos = WorldScapeWorld::GetVirtualPosition(PointPosition, PlanetScaleCode, bFlatWorld);
			
			if (NoiseData.Hole)
			{
				return;
			}

			float randomvalue = Stream.FRand();
			int foliageLayer = ActiveFoliage->FoliageLayer;
			bool OnlySpawnInVolume = ActiveFoliage->SpawnOnlyInVolume;
			if (foliageLayer < 0) {
				foliageLayer = ActiveFoliageCollection->FoliageLayer;
				OnlySpawnInVolume = ActiveFoliageCollection->SpawnOnlyInVolume;
			}

			PointPosition = Normal * NoiseData.Height + VirtualPos;
			if (WorldScapeHelper::IsPointInCube(PointPosition, Sector.Position, Sector.Size) || bFlatWorld)
			{
				bool IsFoliageInForbidenZone = false;
				DVector PositionTransformedPosition;
				DVector PositionWorldTransformedPosition = PointPosition;
				PositionWorldTransformedPosition = RootRef->GetActorTransform().TransformPositionNoScale(PositionWorldTransformedPosition.ToFVector());

				for (int32 h = 0; h < FoliageMaskListVar.Num(); h++) 
				{
					if (!FoliageMaskListVar.IsValidIndex(h))
					{
						continue;
					}
					if (!FoliageMaskListVar[h].FoliageLayerMask.Contains(foliageLayer))
					{
						continue;
					}

					FFoliageMaskVolumeData ActualVolume = FoliageMaskListVar[h];
					float VolumeSize = ActualVolume.VolumeLocation.GetScale3D().X * 100;
					
					if ((PositionWorldTransformedPosition - ActualVolume.VolumeLocation.GetLocation()).SizeSquared() > FMath::Square(VolumeSize * 1.74) ) // Set Square
					{
						continue;
					}
					//TODO : Make it 64 bit
					PositionTransformedPosition = ActualVolume.VolumeLocation.InverseTransformPositionNoScale(PositionWorldTransformedPosition.ToFVector());
					PositionTransformedPosition = PositionTransformedPosition / VolumeSize * 2;

					if (PositionTransformedPosition.X >= -1 && PositionTransformedPosition.X <=1 &&
						PositionTransformedPosition.Y >= -1 && PositionTransformedPosition.Y <= 1 &&
						PositionTransformedPosition.Z >= -1 && PositionTransformedPosition.Z <= 1)
					{
						IsFoliageInForbidenZone = true;
						
						break;
					}
				}

				if (IsFoliageInForbidenZone != OnlySpawnInVolume)
				{
					return;
				}
				bool Contraint = false;

				float Noise = NoiseData.FoliageMask;
				if (!ActiveFoliage->bUseFoliageNoiseMask) {
					Noise = 1;
				}

				if (ActiveFoliage->bOverrideCollectionConstraint) 
				{
						Contraint = randomvalue <= Noise * UWorldScapeFoliagesContraintHelper::GetCombinedFactor(
							NoiseData.Height, NoiseData.Temperature, NoiseData.Humidity, ActiveFoliage->Elevation, ActiveFoliage->Temperature, ActiveFoliage->Humidity
						);

						if (Contraint)
						{
							switch (ActiveFoliage->SpawnInWater)
							{
							case EFoliageWaterSpawn::InWater:
								Contraint = (NoiseData.Height < OceanHeight || NoiseData.WaterMask>0.85);
								break;
							case EFoliageWaterSpawn::OutsideWater:
								Contraint = (NoiseData.Height > OceanHeight && NoiseData.WaterMask < 0.85);
							default:
								break;
							}
						}
				}
				else 
				{
					Contraint = randomvalue <= Noise * UWorldScapeFoliagesContraintHelper::GetCombinedFactor(
						NoiseData.Height, NoiseData.Temperature, NoiseData.Humidity, ActiveFoliageCollection->Elevation, ActiveFoliageCollection->Temperature, ActiveFoliageCollection->Humidity
					);

					if (Contraint)
					{
						switch (ActiveFoliageCollection->SpawnInWater)
						{
						case EFoliageWaterSpawn::InWater:
							Contraint = (NoiseData.Height < OceanHeight || NoiseData.WaterMask>0.85);
							break;
						case EFoliageWaterSpawn::OutsideWater:
							Contraint = (NoiseData.Height > OceanHeight && NoiseData.WaterMask < 0.85);
						default:
							break;
						}

					}
				}
				
				if (!Contraint)
				{
					return;
				}

				FVector FoliageNormal;
				float NoiseDataAHeight = RootRef->GetGroundNoise((PointPosition + Tangent * 100), NoiseVolumeListVar, VolumeListVar, TerrainHoleVolumeListVar, VolumeTransformArray).Height;
				float NoiseDataBHeight = RootRef->GetGroundNoise((PointPosition + BiTangent * 100), NoiseVolumeListVar, VolumeListVar, TerrainHoleVolumeListVar, VolumeTransformArray).Height;
				PNA = PointPosition + Tangent * 100;
				PNB = PointPosition + BiTangent * 100;
				if (!bFlatWorld) 
				{
					PNB.Normalize();
					PNA.Normalize();

					PNA = PNA * (PlanetScaleCode + NoiseDataAHeight);
					PNB = PNB * (PlanetScaleCode + NoiseDataBHeight);
				}
				else 
				{
					PNA = DVector(PNA.X, PNA.Y, NoiseDataAHeight);
					PNB = DVector(PNB.X, PNB.Y, NoiseDataBHeight);
				}

				DVector GroundNormal = DVector::CrossProduct(PNA - PointPosition, PNB - PointPosition);
				GroundNormal.Normalize();
				
				float Slope = 1 - FMath::Clamp(FVector::DotProduct(Normal.ToFVector(), GroundNormal.ToFVector()), 0.f, 1.f);
				
				//Slope Contraint
				if (ActiveFoliage->bOverrideCollectionConstraint)
					Contraint = randomvalue <= UWorldScapeFoliagesContraintHelper::GetCombinedFactor(Slope, ActiveFoliage->Slope);
				else
					Contraint = randomvalue <= UWorldScapeFoliagesContraintHelper::GetCombinedFactor(Slope, ActiveFoliageCollection->Slope);
				
				if (!Contraint)
				{
					return;
				}
				
				if (ActiveFoliage->GetClass() == UWorldScapeFoliagesCluster::StaticClass())
				{
					UWorldScapeFoliagesCluster* FoliageCluster = (UWorldScapeFoliagesCluster*)ActiveFoliage;
					
					for (int32 foliagesClusterId = 0; foliagesClusterId < FoliageCluster->FoliagesClusterUnitList.Num(); foliagesClusterId++)
					{
						FWorldScapeFoliagesClusterUnit foliagesClusterUnit = FoliageCluster->FoliagesClusterUnitList[foliagesClusterId];

						int subFoliagesCount = foliagesClusterUnit.ClusterMax;
						if (foliagesClusterUnit.ClusterMax != foliagesClusterUnit.ClusterMin) 
						{
							subFoliagesCount = FMath::RoundToInt(Stream.FRandRange(foliagesClusterUnit.ClusterMin, foliagesClusterUnit.ClusterMax));
						}
						
						DVector subFoliagePosition;
						for (int subFoliages = 0; subFoliages < subFoliagesCount; subFoliages++)
						{
							double randomAngle = Stream.FRand() * 2 * PI;
							double r = pow(Stream.FRand(), foliagesClusterUnit.FoliagesConcentration*0.5f);
							double u = sin(randomAngle) * foliagesClusterUnit.ClusterRadius * r;
							double v = cos(randomAngle) * foliagesClusterUnit.ClusterRadius * r;
							
							subFoliagePosition = Tangent * u + BiTangent*v;

							DVector SubPointPosition = PointPosition + subFoliagePosition;
							FNoiseData SubNoiseData = RootRef->GetGroundNoise(SubPointPosition, NoiseVolumeListVar, VolumeListVar, TerrainHoleVolumeListVar, VolumeTransformArray);

							DVector SubPointPositionNormal = WorldScapeWorld::GetUpVector(SubPointPosition, bFlatWorld);

							DVector subVirtualPos = WorldScapeWorld::GetVirtualPosition(SubPointPosition, PlanetScaleCode, bFlatWorld);
							SubPointPosition = SubPointPositionNormal * (SubNoiseData.Height) + subVirtualPos;
							FTransform transform;
							FQuat Rotation;
							FoliageNormal.Normalize();
							float angle = 0;
							
							float SubNoiseDataAHeight = RootRef->GetGroundNoise((SubPointPosition + Tangent * 100), NoiseVolumeListVar, VolumeListVar, TerrainHoleVolumeListVar, VolumeTransformArray).Height;
							float SubNoiseDataBHeight = RootRef->GetGroundNoise((SubPointPosition + BiTangent * 100), NoiseVolumeListVar, VolumeListVar, TerrainHoleVolumeListVar, VolumeTransformArray).Height;
							float LerpValue = (foliagesClusterUnit.GroundRotationInfluenceMax - foliagesClusterUnit.GroundRotationInfluenceMin) * Stream.FRand() + foliagesClusterUnit.GroundRotationInfluenceMin;
							
							PNA = SubPointPosition + Tangent * 100;
							PNB = SubPointPosition + BiTangent * 100;
							if (!bFlatWorld)
							{
								PNB.Normalize();
								PNA.Normalize();

								PNA = PNA * (PlanetScaleCode + SubNoiseDataAHeight);
								PNB = PNB * (PlanetScaleCode + SubNoiseDataBHeight);
							}
							else
							{
								PNA = DVector(PNA.X, PNA.Y, SubNoiseDataAHeight);
								PNB = DVector(PNB.X, PNB.Y, SubNoiseDataBHeight);
							}
							DVector SubGroundNormal = DVector::CrossProduct(PNA - SubPointPosition, PNB - SubPointPosition);
							SubGroundNormal.Normalize();

							if (FoliageCluster->ConstraintAffectFoliages)
							{
								float subRandomValue = Stream.FRand();


								//Slope Contraint
								if (ActiveFoliage->bOverrideCollectionConstraint)
								{
									Contraint = subRandomValue <= UWorldScapeFoliagesContraintHelper::GetCombinedFactor(
										SubNoiseData.Height, SubNoiseData.Temperature, SubNoiseData.Humidity, ActiveFoliage->Elevation, ActiveFoliage->Temperature, ActiveFoliage->Humidity
									);

									if (Contraint)
									{
										switch (ActiveFoliage->SpawnInWater)
										{
										case EFoliageWaterSpawn::InWater:
											Contraint = (NoiseData.Height < OceanHeight || NoiseData.WaterMask>0.85);
											break;
										case EFoliageWaterSpawn::OutsideWater:
											Contraint = (NoiseData.Height > OceanHeight && NoiseData.WaterMask < 0.85);
										default:
											break;
										}
									}

								}
								else
								{
									Contraint = subRandomValue <= UWorldScapeFoliagesContraintHelper::GetCombinedFactor(
										SubNoiseData.Height, SubNoiseData.Temperature, SubNoiseData.Humidity, ActiveFoliageCollection->Elevation, ActiveFoliageCollection->Temperature, ActiveFoliageCollection->Humidity
									);

									if (Contraint)
									{
										switch (ActiveFoliageCollection->SpawnInWater)
										{
										case EFoliageWaterSpawn::InWater:
											Contraint = (NoiseData.Height < OceanHeight || NoiseData.WaterMask>0.85);
											break;
										case EFoliageWaterSpawn::OutsideWater:
											Contraint = (NoiseData.Height > OceanHeight && NoiseData.WaterMask < 0.85);
										default:
											break;
										}
									}

								}

								if (Contraint) 
								{
									float SubSlope = 1 - FMath::Clamp(FVector::DotProduct(Normal.ToFVector(), SubGroundNormal.ToFVector()), 0.f, 1.f);

									//Slope Contraint
									if (ActiveFoliage->bOverrideCollectionConstraint)
										Contraint = randomvalue <= UWorldScapeFoliagesContraintHelper::GetCombinedFactor(SubSlope, ActiveFoliage->Slope);
									else
										Contraint = randomvalue <= UWorldScapeFoliagesContraintHelper::GetCombinedFactor(SubSlope, ActiveFoliageCollection->Slope);

								}
								
								if (!Contraint)
								{
									continue;
								}
							}

							if (foliagesClusterUnit.bAlignedToGround)
							{
								FoliageNormal = FMath::Lerp(Normal.ToFVector(), SubGroundNormal.ToFVector(), LerpValue);

								Rotation = UKismetMathLibrary::MakeRotFromZ(FoliageNormal).Quaternion();

								if (foliagesClusterUnit.bRandomRotation) 
									Rotation *= FRotator(0, Stream.FRand() * 360.f, 0).Quaternion();
								
							}
							else if (foliagesClusterUnit.bRandomRotation) {
								FoliageNormal = Normal.ToFVector();
								Rotation = UKismetMathLibrary::RandomRotatorFromStream(Stream,true ).Quaternion();
							}
							else 
								FoliageNormal = Normal.ToFVector();

							float scale = (foliagesClusterUnit.MaxScale - foliagesClusterUnit.MinScale) * Stream.FRand() + foliagesClusterUnit.MinScale;

							Rotation.Normalize();

							FVector Offset = foliagesClusterUnit.Offset;
							Offset = Rotation.RotateVector(Offset);

#if ENGINE_MAJOR_VERSION == 5
							transform.SetLocation((SubPointPosition + DVector(Offset) - DVector(Sector.Position)).ToFVector());
#else
							transform.SetLocation((SubPointPosition + DVector(Offset)).ToFVector());
#endif
							

							transform.SetRotation(Rotation);
							transform.SetScale3D(FVector(scale));

							if (!transform.IsRotationNormalized()) {

								UE_LOG(LogWorldScapeCore, Log, TEXT(" Threaded Rotation is not normalized and would crash\nRotation Value : %s\nTransform removed from the spawning pool"), *transform.GetRotation().ToString());
							}
							ComputeTransform[p].Data[foliagesClusterId].VegetationTransform.Add(transform);
							//Sector.VegetationTransformArray[foliagesClusterId].VegetationTransform.Add(transform);
#if ENGINE_MAJOR_VERSION == 4
							//Sector.VegetationTransformArray[foliagesClusterId].VegetationPosition.Add(SubPointPosition + DVector(Offset));
#endif
							
						}
					}
				}

				//FOLIAGES BLUEPRINT CASE :
				
				else if (ActiveFoliage->GetClass() == UWorldScapeFoliagesBlueprint::StaticClass())
				{
					UWorldScapeFoliagesBlueprint* FoliagesToUse = (UWorldScapeFoliagesBlueprint*)ActiveFoliage;

					if (FoliagesToUse->bAlignedToGround)
					{
						float LerpValue = (FoliagesToUse->GroundRotationInfluenceMax - FoliagesToUse->GroundRotationInfluenceMin) * Stream.FRand() + FoliagesToUse->GroundRotationInfluenceMin;
						FoliageNormal = FMath::Lerp(Normal.ToFVector(), GroundNormal.ToFVector(), LerpValue);
					}
					else
						FoliageNormal = Normal.ToFVector();

					float scale = (FoliagesToUse->MaxScale - FoliagesToUse->MinScale) * Stream.FRand() + FoliagesToUse->MinScale;

					FTransform transform;
					FRotator LocalRandomRotation = RootRef->GetActorRotation();		

					if (FoliagesToUse->bRandomRotation) {
						LocalRandomRotation = UKismetMathLibrary::RandomRotatorFromStream(Stream, true);
					}

					FQuat Rotation = FQuat(LocalRandomRotation);
					FoliageNormal.Normalize();		
					
					if (FoliagesToUse->bAlignedToGround){
						Rotation = UKismetMathLibrary::MakeRotFromZX(FoliageNormal, LocalRandomRotation.Vector()).Quaternion();
					}

					Rotation.Normalize();

					FVector Offset = FoliagesToUse->Offset;
					Offset = Rotation.RotateVector(Offset);


#if ENGINE_MAJOR_VERSION == 5
					transform.SetLocation((PointPosition + DVector(Offset * scale) - DVector(Sector.Position)).ToFVector());
#else
					transform.SetLocation((PointPosition + DVector(Offset * scale)).ToFVector());
#endif

					transform.SetRotation(Rotation);
					transform.SetScale3D(FVector(scale));

					if (!transform.IsRotationNormalized()) {

						UE_LOG(LogWorldScapeCore, Log, TEXT(" Threaded Rotation is not normalized and would crash\nRotation Value : %s\nTransform removed from the spawning pool"), *transform.GetRotation().ToString());
					}
					ComputeTransform[p].Data[0].VegetationTransform.Add(transform);
					//Sector.VegetationTransformArray[0].VegetationTransform.Add(transform);
#if ENGINE_MAJOR_VERSION == 4
					//Sector.VegetationTransformArray[0].VegetationPosition.Add(PointPosition + DVector(Offset));
#endif
				}


				//FOLIAGES ASSETS CASE :
				else if (ActiveFoliage->GetClass() == UWorldScapeFoliagesAsset::StaticClass())
				{
					UWorldScapeFoliagesAsset* FoliagesToUse = (UWorldScapeFoliagesAsset*)ActiveFoliage;

					if (FoliagesToUse->bAlignedToGround)
					{
						float LerpValue = (FoliagesToUse->GroundRotationInfluenceMax - FoliagesToUse->GroundRotationInfluenceMin) * Stream.FRand() + FoliagesToUse->GroundRotationInfluenceMin;
						FoliageNormal = FMath::Lerp(Normal.ToFVector(), GroundNormal.ToFVector(), LerpValue);
					}
					else
						FoliageNormal = Normal.ToFVector();

					float scale = (FoliagesToUse->MaxScale - FoliagesToUse->MinScale) * Stream.FRand() + FoliagesToUse->MinScale;

					FTransform transform;
					FQuat Rotation;
					FoliageNormal.Normalize();
					float angle = 0;

					if (FoliagesToUse->bAlignedToGround)
					{
						Rotation = UKismetMathLibrary::MakeRotFromZ(FoliageNormal).Quaternion();

						if (FoliagesToUse->bRandomRotation) {
							Rotation *= FRotator(0, Stream.FRand() * 360.f, 0).Quaternion();
						}

					}
					else if (FoliagesToUse->bRandomRotation) {
						//angle = Stream.FRand() * 360.f;
						Rotation = UKismetMathLibrary::RandomRotatorFromStream(Stream, true).Quaternion();
					}
					
					Rotation.Normalize();

					FVector Offset = FoliagesToUse->Offset;
					Offset = Rotation.RotateVector(Offset);


#if ENGINE_MAJOR_VERSION == 5
					transform.SetLocation((PointPosition + DVector(Offset) - DVector(Sector.Position)).ToFVector());
#else
					transform.SetLocation((PointPosition + DVector(Offset)).ToFVector());
#endif

					transform.SetRotation(Rotation);
					transform.SetScale3D(FVector(scale));

					if (!transform.IsRotationNormalized()) {

						UE_LOG(LogWorldScapeCore, Log, TEXT(" Threaded Rotation is not normalized and would crash\nRotation Value : %s\nTransform removed from the spawning pool"), *transform.GetRotation().ToString());
					}
					ComputeTransform[p].Data[0].VegetationTransform.Add(transform);
					//Sector.VegetationTransformArray[0].VegetationTransform.Add(transform);
#if ENGINE_MAJOR_VERSION == 4
					//Sector.VegetationTransformArray[0].VegetationPosition.Add(PointPosition + DVector(Offset));
#endif
				}
			}
			
		});
		for ( int32 Index = 0 ; Index < ComputeTransform.Num() ; Index++ )
		{
			for ( int32 Sub = 0 ; Sub < ComputeTransform[Index].Data.Num() ; Sub++ )
			{
				Sector.VegetationTransformArray[Index].VegetationTransform.Append(MoveTemp(ComputeTransform[Index].Data[Sub].VegetationTransform));
			}
			RootRef->FoliageCountLeft = PointsList.Num() - Index;
		}
		ComputeTransform.Empty();
		PointsList.Empty();
	}
}


bool ListCountainSector(TArray<FFoliagesSector>* Array, FFoliagesSector item)
{
	return false;
}

void FoliageGenerationThread::DoWork() 
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WS_Foliage_DoWork);
	TArray<FFoliagesSector> FoliageSectorList;

	if (!IsValid(RootRef))
	{
		RootRef->FoliageGenerationInProgress = false;
		return;
	}

	
	//Focus generating foliage with bigger sector size
	VegetationList.Sort([](const FVegetation& Veg1, const FVegetation& Veg2) {
		if (Veg1.IsActor && !Veg2.IsActor )
			return true;
	 return  Veg1.SectorSize > Veg2.SectorSize;
	});
	

	for (int32 i = 0; i < VegetationList.Num(); i++) 
	{
		bool TooHigh = true;

		const int32 FCI = VegetationList[i].FoliageCollectionIndex;

		const int32 FLI = VegetationList[i].FoliageListIndex;
		
		const int32 FI = VegetationList[i].FoliageIndex;

		if (!IsValid(RootRef)) {
			RootRef->FoliageGenerationInProgress = false;
			return;
		}
			


		if (!(RootRef->Foliages.IsValidIndex(FCI) && IsValid(RootRef->Foliages[FCI])))
		{
			continue;
		}

		if (!(RootRef->Foliages[FCI]->FoliageList.IsValidIndex(FI) && IsValid(RootRef->Foliages[FCI]->FoliageList[FI])))
		{
			continue;
		}

		//Skip Foliage Generation for server when asked
		if (IsValid(RootRef) && RootRef->GetNetMode() == ENetMode::NM_DedicatedServer && !(RootRef->Foliages[FCI]->FoliageList[FI]->bGenerateOnServer))
		{
			continue;
		}
		
		FoliageSectorList = RootRef->GetSurroundingFoliageSector(PlayerPos, RootRef->Foliages[FCI]->FoliageList[FI]->FoliageSectorSize);

		if (IsValid(RootRef) && PlayerDistanceToGround < 2 * RootRef->Foliages[FCI]->FoliageList[FI]->FoliageSectorSize) 
		{
			TooHigh = false;	
		}

		
			
		
		//Skip Foliage Generation if altitude is too high;
		if (TooHigh)
			continue;

		if (IsValid(RootRef)) RootRef->FoliageLeft = VegetationList.Num() - i;

		for (int32 j = 0; j < FoliageSectorList.Num(); j++) 
		{

			if (IsValid(RootRef)) RootRef->FoliageSectorLeft = FoliageSectorList.Num() - j;
			

			bool SectorAlreadyExist = false;

			for (int32 k = 0; k < VegetationList[i].ActiveFoliageSector.Num(); k++)
			{
				if (VegetationList[i].ActiveFoliageSector[k] == FoliageSectorList[j]) 
				{
					SectorAlreadyExist = true;
					break;
				}
			}

			if (SectorAlreadyExist || !FoliageSectorList.IsValidIndex(j))
			{
				continue;
			}
			

			if (ShouldSectorBeSampled(FoliageSectorList[j], FCI, FI, FLI)) 
			{
				if ( IsValid(RootRef) )
				{
					if ( RootRef->FoliageParallelForCompute )
					{
						GenerateVegetationForSector_Para(FoliageSectorList[j], FCI, FI, FLI);
					}
					else
					{
						GenerateVegetationForSector(FoliageSectorList[j], FCI, FI, FLI);
					}
				}
			}

			if (!IsValid(RootRef) || !RootRef->Foliages.IsValidIndex(FCI) || !IsValid(RootRef->Foliages[FCI]) || !RootRef->Foliages[FCI]->FoliageList.IsValidIndex(FI) || !IsValid(RootRef->Foliages[FCI]->FoliageList[FI]))
				continue;

			if (RootRef->Foliages[FCI]->FoliageList[FI]->GetClass() == UWorldScapeFoliagesCluster::StaticClass())
			{
				if (FoliageSectorList[j].VegetationTransformArray.Num() > 0)
				{
					if (IsValid(RootRef) && RootRef->FoliageDataToSpawn.IsValidIndex(FLI))
					{
						RootRef->FoliageDataToSpawn[FLI].ActiveFoliageSector.Enqueue(FoliageSectorList[j]);
					}
					else
					{
						RootRef->FoliageGenerationInProgress = false;
						return;
					}

				}
				
			}
			else 
			{
				if (FoliageSectorList[j].VegetationTransformArray.Num() > 0 && FoliageSectorList[j].VegetationTransformArray[0].VegetationTransform.Num() > 0)
				{
					if (IsValid(RootRef) && RootRef->FoliageDataToSpawn.IsValidIndex(FLI))
					{
						RootRef->FoliageDataToSpawn[FLI].ActiveFoliageSector.Enqueue(FoliageSectorList[j]);
					}
					else
					{
						RootRef->FoliageGenerationInProgress = false;
						return;
					}
				}
			}
			
		}
	}

	if (!IsValid(RootRef)) 
	{
		RootRef->FoliageGenerationInProgress = false;
		return;
	}
	RootRef->FoliageCountLeft = 0;
	RootRef->FoliageSectorLeft = 0;
	RootRef->FoliageLeft = 0;
	VegetationList.Empty();
	RootRef->FoliageGenerationInProgress = false;
}

FoliageGenerationThread::FoliageGenerationThread(AWorldScapeRoot* pRoot, const double& pPlanetScale, const bool& pFlatWorld,const  DVector& pPlayerPosition, const TArray<FVegetation>& pVegetationList, const double& pPlayerDistanceToGround) 
{
	RootRef = pRoot;
	//PlanetPos  pPlanetLocation
	PlanetScaleCode = pPlanetScale;
	bFlatWorld = pFlatWorld;
	PlayerPos = pPlayerPosition;
	VegetationList = pVegetationList;
	PlayerDistanceToGround = pPlayerDistanceToGround;
}