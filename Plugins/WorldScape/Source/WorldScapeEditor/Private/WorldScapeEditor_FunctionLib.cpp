// Fill out your copyright notice in the Description page of Project Settings.


#include "WorldScapeEditor_FunctionLib.h"

#include "Landscape.h"
#include "Engine/World.h"
#include "LandscapeInfo.h"
#include "LandscapeLayerInfoObject.h"

#include "Math/UnrealMathUtility.h"
#include "LandscapeEdit.h"
#include "LandscapeProxy.h"
#include "LandscapeDataAccess.h"
#include "Engine/TextureRenderTarget2D.h"
//#include "MeshDescription.h"//


ALandscape* UWorldScapeEditor_FunctionLib::WS_CreateLandscape(const FTransform& LandscapeTransform, const int32& SectionSize, const int32& SectionsPerComponent, const int32& ComponentCountX, const int32& ComponentCountY)
{
	int32 QuadsPerComponent = SectionSize * SectionsPerComponent;

	int32 SizeX = ComponentCountX * QuadsPerComponent + 1;
	int32 SizeY = ComponentCountY * QuadsPerComponent + 1;

	TArray<FLandscapeImportLayerInfo> MaterialImportLayers;
	MaterialImportLayers.Reserve(0);


	TMap<FGuid, TArray<uint16>> HeightDataPerLayers;
	TMap<FGuid, TArray<FLandscapeImportLayerInfo>> MaterialLayerDataPerLayers;


	TArray<uint16> HeightData;
	HeightData.SetNum(SizeX * SizeY);
	for (int32 i = 0; i < HeightData.Num(); i++)
	{
		HeightData[i] = 32768;
	}

	HeightDataPerLayers.Add(FGuid(), MoveTemp(HeightData)); /*ENewLandscapePreviewMode.NewLandscape*/
	// ComputeHeightData will also modify/expand material layers data, which is why we create MaterialLayerDataPerLayers after calling ComputeHeightData
	MaterialLayerDataPerLayers.Add(FGuid(), MoveTemp(MaterialImportLayers));

	//FScopedTransaction Transaction(TEXT("Undo", "Creating New Landscape"));

	UWorld* World = nullptr;
	{
		// We want to create the landscape in the landscape editor mode's world
		FWorldContext& EditorWorldContext = GEditor->GetEditorWorldContext();
		World = EditorWorldContext.World();
	}

	ALandscape* Landscape = World->SpawnActor<ALandscape>();
	//Landscape->bCanHaveLayersContent = false;
	Landscape->LandscapeMaterial = nullptr;

	Landscape->SetActorTransform(LandscapeTransform);

	// automatically calculate a lighting LOD that won't crash lightmass (hopefully)
	// < 2048x2048 -> LOD0
	// >=2048x2048 -> LOD1
	// >= 4096x4096 -> LOD2
	// >= 8192x8192 -> LOD3


	//const FGuid& InGuid, int32 InMinX, int32 InMinY, int32 InMaxX, int32 InMaxY, int32 InNumSubsections, int32 InSubsectionSizeQuads, const TMap<FGuid, TArray<uint16>>& InImportHeightData,
	//	const TCHAR* const InHeightmapFileName, const TMap<FGuid, TArray<FLandscapeImportLayerInfo>>& InImportMaterialLayerInfos, ELandscapeImportAlphamapType InImportMaterialLayerType, const TArray<struct FLandscapeLayer>* InImportLayers = nullptr

	//Landscape->Import(FGuid::NewGuid(), 0, 0, SizeX - 1, SizeY - 1, SectionsPerComponent, QuadsPerComponent, HeightDataPerLayers, nullptr, MaterialLayerDataPerLayers, ELandscapeImportAlphamapType::Additive);

	Landscape->StaticLightingLOD = FMath::DivideAndRoundUp(FMath::CeilLogTwo((SizeX * SizeY) / (2048 * 2048) + 1), (uint32)2);
	// Register all the landscape components
	ULandscapeInfo* LandscapeInfo = Landscape->GetLandscapeInfo();

	LandscapeInfo->UpdateLayerInfoMap(Landscape);


	Landscape->RegisterAllComponents();

	// Need to explicitly call PostEditChange on the LandscapeMaterial property or the landscape proxy won't update its material
	FPropertyChangedEvent MaterialPropertyChangedEvent(FindFieldChecked< FProperty >(Landscape->GetClass(), FName("LandscapeMaterial")));
	Landscape->PostEditChangeProperty(MaterialPropertyChangedEvent);
	Landscape->PostEditChange();


	return Landscape;
}

int32 UWorldScapeEditor_FunctionLib::WS_GetSectionSize(const ALandscape* Landscape)
{
//	ULandscapeInfo* LandscapeInfo = Landscape->GetLandscapeInfo();
	if (Landscape == nullptr)
	{
		return 0;
	}
	return  Landscape->ComponentSizeQuads;
}

int32 UWorldScapeEditor_FunctionLib::WS_GetSubSectionSize(const ALandscape* Landscape)
{
//	ULandscapeInfo* LandscapeInfo = Landscape->GetLandscapeInfo();
	if (Landscape == nullptr)
	{
		return 0;
	}
	return  Landscape->SubsectionSizeQuads;
}

int32 UWorldScapeEditor_FunctionLib::WS_GetNumSubSectionSize(const ALandscape* Landscape)
{
//	ULandscapeInfo* LandscapeInfo = Landscape->GetLandscapeInfo();
	if (Landscape == nullptr)
	{
		return 0;
	}
	return  Landscape->NumSubsections;
}

ULandscapeInfo* UWorldScapeEditor_FunctionLib::WS_GetLandscapeInfo( const ALandscape* Landscape)
{
	ULandscapeInfo* LandscapeInfo = Landscape->GetLandscapeInfo();
	return  LandscapeInfo;
}

//bool UWorldScapeEditor_FunctionLib::WS_ExportLandscapeToRawMesh(const ALandscapeProxy* Landscape, int32 InExportLOD, FMeshDescription& OutRawMesh)//
//{
//	ALandscapeProxy::FRawMeshExportParams Params;
//	Params.ExportLOD = InExportLOD;

//	return Landscape->ExportToRawMesh(Params, OutRawMesh);
//}

