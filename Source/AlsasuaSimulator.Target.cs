using UnrealBuildTool;
using System.Collections.Generic;

public class AlsasuaSimulatorTarget : TargetRules
{
    public AlsasuaSimulatorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.AddRange(new string[] {
            "AlsasuaKernel", "AlsasuaContracts",
            "AlsasuaCore",
            "AlsasuaWorld",
            "AlsasuaEntities",
            "AlsasuaGameplay",
            "AlsasuaUI",
            "AlsasuaSimulator"
        });
        CppStandard = CppStandardVersion.Cpp20;
    }
}
