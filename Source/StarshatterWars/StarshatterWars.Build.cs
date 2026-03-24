// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class StarshatterWars : ModuleRules
{
    public StarshatterWars(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        bUseRTTI = true;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "EnhancedInput",
            "Slate",
            "SlateCore",
            "UMG",
            "MediaAssets",
            "CommonUI",
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Niagara",
            "NiagaraCore",
            "RenderCore",
            "RHI",
            "DeveloperSettings",
        });

        // Vorbis rules token exists in UE 5.6:
        AddEngineThirdPartyPrivateStaticDependencies(Target, "Vorbis");

        string EnginePath = Path.GetFullPath(Target.RelativeEnginePath);

        // Vorbis headers:
        PublicSystemIncludePaths.Add(Path.Combine(
            EnginePath, "Source", "ThirdParty", "Vorbis", "libvorbis-1.3.2", "include"));

        // Ogg headers (auto-detect version folder):
        string OggRoot = Path.Combine(EnginePath, "Source", "ThirdParty", "Ogg");
        if (Directory.Exists(OggRoot))
        {
            // Find a folder like "libogg-1.x.x"
            string[] Candidates = Directory.GetDirectories(OggRoot, "libogg-*");
            if (Candidates.Length > 0)
            {
                // Pick the first match (good enough; usually only one)
                string OggInclude = Path.Combine(Candidates[0], "include");
                PublicSystemIncludePaths.Add(OggInclude);
            }
        }

        PublicIncludePaths.AddRange(new string[]
        {
            Path.Combine(ModuleDirectory, "System"),
            Path.Combine(ModuleDirectory, "Player"),
            Path.Combine(ModuleDirectory, "Planners"),
            Path.Combine(ModuleDirectory, "Environment"),
            Path.Combine(ModuleDirectory, "Screen"),
            Path.Combine(ModuleDirectory, "Ship"),
            Path.Combine(ModuleDirectory, "Sim"),
            Path.Combine(ModuleDirectory, "AIAgent"),
            Path.Combine(ModuleDirectory, "Game"),
            Path.Combine(ModuleDirectory, "View"),
            Path.Combine(ModuleDirectory, "Display"),
            Path.Combine(ModuleDirectory, "Manager"),
            Path.Combine(ModuleDirectory, "Design"),
            Path.Combine(ModuleDirectory, "Levels"),
            Path.Combine(ModuleDirectory, "Actors"),
            Path.Combine(ModuleDirectory, "Campaign"),
            Path.Combine(ModuleDirectory, "Util"),
            Path.Combine(ModuleDirectory, "Combat"),
            Path.Combine(ModuleDirectory, "Mission"),
        });

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new[]
            {
                "UnrealEd",
                "DataTableEditor",   // depending on UE version; safe to include
                "EditorSubsystem",
                "AssetRegistry",
                "Projects"
            });
        }
    }
}
