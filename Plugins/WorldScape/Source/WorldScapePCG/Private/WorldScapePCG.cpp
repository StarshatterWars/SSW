// Copyright 2021 IOLACORP STUDIO. All Rights Reserved

#include "WorldScapePCG.h"

#define LOCTEXT_NAMESPACE "FWorldScapePCGModule"

DEFINE_LOG_CATEGORY(LogWorldScapePCG);

void FWorldScapePCGModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file
	UE_LOG(LogWorldScapePCG, Log, TEXT("WorldScapePCG module started"));
}

void FWorldScapePCGModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	UE_LOG(LogWorldScapePCG, Log, TEXT("WorldScapePCG module shutdown"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FWorldScapePCGModule, WorldScapePCG)