// Copyright 2018-2025 Athian Games. All Rights Reserved. 

using UnrealBuildTool;

public class ProceduralVortexTunnel : ModuleRules
{
	public ProceduralVortexTunnel(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core", "Niagara",
		}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject", 
				"Engine",
				"Slate",
				"SlateCore", "RenderCore", "Projects", "ProceduralMeshComponent"
			}
			);

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("EditorStyle");
			PublicDependencyModuleNames.Add("LevelEditor");
			PublicDependencyModuleNames.Add("UnrealEd");

		}



		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
