using UnrealBuildTool;
public class FractalVerseTarget : TargetRules
{
    public FractalVerseTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Server;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
        ExtraModuleNames.Add("StarshatterWars");
        ProjectDefinitions.Add("SSW_FRACTALVERSE=1");
    }
}
