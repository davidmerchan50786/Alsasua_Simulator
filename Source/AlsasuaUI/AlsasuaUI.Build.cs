using UnrealBuildTool;

public class AlsasuaUI : ModuleRules
{
    public AlsasuaUI(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine", "UMG", "Slate", "SlateCore", "InputCore",
            "AlsasuaCore", "AlsasuaWorld", "AlsasuaEntities", "AlsasuaGameplay", "AlsasuaKernel",
            "GF_Social", "GF_Politica", "GF_World", "GF_Systems", "GF_NPCs",
            "GameplayAbilities", "Json", "JsonUtilities"
        });
    }
}
