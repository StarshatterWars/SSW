// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class StarshatterWarsTarget : TargetRules
{
	public StarshatterWarsTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
		ExtraModuleNames.Add("StarshatterWars");
		if (Target.Platform == UnrealTargetPlatform.Win64 && ProjectFile != null)
			WindowsPlatform.ApplicationIcon = System.IO.Path.Combine(ProjectFile.Directory.FullName, "Application.ico");
        PreBuildSteps.Add("powershell.exe -NoProfile -ExecutionPolicy Bypass -File \"$(ProjectDir)/Script/Update-BuildVersion.ps1\" -ProjectRoot \"$(ProjectDir)\"");
	}
}
