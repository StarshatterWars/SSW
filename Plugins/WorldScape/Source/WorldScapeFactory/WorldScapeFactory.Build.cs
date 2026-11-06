// Copyright 2021 IOLACORP STUDIO. All Rights Reserved


using System.IO;				
using UnrealBuildTool;

public class WorldScapeFactory : ModuleRules
{
	public WorldScapeFactory(ReadOnlyTargetRules Target) : base(Target)
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
                 "Json",
                "Slate",
                "SlateCore",
                "Engine",
                "InputCore",
                "UnrealEd",
                "KismetWidgets",
                "Kismet",
                "PropertyEditor",
                "RenderCore",
                "ContentBrowser",
                "WorkspaceMenuStructure",
                "EditorStyle",
                "EditorWidgets",
                "Projects",
                "AssetRegistry",
                "WorldScapeCore",
                "WorldScapeNoise",
                "ClassViewer",
                "SlateCore"
    }
        );


        PrivateDependencyModuleNames.AddRange(
            new string[]
             {
                "Core",
                "Settings",
                "UnrealEd",
                "LevelEditor",
                "WorldScapeCore",
                "WorldScapeFoliages",
                "WorldScapeNoise",
                "WorldScapeVolume"
            });


        DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
               "AssetTools"
			}
            );
	}
}
