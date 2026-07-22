using UnrealBuildTool;
using System.Collections.Generic;

public class AlsasuaSimulatorEditorTarget : TargetRules
{
    public AlsasuaSimulatorEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.AddRange(new string[] {
            "AlsasuaManifa",
            "AlsasuaCore",
            "AlsasuaWorld",
            "AlsasuaEntities",
            "AlsasuaGameplay",
            "AlsasuaUI",
            "AlsasuaEditor",
            "AlsasuaSimulator"
        });
        CppStandard = CppStandardVersion.Cpp20;
    }
}
