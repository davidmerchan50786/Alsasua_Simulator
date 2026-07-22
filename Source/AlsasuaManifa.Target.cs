using UnrealBuildTool;
using System.Collections.Generic;

public class AlsasuaManifaTarget : TargetRules
{
    public AlsasuaManifaTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("AlsasuaManifa");
        CppStandard = CppStandardVersion.Cpp20;
    }
}
