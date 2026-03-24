// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "EngineUtils.h" // For TActorIterator
#include "MusicController.h"

/**
 * Get the global instance of AMusicController from the current world.
 * Searches the level for the first valid instance.
 */

inline AMusicController* GetMusicController(UObject* WorldContext)
{
	if (!WorldContext) return nullptr;

	UWorld* World = WorldContext->GetWorld();
	if (!World) return nullptr;

	for (TActorIterator<AMusicController> It(World); It; ++It)
	{
		return *It;
	}

	return nullptr; // notxfound
}