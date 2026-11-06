// Copyright 2021 IOLACORP STUDIO. All Rights Reserved

#include "WorldScapePCGBlueprintLibrary.h"
#include "WorldScapeTestPCGNode.h"
#include "WorldScapeCore/Public/WorldScapeRoot.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Modules/ModuleManager.h"

UWorldScapeTestPCGNodeSettings* UWorldScapePCGBlueprintLibrary::CreateWorldScapeTestPCGNode(UObject* Outer)
{
	if (!Outer)
	{
		Outer = GetTransientPackage();
	}

	UWorldScapeTestPCGNodeSettings* NewSettings = NewObject<UWorldScapeTestPCGNodeSettings>(Outer);
	return NewSettings;
}

TArray<AWorldScapeRoot*> UWorldScapePCGBlueprintLibrary::GetAllWorldScapeRoots(const UObject* WorldContext)
{
	TArray<AWorldScapeRoot*> WorldScapeRoots;

	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull))
	{
		for (TActorIterator<AWorldScapeRoot> ActorIterator(World); ActorIterator; ++ActorIterator)
		{
			AWorldScapeRoot* WorldScapeRoot = *ActorIterator;
			if (IsValid(WorldScapeRoot))
			{
				WorldScapeRoots.Add(WorldScapeRoot);
			}
		}
	}

	return WorldScapeRoots;
}

bool UWorldScapePCGBlueprintLibrary::IsWorldScapePCGAvailable()
{
	return FModuleManager::Get().IsModuleLoaded("WorldScapePCG");
}

FString UWorldScapePCGBlueprintLibrary::GetWorldScapePCGVersion()
{
	return TEXT("1.0.0");
}