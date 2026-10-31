// Copyright 2018-2025 Athian Games. All Rights Reserved. 


#include "PVTLibrary.h"
#include "VortexTunnelActor.h"
#include "VortexData.h"
#include "Kismet/KismetMathLibrary.h"

void FVortexComponent::UpdateVortexMesh(FVortexProperties VTProperties, USplineComponent* CurrentSplineComponent)
{
	if (MeshComponent && CurrentSplineComponent)
	{
		DistanceAlongSpline = StartDistance;
		FVector VortexStartLocation = CurrentSplineComponent->GetLocationAtDistanceAlongSpline(StartDistance, ESplineCoordinateSpace::Type::Local);

		FVector VortexStartTangent = CurrentSplineComponent->GetTangentAtDistanceAlongSpline(StartDistance, ESplineCoordinateSpace::Type::Local);
		VortexStartTangent.Normalize();
		VortexStartTangent *= EndDistance - StartDistance;

		MeshDirection = CurrentSplineComponent->GetTangentAtDistanceAlongSpline(EndDistance, ESplineCoordinateSpace::Type::Local);
		MeshDirection -= CurrentSplineComponent->GetTangentAtDistanceAlongSpline(StartDistance, ESplineCoordinateSpace::Type::Local);
		MeshDirection.Normalize();
		FVector VortexEndLocation = CurrentSplineComponent->GetLocationAtDistanceAlongSpline(EndDistance, ESplineCoordinateSpace::Type::Local);

		FVector VortexEndTangent = CurrentSplineComponent->GetTangentAtDistanceAlongSpline(EndDistance, ESplineCoordinateSpace::Type::Local);
		VortexEndTangent.Normalize();
		VortexEndTangent *= EndDistance - StartDistance;
			
		MeshComponent->SetStartAndEnd(VortexStartLocation, VortexStartTangent, VortexEndLocation, VortexEndTangent, false);

		UpdateVortexMeshScalingAtIndex(VTProperties);

	}
}

void FVortexComponent::UpdateVortexMeshScalingAtIndex(FVortexProperties VTProperties)
{
	if (MeshComponent)
	{
		FVector2D StartScale = FVector2D(VTProperties.RadiusScale, VTProperties.RadiusScale);
		FVector2D EndScale = FVector2D(VTProperties.RadiusScale, VTProperties.RadiusScale);

		FRichCurve* RichCurve = VTProperties.ScaleDistributionCurve.GetRichCurve();

		float PreviousCurveValue = RichCurve->Eval(PreviousTimeIndex);
		StartScale.X *= PreviousCurveValue;
		StartScale.Y *= PreviousCurveValue;

		float CurrentCurveValue = RichCurve->Eval(CurrentTimeIndex);
		EndScale.X *= CurrentCurveValue;
		EndScale.Y *= CurrentCurveValue;

		MeshComponent->SetStartScale(StartScale, false);
		MeshComponent->SetEndScale(EndScale, false);
		
	}


}

bool FVortexComponent::CreateSingleSplineMesh(AActor* Owner, FVortexProperties VTProperties, int32 MeshIndex)
{
	if (Owner && CurrentMeshInfo.Mesh && !GIsSavingPackage)
	{
		FString MeshName = "VortexMesh_" + FString::FromInt(MeshIndex);
		MeshComponent = NewObject<USplineMeshComponent>(Owner, USplineMeshComponent::StaticClass(), *MeshName, RF_Transactional);
		MeshComponent->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		MeshComponent->SetStaticMesh(CurrentMeshInfo.Mesh);
		MeshComponent->RegisterComponentWithWorld(Owner->GetWorld());
		MeshComponent->CreationMethod = EComponentCreationMethod::Instance;
		MeshComponent->SetMobility(EComponentMobility::Movable);
		MeshComponent->SetForwardAxis(VTProperties.SplineMeshAxis, false);
		MeshComponent->SetRelativeRotation(FRotator(0, 0, 0));

		if(VTProperties.bEnableCollision)
		{
			MeshComponent->SetCollisionEnabled(ECollisionEnabled::Type::QueryAndPhysics);
		}
		else
		{
			MeshComponent->SetCollisionEnabled(ECollisionEnabled::Type::QueryOnly);
		}
		
		return true;
			
	}

	return false;

}

void FVortexComponent::CreateVortex(AActor* Owner,  FVortexProperties VTProperties, int32 MeshIndex,
                                    USplineComponent* CurrentSplineComponent)
{
	
	if (CreateSingleSplineMesh(Owner, VTProperties, MeshIndex))
	{
				
		UpdateVortexMesh(VTProperties, CurrentSplineComponent);
		
		if(!GIsSavingPackage)
		{
			this->MeshComponent->SetWorldLocation(CurrentSplineComponent->GetOwner()->GetActorLocation());
			this->MeshComponent->UpdateMesh();
		}
	}
	
}

void FVortexComponent::CreateVortexInRange(AActor* Owner, FVortexProperties VTProperties, int32 MeshIndex,
	USplineComponent* CurrentSplineComponent, float CurrentDistance, float EnterDistance)
{
	if(Owner)
	{
		if(!GroupEntered && CurrentDistance + EnterDistance >= StartDistance)
		{
			GroupEntered = true;
			CreateVortex(Owner, VTProperties, MeshIndex, CurrentSplineComponent);
		}
	}
}

void FVortexComponent::DestroyVortexOutOfRange(float CurrentDistance, float ExitDistance)
{
	if(!GroupExited && CurrentDistance - ExitDistance >= EndDistance)
	{
		GroupExited = true;
		if(MeshComponent)
		{
			MeshComponent->DestroyComponent();
			MeshComponent = nullptr;
		}
	}
}

void FVortexComponent::DestroyVortex()
{
	if(MeshComponent)
	{
		MeshComponent->DestroyComponent();
		MeshComponent = nullptr;
	}
}

void FVortexComponent::UpdateDynamicMaterial(FVortexProperties VTProperties, USplineComponent* CurrentSplineComponent, float RelativeStartDistance, float RelativeEndDistance)
{

	if(MeshComponent)
	{
		TMap<int32, UMaterialInterface*> MaterialIDMap;
		UPVTLibrary::GetMaterialIDMap(MeshComponent->GetOwner(), CurrentMeshInfo.CustomMaterials, MaterialIDMap);

		if(CurrentVortexData)
		{
			UPVTLibrary::GetMaterialIDMap(MeshComponent->GetOwner(), CurrentVortexData->DefaultMaterials, MaterialIDMap);
		}
	
					
		TArray<int32> MaterialIDs;
		MaterialIDMap.GetKeys(MaterialIDs);

		TArray<FString> UCurrentVMDScalarKeys;
		TArray<FString> UNextVMDScalarKeys;
		TArray<FString> CommonScalarKeys;

		TArray<FString> UCurrentVMDColorKeys;
		TArray<FString> UNextVMDColorKeys;
		TArray<FString> CommonColorKeys;

		UMaterialInterface* VMDMaterial = nullptr;

		if(!CurrentVortexData->bDisableMaterialBlending)
		{
			float splinelength  =  CurrentSplineComponent->GetSplineLength();
			float relstart = RelativeStartDistance * splinelength / 100;
			float relend = RelativeEndDistance * splinelength / 100;
			float relativelength = relend - relstart;

			if(relativelength > 0)
			{
				if(relstart <= StartDistance && StartDistance <= relend)
				{
					float DesiredVMDIndexAlpha;
					float DistanceRatio = (StartDistance -  relstart) / relativelength;

					if(CurrentVortexData->bEqualBlendDistribution)
					{
						int32 NumVMDs = CurrentVortexData->MaterialsToBlend.Num();
						DesiredVMDIndexAlpha = DistanceRatio * NumVMDs;
					}
					else
					{
						FRichCurve* RichCurve = CurrentVortexData->MBDistributionCurve.GetRichCurve();
						DesiredVMDIndexAlpha = RichCurve->Eval(DistanceRatio * 100);
					}

					DesiredVMDIndexAlpha = FMath::Max(0.0f, DesiredVMDIndexAlpha);
			
					int32 VMDCurrentIndex = FMath::FloorToInt(DesiredVMDIndexAlpha);
					int32 VMDNextIndex = FMath::CeilToInt(DesiredVMDIndexAlpha);

					if(CurrentVortexData->MaterialsToBlend.Num() > VMDCurrentIndex)
					{
						CurrentVMD = CurrentVortexData->MaterialsToBlend[VMDCurrentIndex];

						if(CurrentVortexData->MaterialsToBlend.Num() > VMDNextIndex)
						{
							NextVMD = CurrentVortexData->MaterialsToBlend[VMDNextIndex];
							VMDAlpha = FMath::Clamp(DesiredVMDIndexAlpha - VMDCurrentIndex, 0.0f, 1.0f);
						}
						else
						{
							NextVMD = CurrentVMD;
							VMDAlpha = 0;
						}
					}

					if(CurrentVMD && NextVMD)
					{
						TArray<FMaterialParameterInfo> ParameterInfo;
						TArray<FGuid> ParameterGuids;
						CurrentVMD->GetAllScalarParameterInfo(ParameterInfo, ParameterGuids);

						for (int iIndex = 0; iIndex < ParameterInfo.Num(); iIndex++)
						{
							float ParameterValue;
							if(CurrentVMD->GetScalarParameterValue(ParameterInfo[iIndex], ParameterValue, true))
							{
								if(NextVMD->GetScalarParameterValue(ParameterInfo[iIndex], ParameterValue, true))
								{
									CommonScalarKeys.Add(ParameterInfo[iIndex].Name.ToString());
								}
								else
								{
									UCurrentVMDScalarKeys.Add(ParameterInfo[iIndex].Name.ToString());
								}
							}
						}
	
						GetCommonProperties(CommonScalarKeys, true);
						GetUniqueProperties(UCurrentVMDScalarKeys, false, true);
						GetUniqueProperties(UNextVMDScalarKeys, true, true);

						GetCommonProperties(CommonColorKeys, false);
						GetUniqueProperties(UCurrentVMDColorKeys, false, false);
						GetUniqueProperties(UNextVMDColorKeys, true, false);

						VMDMaterial = VMDAlpha < 0.5 ?  CurrentVMD : NextVMD;

					}
				}
			}
		
				
		}
	
		for (int iIndex = 0; iIndex < MaterialIDs.Num(); iIndex++)
		{
			int CurrentMaterialID = MaterialIDs[iIndex];

			UMaterialInterface* CurrentMaterial = VMDMaterial ? VMDMaterial : MaterialIDMap[CurrentMaterialID];
			UMaterialInstanceDynamic* DynamicMaterial = UPVTLibrary::FindOrAssignDynamicMaterial(MeshComponent->GetOwner(), CurrentMaterial, DynamicMaterialMap);
       
			if (DynamicMaterial)
			{

				auto GetScalarValueFromKey = ([](UMaterialInterface* Material, FString Key)
				{
					float ScalarValue = 0;
					if(Material)
					{
						FMaterialParameterInfo Parameterinfo;
						Parameterinfo.Name = FName(*Key);
						Material->GetScalarParameterValue(Parameterinfo, ScalarValue);
					}

					return ScalarValue;
				});

				auto GetColorValueFromKey = ([](UMaterialInterface* Material, FString Key)
				{
					FLinearColor ColorValue = FLinearColor();
					if(Material)
					{
						FMaterialParameterInfo Parameterinfo;
						Parameterinfo.Name = FName(*Key);
						Material->GetVectorParameterValue(Parameterinfo, ColorValue);
					}

					return ColorValue;
				});
			
				for (auto& ScalarKey : UCurrentVMDScalarKeys)
				{
					float ScalarValue = GetScalarValueFromKey(CurrentVMD, ScalarKey);
					DynamicMaterial->SetScalarParameterValue(FName(*ScalarKey),  ScalarValue);
				}
			
				for (auto& ScalarKey : UNextVMDScalarKeys)
				{
					float ScalarValue = GetScalarValueFromKey(NextVMD, ScalarKey);
					DynamicMaterial->SetScalarParameterValue(FName(*ScalarKey),  ScalarValue);
				}

				for (auto& ScalarKey : CommonScalarKeys)
				{
					float ScalarValue1 = GetScalarValueFromKey(CurrentVMD, ScalarKey);
					float ScalarValue2 = GetScalarValueFromKey(NextVMD, ScalarKey);
					float FinalValue = FMath::Lerp(ScalarValue1, ScalarValue2, VMDAlpha);
					DynamicMaterial->SetScalarParameterValue(FName(*ScalarKey),  FinalValue);
				}
			
				for (auto& ColorKey : UCurrentVMDColorKeys)
				{
					FLinearColor ColorValue = GetColorValueFromKey(CurrentVMD, ColorKey);
					DynamicMaterial->SetVectorParameterValue(FName(*ColorKey),  ColorValue);
				}
			
				for (auto& ColorKey : UNextVMDColorKeys)
				{
					FLinearColor ColorValue = GetColorValueFromKey(NextVMD, ColorKey);
					DynamicMaterial->SetVectorParameterValue(FName(*ColorKey), ColorValue);
				}

				for (auto& ColorKey : CommonColorKeys)
				{
					FLinearColor Value1 = GetColorValueFromKey(CurrentVMD, ColorKey);
					FLinearColor Value2 = GetColorValueFromKey(NextVMD, ColorKey);

					FLinearColor FinalValue = FMath::Lerp(Value1, Value2, VMDAlpha);
					DynamicMaterial->SetVectorParameterValue(FName(*ColorKey),  FinalValue);
				}

				MeshComponent->SetMaterial(CurrentMaterialID, DynamicMaterial);
			}
		}
	}
		
}

void FVortexComponent::UpdateScalarProperty(int32 MaterialIndex, FString PropertyName, float PropertyValue)
{
	if(MeshComponent)
	{
		UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(MeshComponent->GetMaterial(MaterialIndex));
		if(DynamicMaterial)
		{
			DynamicMaterial->SetScalarParameterValue(*PropertyName, PropertyValue);
		}
	}
	
	
}

void FVortexComponent::UpdateColorProperty(int32 MaterialIndex, FString PropertyName, FLinearColor PropertyValue)
{
	if(MeshComponent)
	{
		UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(MeshComponent->GetMaterial(MaterialIndex));
		if(DynamicMaterial)
		{
			DynamicMaterial->SetVectorParameterValue(*PropertyName, PropertyValue);
		}
	}
}

FString UPVTLibrary::LoadAssetPath(FString FullName)
{
	if (FullName.IsEmpty())
	{
		return "";
	}

	FullName.RemoveFromEnd("'");

	FString LeftString;
	FString Right;

	if (FullName.Contains("_C"))
	{
		FullName.Split("_C", &LeftString, &Right, ESearchCase::CaseSensitive, ESearchDir::Type::FromEnd);
	}
	else
	{
		LeftString = FullName;
	}

	FString Left;
	FString FinalString;
	LeftString.Split("/", &Left, &FinalString);
	FinalString = TEXT("/") + FinalString;

	return FinalString;
}

UMaterialInterface* GetMaterialFromInfo(FVortexMaterialData MaterialInfo)
{
	UMaterialInterface* CurrentMaterial = nullptr;
		
	TArray<UMaterialInterface*> ValidMaterials;
	for (int jIndex = 0; jIndex < MaterialInfo.Materials.Num(); jIndex++)
	{
		if(UMaterialInterface* material = MaterialInfo.Materials[jIndex])
		{
			ValidMaterials.Add(material);
		}
			
	}

	if (ValidMaterials.Num() > 0)
	{
		int32 RandomMaterialIndex = FMath::RandRange(0, 1) * (ValidMaterials.Num() - 1);
		CurrentMaterial = ValidMaterials[RandomMaterialIndex];
	}

	return CurrentMaterial;
}

void UPVTLibrary::GetMaterialIDMap(AActor* Owner, TArray<FVortexMaterialData> Materials, TMap<int32, UMaterialInterface*>& MaterialIDMap)
{
	if(Owner)
	{
		for (int iIndex = 0; iIndex < Materials.Num(); iIndex++)
		{
			FVortexMaterialData CurrentMaterialInfo = Materials[iIndex];
			if(!MaterialIDMap.Contains(CurrentMaterialInfo.ID))
			{
				if(UMaterialInterface* CurrentMaterial = GetMaterialFromInfo(CurrentMaterialInfo))
				{
					MaterialIDMap.Add(CurrentMaterialInfo.ID, CurrentMaterial);
				}
			}
		}
		
	}
	
}

UMaterialInstanceDynamic* UPVTLibrary::FindOrAssignDynamicMaterial(AActor* Owner, UMaterialInterface* SourceMaterial, TMap<UMaterialInterface*, UMaterialInstanceDynamic*> DynamicMaterialMap)
{
	
	UMaterialInstanceDynamic* DynamicMaterial = nullptr;
	if(Owner && SourceMaterial)
	{
		if(!DynamicMaterialMap.Contains(SourceMaterial))
		{
			DynamicMaterialMap.Add(SourceMaterial);
		}
		DynamicMaterial = DynamicMaterialMap[SourceMaterial];
		if(!DynamicMaterial || !DynamicMaterial->IsValidLowLevel())
		{
			DynamicMaterial = UMaterialInstanceDynamic::Create(SourceMaterial, Owner);
			DynamicMaterialMap[SourceMaterial] = DynamicMaterial;

			
		}
	}

	return DynamicMaterial;
}
