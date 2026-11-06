// Copyright 2021 IOLACORP STUDIO. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/World.h"
#include "WorldScapePCGBlueprintLibrary.generated.h"

class UWorldScapeTestPCGNodeSettings;
class AWorldScapeRoot;

/**
 * Blueprint function library for WorldScape PCG functionality
 */
UCLASS()
class WORLDSCAPEPCG_API UWorldScapePCGBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Create a WorldScape Test PCG Node settings object
	 * @param Outer The outer object for the settings
	 * @return New WorldScape Test PCG Node settings
	 */
	UFUNCTION(BlueprintCallable, Category = "WorldScape PCG", meta = (CallInEditor = "true"))
	static UWorldScapeTestPCGNodeSettings* CreateWorldScapeTestPCGNode(UObject* Outer = nullptr);

	/**
	 * Get all WorldScape Root actors in the current world
	 * @param WorldContext World context object
	 * @return Array of WorldScape Root actors
	 */
	UFUNCTION(BlueprintCallable, Category = "WorldScape PCG", meta = (CallInEditor = "true", WorldContext = "WorldContext"))
	static TArray<AWorldScapeRoot*> GetAllWorldScapeRoots(const UObject* WorldContext);

	/**
	 * Check if the WorldScape PCG module is loaded and available
	 * @return True if the module is available
	 */
	UFUNCTION(BlueprintPure, Category = "WorldScape PCG", meta = (CallInEditor = "true"))
	static bool IsWorldScapePCGAvailable();

	/**
	 * Get the version string of the WorldScape PCG module
	 * @return Version string
	 */
	UFUNCTION(BlueprintPure, Category = "WorldScape PCG", meta = (CallInEditor = "true"))
	static FString GetWorldScapePCGVersion();
};