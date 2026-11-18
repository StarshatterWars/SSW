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
        PreBuildSteps.Add("powershell.exe -NoProfile -ExecutionPolicy Bypass -File \"$(ProjectDir)/Script/Update-BuildVersion.ps1\" -ProjectRoot \"$(ProjectDir)\"");
	}
}
