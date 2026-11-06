// Copyright 2021 IOLACORP STUDIO. All Rights Reserved
#include "FoliageMaskVolume.h" 


// -------------------------------------------------------------
FFoliageMaskVolumeData AFoliageMaskVolume::GetFoliageMaskVolumeData()
{
	FFoliageMaskVolumeData DataToReturn = FFoliageMaskVolumeData();
	if ( IsValid(this))
	{
		DataToReturn.VolumeLocation = IsValid(this) ? GetActorTransform() : FTransform();
	}
	DataToReturn.FoliageLayerMask = FoliageLayerMask;
	DataToReturn.SpawnVolume = SpawnVolume;
	DataToReturn.IsEnabled = IsEnabled;
	DataToReturn.UseBlending = UseBlending;
	DataToReturn.BlendFalloff = BlendFalloff;
	DataToReturn.MaskAll = MaskAll;
	return DataToReturn;
}
// -------------------------------------------------------------
void AFoliageMaskVolume::OnConstruction(const FTransform& Transform)
{
	if (PREVIOUS_Location != GetActorLocation()) 
	{
		NeedRefresh = true;
	}

	if (PREVIOUS_Rotation != GetActorRotation()) 
	{
		NeedRefresh = true;
	}

	if (PREVIOUS_Scale != GetActorScale()) 
	{
		NeedRefresh = true;
	}

	if (FoliageLayerMask.Num() != PREVIOUS_FoliageLayerMask.Num()) 
	{
		NeedRefresh = true;
	}
	if (SpawnVolume != PREVIOUS_SpawnVolume)
	{
		NeedRefresh = true;
	}

	if (UseBlending != PREVIOUS_UseBlending)
	{
		NeedRefresh = true;
	}
	if (BlendFalloff != PREVIOUS_BlendFalloff)
	{
		NeedRefresh = true;
	}
	if (MaskAll != PREVIOUS_MaskAll)
	{
		NeedRefresh = true;
	}
	else {
		for (int i = 0; i < FoliageLayerMask.Num(); i++) 
		{
			if (FoliageLayerMask.IsValidIndex(i) && PREVIOUS_FoliageLayerMask.IsValidIndex(i))
			{
				if (FoliageLayerMask[i] != PREVIOUS_FoliageLayerMask[i]) 
				{
					NeedRefresh = true;
					break;
				}
			}
			else 
			{
				NeedRefresh = true;
				break;
			}
		}
	}
	if (NeedRefresh)
	{
		PREVIOUS_Location = GetActorLocation();
		PREVIOUS_Rotation = GetActorRotation();
		PREVIOUS_Scale = GetActorScale();
		PREVIOUS_FoliageLayerMask = FoliageLayerMask;
		PREVIOUS_SpawnVolume = SpawnVolume;
		PREVIOUS_UseBlending = UseBlending;
		PREVIOUS_BlendFalloff = BlendFalloff;
		PREVIOUS_MaskAll = MaskAll;
	}
}
// -------------------------------------------------------------
AFoliageMaskVolume::AFoliageMaskVolume()
{
	PrimaryActorTick.bCanEverTick = false;
	FoliageLayerMask.Empty();
}


