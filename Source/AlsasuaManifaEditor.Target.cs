using UnrealBuildTool;
using System.Collections.Generic;

public class AlsasuaManifaEditorTarget : TargetRules
{
    public AlsasuaManifaEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("AlsasuaManifa");
        CppStandard = CppStandardVersion.Cpp20;
    }
}
