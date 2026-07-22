using UnrealBuildTool;

public class AlsasuaUI : ModuleRules
{
    public AlsasuaUI(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine", "UMG", "Slate", "SlateCore",
            "AlsasuaCore", "AlsasuaWorld", "AlsasuaEntities", "AlsasuaGameplay", "AlsasuaManifa", "GameplayAbilities"
        });
    }
}
