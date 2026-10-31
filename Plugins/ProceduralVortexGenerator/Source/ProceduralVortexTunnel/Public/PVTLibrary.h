// Copyright 2018-2025 Athian Games. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "NiagaraSystem.h"
#include "Async/Async.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Curves/CurveFloat.h"

#include "PVTLibrary.generated.h"

class UVortexData;

UENUM(BlueprintType)
enum class EVortexType : uint8
{
    Normal UMETA(DisplayName = "Normal"),
    Wired UMETA(DisplayName = "Wired"),
    CubeCluster UMETA(DisplayName = "Cube Cluster"),
    Ring UMETA(DisplayName = "Ring")
};

USTRUCT(BlueprintType, Category = "Vortex")
struct FVortexMaterialData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Mesh Override")
	int32 ID;

	UPROPERTY(EditAnywhere, Category = "Mesh Override")
	TArray<UMaterialInterface*> Materials;

	FVortexMaterialData(): ID(0)
	{
	}
};

USTRUCT(BlueprintType, Category = "Vortex")
struct FVortexProperties
{
	GENERATED_BODY()

	UPROPERTY(EditAnyWhere, Category = "Spline")
		TEnumAsByte<ESplineMeshAxis::Type> SplineMeshAxis;
	
	UPROPERTY(EditAnyWhere, Category = "Vortex")
		float RadiusScale;

	UPROPERTY(EditAnyWhere, Category = "Vortex")
		bool bEnableCollision;
	

	UPROPERTY(EditAnyWhere, Category = "Spline")
		FRuntimeFloatCurve ScaleDistributionCurve;

	
	FVortexProperties(): bEnableCollision(true)
	{
		SplineMeshAxis = ESplineMeshAxis::Z;
		RadiusScale = 1;

		ScaleDistributionCurve.GetRichCurve()->AddKey(0, 1);
		ScaleDistributionCurve.GetRichCurve()->AddKey(100, 1);
	}
};


USTRUCT(BlueprintType, Category = "Vortex")
struct FVortexMeshData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Mesh Override")
		UStaticMesh* Mesh;

	UPROPERTY(EditAnywhere, Category = "Mesh Override")
		TArray<FVortexMaterialData> CustomMaterials;

	FVortexMeshData()
	{
		Mesh = nullptr;
	}


};

USTRUCT(BlueprintType)
struct FVortexParameters
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vortex Parameters")
    EVortexType VortexType = EVortexType::Normal;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vortex Parameters")
    float Radius = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vortex Parameters")
    float Height = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vortex Parameters")
    int32 RadialSegments = 32;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vortex Parameters")
    int32 HeightSegments = 64;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vortex Parameters")
    float TwistAmount = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vortex Parameters")
    float TwistOffset = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vortex Parameters")
    float WaveAmplitude = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vortex Parameters")
    float WaveFrequency = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vortex Parameters")
    float WavePhase = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vortex Parameters")
    float TaperTop = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vortex Parameters")
    float TaperBottom = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vortex Parameters")
    float OrganicIntensity = 10.0f;

    // Wired type parameters
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wired Parameters", meta = (EditCondition = "VortexType == EVortexType::Wired"))
    float WireThickness = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wired Parameters", meta = (EditCondition = "VortexType == EVortexType::Wired"))
	int32 WireSegments = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wired Parameters", meta = (EditCondition = "VortexType == EVortexType::Wired"))
	int32 RingInterval = 4;

    // Cube Cluster parameters
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Cluster Parameters", meta = (EditCondition = "VortexType == EVortexType::CubeCluster"))
    float MinCubeSize = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Cluster Parameters", meta = (EditCondition = "VortexType == EVortexType::CubeCluster"))
    float MaxCubeSize = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Cluster Parameters", meta = (EditCondition = "VortexType == EVortexType::CubeCluster"))
    int32 CubesPerCluster = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Cluster Parameters", meta = (EditCondition = "VortexType == EVortexType::CubeCluster"))
    float ClusterSpacing = 50.0f;

    // Ring parameters
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ring Parameters", meta = (EditCondition = "VortexType == EVortexType::Ring"))
    float MinRingThickness = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ring Parameters", meta = (EditCondition = "VortexType == EVortexType::Ring"))
    float MaxRingThickness = 10.0f;
};


USTRUCT(BlueprintType, Category = "Vortex")
struct FVortexComponent 
{
	
	GENERATED_BODY()

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vortex")
	    USplineMeshComponent* MeshComponent;

	UPROPERTY()
		float DistanceAlongSpline;

	UPROPERTY()
		FVector MeshDirection;

	UPROPERTY()
		FVortexMeshData CurrentMeshInfo;

	UPROPERTY()
		UVortexData* CurrentVortexData;
		
	UPROPERTY()	
		UMaterialInterface* CurrentVMD;

	UPROPERTY()	
		UMaterialInterface* NextVMD;

	UPROPERTY()	
		float VMDAlpha;
	
	UPROPERTY()
	float StartDistance;

	UPROPERTY()
	float EndDistance;

	UPROPERTY()
	float PreviousTimeIndex;

	UPROPERTY()
	float CurrentTimeIndex;

	UPROPERTY()
	 bool GroupEntered;

	UPROPERTY()
	bool GroupExited;

	UPROPERTY()
		TMap<UMaterialInterface*, UMaterialInstanceDynamic*> DynamicMaterialMap;

	void UpdateVortexMesh(FVortexProperties VTProperties, USplineComponent* CurrentSplineComponent);

	void UpdateVortexMeshScalingAtIndex(FVortexProperties VTProperties);

	bool CreateSingleSplineMesh(AActor* Owner, FVortexProperties VTProperties, int32 MeshIndex);

	void CreateVortex(AActor* Owner,  FVortexProperties VTProperties, int32 MeshIndex, USplineComponent* CurrentSplineComponent);

	void CreateVortexInRange(AActor* Owner,  FVortexProperties VTProperties, int32 MeshIndex,  USplineComponent* CurrentSplineComponent, float CurrentDistance, float EnterDistance);

	void DestroyVortexOutOfRange(float CurrentDistance, float ExitDistance);

	void UpdateDynamicMaterial(FVortexProperties VTProperties, USplineComponent* CurrentSplineComponent, float RelativeStartDistance, float RelativeEndDistance);
	void UpdateScalarProperty(int32 MaterialIndex, FString PropertyName, float PropertyValue);
	void UpdateColorProperty(int32 MaterialIndex, FString PropertyName, FLinearColor PropertyValue);

	void DestroyVortex();

	FVortexComponent(): MeshComponent(nullptr), DistanceAlongSpline(0), CurrentVMD(nullptr), NextVMD(nullptr),
	                    VMDAlpha(0), StartDistance(0),
	                    EndDistance(0),
	                    PreviousTimeIndex(0),
	                    CurrentTimeIndex(0),
	                    GroupEntered(false),
	                    GroupExited(false)
	{
		CurrentVortexData = nullptr;
		MeshDirection = FVector(1, 0, 0);
	}


	friend bool operator==(const FVortexComponent& first, const FVortexComponent& second)
	{
		return (first.MeshComponent ==  second.MeshComponent);
	}

	void GetMaterialParamKeys(UMaterialInterface* SourceMaterial, TArray<FString>& SourceKeys, bool bIsScalar)
	{
		if(SourceMaterial)
		{
			TArray<FMaterialParameterInfo> ParameterInfo;
			TArray<FGuid> ParameterGuids;
			if(bIsScalar)
			{
				
				SourceMaterial->GetAllScalarParameterInfo(ParameterInfo, ParameterGuids);

				for (int iIndex = 0; iIndex < ParameterInfo.Num(); iIndex++)
				{
					float ParameterValue;
					if(SourceMaterial->GetScalarParameterValue(ParameterInfo[iIndex], ParameterValue, true))
					{
						SourceKeys.Add(ParameterInfo[iIndex].Name.ToString());
					
					}
				}
				
			}
			else
			{
				SourceMaterial->GetAllVectorParameterInfo(ParameterInfo, ParameterGuids);

				for (int iIndex = 0; iIndex < ParameterInfo.Num(); iIndex++)
				{
					FLinearColor ParameterValue;
					if(SourceMaterial->GetVectorParameterValue(ParameterInfo[iIndex], ParameterValue, true))
					{
						SourceKeys.Add(ParameterInfo[iIndex].Name.ToString());
					
					}
				}
				
			}
		}
	
	}
	
	void GetCommonProperties(TArray<FString>& CommonPropeties, bool bIsScalar)
	{
		TArray<FString> SourceKeys;
		GetMaterialParamKeys(CurrentVMD, SourceKeys, bIsScalar);

		TArray<FString> TargetKeys;
		GetMaterialParamKeys(NextVMD, TargetKeys, bIsScalar);
		
		for (auto& Key : SourceKeys)
		{
			if(TargetKeys.Contains(Key))
			{
				CommonPropeties.Add(Key);
			}
		}
	}

	void GetUniqueProperties(TArray<FString>& UniqueProperties, bool bTarget, bool bIsScalar)
	{
		TArray<FString> SourceKeys;
		TArray<FString> TargetKeys;
		if(bTarget)
		{
			GetMaterialParamKeys(NextVMD, SourceKeys, bIsScalar);
			GetMaterialParamKeys(CurrentVMD, TargetKeys, bIsScalar);
		}
		else
		{
			GetMaterialParamKeys(CurrentVMD, SourceKeys, bIsScalar);
			GetMaterialParamKeys(NextVMD, TargetKeys, bIsScalar);
		}
		
		for (auto& Key : SourceKeys)
		{
			if(!TargetKeys.Contains(Key))
			{
				UniqueProperties.Add(Key);
			}
		}
	}

	

};


USTRUCT(BlueprintType, Category = "Vortex")
struct FSimpleScalarProperty
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Scalar Properties")
	int32 MaterialIndex;
	
	UPROPERTY(EditAnywhere, Category = "Scalar Properties")
	FString Name;

	UPROPERTY(EditAnywhere, Category = "Scalar Properties")
	float Value;

	FSimpleScalarProperty(): MaterialIndex(0), Value(0)
	{
	}
};

USTRUCT(BlueprintType, Category = "Vortex")
struct FSimpleColorProperty
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Scalar Properties")
	int32 MaterialIndex;

	UPROPERTY(EditAnywhere, Category = "Scalar Properties")
	FString Name;

	UPROPERTY(EditAnywhere, Category = "Color Properties")
	FLinearColor Value;

	UPROPERTY(EditAnywhere, Category = "Color Properties")
	float Scale;

	FSimpleColorProperty(): MaterialIndex(0), Scale(1)
	{
		Value = FLinearColor(1, 1, 1);
	}
};


USTRUCT(BlueprintType, Category = "Vortex")
struct FVortexScalarProperty
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Scalar Properties")
	FName Name;

	UPROPERTY(EditAnywhere, Category = "Scalar Properties")
	float MinValue;

	UPROPERTY(EditAnywhere, Category = "Scalar Properties")
	float MaxValue;

	UPROPERTY(EditAnywhere, Category = "Scalar Properties")
	float MaxAlpha;

	FVortexScalarProperty(): MinValue(0), MaxValue(0)
	{
		MaxAlpha = 1;
	}
};

USTRUCT(BlueprintType, Category = "Vortex")
struct FVortexColorProperty
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Scalar Properties")
	FName Name;

	UPROPERTY(EditAnywhere, Category = "Color Properties")
	FLinearColor MinValue;

	UPROPERTY(EditAnywhere, Category = "Color Properties")
	FLinearColor MaxValue;

	UPROPERTY(EditAnywhere, Category = "Color Properties")
	float MaxAlpha;
	
	FVortexColorProperty()
	{
		MaxAlpha = 1;
		MinValue = FLinearColor(0, 0, 0);
		MaxValue = FLinearColor(1, 1, 1);
	}
};


UCLASS()
class PROCEDURALVORTEXTUNNEL_API UPVTLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	static FString LoadAssetPath(FString FullName);
	static void GetMaterialIDMap(AActor* Owner, TArray<FVortexMaterialData> Materials, TMap<int32, UMaterialInterface*>& MaterialIDMap);
	static UMaterialInstanceDynamic* FindOrAssignDynamicMaterial(AActor* Owner, UMaterialInterface* SourceMaterial, TMap<UMaterialInterface*, UMaterialInstanceDynamic*> DynamicMaterialMap);
	
};
