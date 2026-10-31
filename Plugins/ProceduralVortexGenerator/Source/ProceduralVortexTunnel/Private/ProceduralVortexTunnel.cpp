// Copyright 2018-2025 Athian Games. All Rights Reserved. 

#include "ProceduralVortexTunnel.h"
#include "Misc/Paths.h"
#include "Interfaces/IPluginManager.h"
#include "ShaderCore.h"

#define LOCTEXT_NAMESPACE "FProceduralVortexTunnelModule"

void FProceduralVortexTunnelModule::StartupModule()
{
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("ProceduralVortexTunnel"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/VortexTunnel"), PluginShaderDir);
}

void FProceduralVortexTunnelModule::ShutdownModule()
{
  
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FProceduralVortexTunnelModule, ProceduralVortexTunnel) 