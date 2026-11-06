// Copyright 2021 IOLACORP STUDIO. All Rights Reserved


using System.IO;				
using UnrealBuildTool;

public class WorldScapeEditor : ModuleRules
{
	public WorldScapeEditor(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = PCHUsageMode.NoPCHs;
#if UE_5_2_OR_LATER

#else
        bEnforceIWYU = true;
#endif
        bLegacyPublicIncludePaths = false;

#if UE_4_24_OR_LATER
        bUseUnity = false;
#else
        bFasterWithoutUnity = true;
#endif
		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));
        PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private"));																																	  
        // For raytracing
        PrivateIncludePaths.Add(EngineDirectory + "/Shaders/Shared");
        // For HLSL translator
        PrivateIncludePaths.Add(EngineDirectory + "/Source/Runtime/Engine/Private");

        PublicDependencyModuleNames.AddRange(
			new string[]
			{
                "Core",
                "CoreUObject",
                "Engine",
                "UnrealEd",
                "Foliage",
                "Landscape"
    }
        );


        PrivateDependencyModuleNames.AddRange(
            new string[]
             {
                "Core",
                "MeshDescription",//
                "StaticMeshDescription",//
                "RawMesh"//
            });


        DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
