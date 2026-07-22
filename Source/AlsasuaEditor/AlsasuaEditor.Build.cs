using UnrealBuildTool;

public class AlsasuaEditor : ModuleRules
{
    public AlsasuaEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine",
            "UnrealEd", "Blutility", "MaterialEditor", "EditorScriptingUtilities", "Landscape",
            "AlsasuaCore", "AlsasuaWorld"
        });
    }
}
