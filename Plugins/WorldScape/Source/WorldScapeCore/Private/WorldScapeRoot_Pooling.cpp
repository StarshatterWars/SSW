// Copyright 2024 IOLACORP STUDIO. All Rights Reserved

#include "WorldScapeRoot.h"
#include "WS_FunctionLibrary.h"


// ----------------------------------------------------
AHeightMapVolume* AWorldScapeRoot::Pooling_Build_HeightMapVolume()
{
	if ( UWorld* World = GetWorld() )
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.ObjectFlags = RF_Transient;
		SpawnParams.Owner = this ;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.bDeferConstruction = true;

		AHeightMapVolume* SpawnedActor = World->SpawnActor<AHeightMapVolume>(Heightmap_Volume_Class,GetActorLocation(),GetActorRotation(),SpawnParams);
		if ( IsValid(SpawnedActor) )
		{
			SpawnedActor->SetFlags(RF_Transient);
			SpawnedActor->IsEnabled = false;
			SpawnedActor->FinishSpawning(GetActorTransform());
			return SpawnedActor;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("WS Pooling : Create Heightmap volume pooling fail"));
		}
		return nullptr;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("WS Pooling : Create Heightmap volume get world fail"));
	}
	return nullptr;
}
// ----------------------------------------------------
bool AWorldScapeRoot::Pooling_Take_HeightmapVolume( AHeightMapVolume*& Volume )
{
	if ( VolumePooling_Enabled )
	{
		if ( WS_Pooling_Dedicated && !VolumePooling_Enabled_OnDedicated )
		{
			return false;
		}
		if ( !Pooling_HeightMapVolumes.IsEmpty() )
		{
			Pooling_HeightMapVolumes_Used.Add( Pooling_HeightMapVolumes[0] );
			Pooling_HeightMapVolumes.RemoveAt(0);
			Volume = Pooling_HeightMapVolumes_Used.Last();
			return true;
		}
		else if ( Pooling_HeightMapVolumes_Used.Num() < ( Heightmap_Pooling_Maximal * (WS_Pooling_Dedicated ? Heightmap_Pooling_Dedicated_Multiplier : 1 )) )
		{
			Volume = Pooling_Build_HeightMapVolume();
			if ( IsValid(Volume) )
			{
				HeightMapVolumeList.Add( Volume );
				Pooling_HeightMapVolumes_Used.Add(Volume);
				return true;
			}
		}
		UE_LOG(LogTemp, Warning, TEXT("WS Pooling : Heightmap pool is full"));
		return false;
	}
	return false;
}
// ----------------------------------------------------
void AWorldScapeRoot::Pooling_Release_HeightmapVolume( AHeightMapVolume* Volume )
{
	if ( VolumePooling_Enabled )
	{
		if ( WS_Pooling_Dedicated && !VolumePooling_Enabled_OnDedicated )
		{
			return;
		}
		if ( IsValid(Volume) )
		{
			Pooling_HeightMapVolumes_Used.Remove(Volume);
			if ( Pooling_HeightMapVolumes.Num() < (Heightmap_Pooling_Minimal * (WS_Pooling_Dedicated ? Heightmap_Pooling_Dedicated_Multiplier : 1 )) )
			{
				Pooling_HeightMapVolumes.Add(Volume);
				Volume->IsEnabled = false;
				Volume->WS_ForceRefresh();
			}
			else
			{
				HeightMapVolumeList.Remove(Volume);
				Volume->Destroy();
			}
		}
	}
}
// ----------------------------------------------------
void AWorldScapeRoot::Pooling_Release_HeightmapVolume_Multi( TArray<AHeightMapVolume*>& Volumes )
{
	for ( auto& Volume : Volumes )
	{
		Pooling_Release_HeightmapVolume(Volume);
	}
}
// ----------------------------------------------------
AFoliageMaskVolume* AWorldScapeRoot::Pooling_Build_FoliageMaskVolume()
{
	if ( UWorld* World = GetWorld() )
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.ObjectFlags = RF_Transient;
		SpawnParams.Owner = this ;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.bDeferConstruction = true;

		AFoliageMaskVolume* SpawnedActor = World->SpawnActor<AFoliageMaskVolume>(Foliage_Volume_Class,GetActorLocation(),GetActorRotation(),SpawnParams);
		if ( IsValid(SpawnedActor) )
		{
			SpawnedActor->SetFlags(RF_Transient);
			SpawnedActor->IsEnabled = false;
			SpawnedActor->FinishSpawning(GetActorTransform());
			return SpawnedActor;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("WS Pooling : Create Foliage volume pooling fail"));
		}
		return nullptr;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("WS Pooling : Create Foliage volume get world fail"));
	}
	return nullptr;
}
// ----------------------------------------------------
bool AWorldScapeRoot::Pooling_Take_FoliageMaskVolume( AFoliageMaskVolume*& Volume )
{
	if ( VolumePooling_Enabled )
	{
		if ( WS_Pooling_Dedicated && !VolumePooling_Enabled_OnDedicated )
		{
			return false;
		}
		if ( !Pooling_Foliage_Volumes.IsEmpty() )
		{
			Pooling_Foliage_Volumes_Used.Add( Pooling_Foliage_Volumes[0] );
			Pooling_Foliage_Volumes.RemoveAt(0);
			Volume = Pooling_Foliage_Volumes_Used.Last();
			return true;
		}
		else if ( Pooling_Foliage_Volumes_Used.Num() < ( Foliage_Pooling_Maximal * (WS_Pooling_Dedicated ? HFoliage_Pooling_Dedicated_Multiplier : 1 )) )
		{
			Volume = Pooling_Build_FoliageMaskVolume();
			if ( IsValid(Volume) )
			{
				FoliageMaskList.Add(Volume);
				Pooling_Foliage_Volumes_Used.Add(Volume);
				return true;
			}
		}
		UE_LOG(LogTemp, Warning, TEXT("WS Pooling : foliage pool is full"));
		return false;
	}
	return false;
}
// ----------------------------------------------------
void AWorldScapeRoot::Pooling_Release_FoliageMaskVolume( AFoliageMaskVolume*  Volume )
{
	if ( VolumePooling_Enabled )
	{
		if ( WS_Pooling_Dedicated && !VolumePooling_Enabled_OnDedicated )
		{
			return;
		}
		if ( IsValid(Volume) )
		{
			Pooling_Foliage_Volumes_Used.Remove(Volume);
			if ( Pooling_Foliage_Volumes.Num() < (Foliage_Pooling_Minimal * (WS_Pooling_Dedicated ? HFoliage_Pooling_Dedicated_Multiplier : 1 )) )
			{
				Pooling_Foliage_Volumes.Add(Volume);
				Volume->IsEnabled = false;
				Volume->WS_ForceRefresh();
			}
			else
			{
				FoliageMaskList.Remove(Volume);
				Volume->Destroy();
			}
		}
	}
}
// ----------------------------------------------------
void AWorldScapeRoot::Pooling_Release_FoliageMaskVolume_Multi( TArray<AFoliageMaskVolume*>& Volumes )
{
	for (auto& Volume : Volumes)
	{
		Pooling_Release_FoliageMaskVolume( Volume );
	}
}
// ----------------------------------------------------
ATerrainHoleVolume* AWorldScapeRoot::Pooling_Build_HoleVolume()
{
	if ( UWorld* World = GetWorld() )
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.ObjectFlags = RF_Transient;
		SpawnParams.Owner = this ;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.bDeferConstruction = true;

		ATerrainHoleVolume* SpawnedActor = World->SpawnActor<ATerrainHoleVolume>(Hole_Volume_Class,GetActorLocation(),GetActorRotation(),SpawnParams);
		if ( IsValid(SpawnedActor) )
		{
			SpawnedActor->SetFlags(RF_Transient);
			SpawnedActor->IsEnabled = false;
			SpawnedActor->FinishSpawning(GetActorTransform());
			return SpawnedActor;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("WS Pooling : Create Hole volume pooling fail"));
		}
		return nullptr;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("WS Pooling : Create Hole volume get world fail"));
	}
	return nullptr;
}
// ----------------------------------------------------
bool AWorldScapeRoot::Pooling_Take_HoleVolume( ATerrainHoleVolume*& Volume )
{
	if ( VolumePooling_Enabled )
	{
		if ( WS_Pooling_Dedicated && !VolumePooling_Enabled_OnDedicated )
		{
			return false;
		}
		if ( !Pooling_HoleVolumes.IsEmpty() )
		{
			Pooling_HoleVolumes_Used.Add( Pooling_HoleVolumes[0] );
			Pooling_HoleVolumes.RemoveAt(0);
			Volume = Pooling_HoleVolumes_Used.Last();
			return true;
		}
		else if ( Pooling_HoleVolumes_Used.Num() < ( Hole_Pooling_Maximal * (WS_Pooling_Dedicated ? Hole_Pooling_Dedicated_Multiplier : 1 )) )
		{
			Volume = Pooling_Build_HoleVolume();
			if ( IsValid(Volume) )
			{
				TerrainHoleList.Add(Volume);
				Pooling_HoleVolumes_Used.Add(Volume);
				return true;
			}
		}
		UE_LOG(LogTemp, Warning, TEXT("WS Pooling : Hole pool is full"));
		return false;
	}
	return false;
}
// ----------------------------------------------------
void AWorldScapeRoot::Pooling_Release_HoleVolume( ATerrainHoleVolume* Volume )
{
	if ( VolumePooling_Enabled )
	{
		if ( WS_Pooling_Dedicated && !VolumePooling_Enabled_OnDedicated )
		{
			return;
		}
		if ( IsValid(Volume) )
		{
			Pooling_HoleVolumes_Used.Remove(Volume);
			if ( Pooling_HoleVolumes.Num() < (Hole_Pooling_Maximal * (WS_Pooling_Dedicated ? Hole_Pooling_Dedicated_Multiplier : 1 )) )
			{
				Pooling_HoleVolumes.Add(Volume);
				Volume->IsEnabled = false;
				Volume->WS_ForceRefresh();
			}
			else
			{
				TerrainHoleList.Remove(Volume);
				Volume->Destroy();
			}
		}
	}
}
// ----------------------------------------------------
void AWorldScapeRoot::Pooling_Release_HoleVolume_Multi( TArray<ATerrainHoleVolume*>& Volumes )
{
	for (auto& Volume : Volumes)
	{
		Pooling_Release_HoleVolume( Volume );
	}
}
// ----------------------------------------------------
void AWorldScapeRoot::Pooling_Init()
{
	if ( VolumePooling_Enabled )
	{
		if ( UWorld* World = GetWorld() )
		{
			if ( World->GetNetMode() == NM_DedicatedServer )
			{
				WS_Pooling_Dedicated = true;
				if ( !VolumePooling_Enabled_OnDedicated )
				{
					return;
				}
			}
			Pooling_Clear();
			for ( int32 i = 0 ; i < Heightmap_Pooling_Minimal * ( WS_Pooling_Dedicated ? Heightmap_Pooling_Dedicated_Multiplier : 1 ); i++ )
			{
				if ( AHeightMapVolume* HM_Volume = Pooling_Build_HeightMapVolume() )
				{
					Pooling_HeightMapVolumes.Add( HM_Volume );
					HeightMapVolumeList.Add( HM_Volume );
				}
			}
			for ( int32 i = 0 ; i < Heightmap_Pooling_Minimal * ( WS_Pooling_Dedicated ? HFoliage_Pooling_Dedicated_Multiplier : 1 ) ; i++ )
			{
				if ( AFoliageMaskVolume* Foliage_Volume = Pooling_Build_FoliageMaskVolume() )
				{
					Pooling_Foliage_Volumes.Add( Foliage_Volume );
					FoliageMaskList.Add( Foliage_Volume );
				}
			}
			for ( int32 i = 0 ; i < Heightmap_Pooling_Minimal * ( WS_Pooling_Dedicated ? Hole_Pooling_Dedicated_Multiplier : 1 ); i++ )
			{
				if ( ATerrainHoleVolume* Hole_Volume = Pooling_Build_HoleVolume() )
				{
					Pooling_HoleVolumes.Add( Hole_Volume );
					TerrainHoleList.Add(Hole_Volume);
				}
			}
			if ( Pooling_AutoCompute_FoliageMaskLayerAll )
			{
				Pooling_ComputeFoliageMask();
			}
			WS_Pooling_Initialized = true;
			WS_Pooling_OnInitialized.Broadcast();
		}
	}
}
// ----------------------------------------------------
void AWorldScapeRoot::Pooling_Clear()
{
	if ( !Pooling_HeightMapVolumes.IsEmpty() )
	{
		for (auto& HM_Volume : Pooling_HeightMapVolumes )
		{
			if ( IsValid(HM_Volume) )
			{
				HeightMapVolumeList.Remove(HM_Volume);
				HM_Volume->Destroy();
			}
		}
		Pooling_HeightMapVolumes.Empty();
	}
	if ( !Pooling_HeightMapVolumes_Used.IsEmpty() )
	{
		for (auto& HM_Volume_Used : Pooling_HeightMapVolumes_Used )
		{
			if ( IsValid(HM_Volume_Used) )
			{
				HeightMapVolumeList.Remove(HM_Volume_Used);
				HM_Volume_Used->Destroy();
			}
		}
		Pooling_HeightMapVolumes_Used.Empty();
	}
	//
	if ( !Pooling_Foliage_Volumes.IsEmpty() )
	{
		for (auto& Foliage_Volume : Pooling_Foliage_Volumes )
		{
			if ( IsValid(Foliage_Volume) )
			{
				FoliageMaskList.Remove(Foliage_Volume);
				Foliage_Volume->Destroy();
			}
		}
		Pooling_Foliage_Volumes.Empty();
	}
	if ( !Pooling_Foliage_Volumes_Used.IsEmpty() )
	{
		for (auto& Foliage_Volume_Used : Pooling_Foliage_Volumes_Used )
		{
			if ( IsValid(Foliage_Volume_Used) )
			{
				FoliageMaskList.Remove(Foliage_Volume_Used);
				Foliage_Volume_Used->Destroy();
			}
		}
		Pooling_Foliage_Volumes_Used.Empty();
	}
	//
	if ( !Pooling_HoleVolumes.IsEmpty() )
	{
		for (auto& Hole_Volume : Pooling_HoleVolumes )
		{
			if ( IsValid(Hole_Volume) )
			{
				TerrainHoleList.Remove(Hole_Volume);
				Hole_Volume->Destroy();
			}
		}
		Pooling_HoleVolumes.Empty();
	}
	if ( !Pooling_HoleVolumes_Used.IsEmpty() )
	{
		for (auto& Hole_Volume_Used : Pooling_HoleVolumes_Used )
		{
			if ( IsValid(Hole_Volume_Used) )
			{
				TerrainHoleList.Remove(Hole_Volume_Used);
				Hole_Volume_Used->Destroy();
			}
		}
		Pooling_HoleVolumes_Used.Empty();
	}
}
// ----------------------------------------------------
bool AWorldScapeRoot::WS_Pooling_IsInitialized()
{
	return WS_Pooling_Initialized;
}
// ----------------------------------------------------
TArray<int> AWorldScapeRoot::Pooling_GetFoliageMaskLayersAll()
{
	return Pooling_Foliage_MaskAll;
}
// ----------------------------------------------------
void AWorldScapeRoot::Pooling_ComputeFoliageMask()
{
	Pooling_Foliage_MaskAll = UWS_FunctionLibrary::WS_GetAllFoliageIndexMask(this);
}