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
            "AlsasuaKernel", "AlsasuaContracts",
            "AlsasuaCore",
            "AlsasuaWorld",
            "AlsasuaEntities",
            "AlsasuaGameplay",
            "AlsasuaUI",
            "AlsasuaEditor",
            "AlsasuaSimulator",
            "GF_Abilities", "GF_AI", "GF_Audio", "GF_Carreteras", "GF_Clima",
            "GF_Core", "GF_Debug", "GF_Dialogos", "GF_Edificios", "GF_Ferrocarril",
            "GF_NPCs", "GF_Optimization", "GF_Politica", "GF_Social",
            "GF_Systems", "GF_Trafico", "GF_UI", "GF_Vegetacion", "GF_Vehiculos",
            "GF_World"
        });
        CppStandard = CppStandardVersion.Cpp20;
    }
}
