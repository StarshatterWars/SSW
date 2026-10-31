// Copyright 2018-2025 Athian Games. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "PVTLibrary.h"
#include "UObject/NoExportTypes.h"
#include "VortexData.generated.h"


UCLASS()
class PROCEDURALVORTEXTUNNEL_API UVortexData : public UObject
{
	GENERATED_BODY()


public:
	
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Vortex")
		TArray<FVortexMaterialData> DefaultMaterials;

	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Vortex")
		TArray<FVortexMeshData> VortexMeshData;

	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Material Blending")
		bool bDisableMaterialBlending;
	
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Material Blending")
		TArray<UMaterialInterface*> MaterialsToBlend;

	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Material Blending")
		bool bEqualBlendDistribution;

	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Material Blending", meta = (EditCondition = "!bEqualBLendDistribution"))
		FRuntimeFloatCurve MBDistributionCurve;

	UVortexData()
	{
		bEqualBlendDistribution = true;
		bDisableMaterialBlending = true;
		
		MBDistributionCurve.GetRichCurve()->AddKey(0, 0);
		MBDistributionCurve.GetRichCurve()->AddKey(100, 1);
	}

	TArray<FVortexMeshData> GetValidMeshData()
	{
		TArray<FVortexMeshData> ValidMeshInfo;
		for (int iIndex = 0; iIndex < VortexMeshData.Num(); iIndex++)
		{
			if(VortexMeshData[iIndex].Mesh)
			{
				ValidMeshInfo.Add(VortexMeshData[iIndex]);
				
			}
		}

		return ValidMeshInfo;
	}
	
};
