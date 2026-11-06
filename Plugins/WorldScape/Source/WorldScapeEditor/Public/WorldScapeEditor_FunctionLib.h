// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Editor.h"
//#include "MeshDescription.h"//
#include "WorldScapeEditor_FunctionLib.generated.h"

struct FMeshDescription;//

/**
 * 
 */
UCLASS()
class WORLDSCAPEEDITOR_API UWorldScapeEditor_FunctionLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "WorldScape_Function")
		static ALandscape* WS_CreateLandscape(const FTransform& LandscapeTransform, const int32& SectionSize, const int32& SectionsPerComponent, const int32& ComponentCountX, const int32& ComponentCountY);

	UFUNCTION(BlueprintCallable, Category = "WorldScape_Function")
		static int32 WS_GetSectionSize(const ALandscape* Landscape);

	UFUNCTION(BlueprintCallable, Category = "WorldScape_Function")

		static int32 WS_GetSubSectionSize(const ALandscape* Landscape);
	UFUNCTION(BlueprintCallable, Category = "WorldScape_Function")

		static int32 WS_GetNumSubSectionSize(const ALandscape* Landscape);

	UFUNCTION(BlueprintCallable, Category = "WorldScape_Function")
		static ULandscapeInfo* WS_GetLandscapeInfo( const ALandscape* Landscape);

//	UFUNCTION(BlueprintCallable, Category = "WorldScape_Function")//
//		static bool WS_ExportLandscapeToRawMesh(const ALandscapeProxy* Landscape, int32 InExportLOD, FMeshDescription& OutRawMesh);//
};
