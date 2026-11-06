// Copyright 2021 IOLACORP STUDIO. All Rights Reserved

using System.IO;
using UnrealBuildTool;

public class WorldScapePCG : ModuleRules
{
	public WorldScapePCG(ReadOnlyTargetRules Target) : base(Target)
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

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"PCG"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"PCG",
				"WorldScapeCore",
				"WorldScapeCommon",
				"WorldScapeNoise",
				"WorldScapeVolume",
				"WorldScapeFoliages"
			}
		);

		if (Target.Type == TargetType.Editor)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"UnrealEd",
					"BlueprintGraph",
					"KismetCompiler",
					"ToolMenus",
					"EditorStyle",
					"EditorWidgets",
					"GraphEditor",
					"PropertyEditor",
					"Slate",
					"SlateCore",
					"UMG",
					"WorldScapeEditor",
					"WorldScapeFactory"
				}
			);
		}

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
		);
	}
}