using UnrealBuildTool;

public class AlsasuaGameplay : ModuleRules
{
    public AlsasuaGameplay(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine", "InputCore", "Json",
            "AlsasuaCore", "AlsasuaWorld", "AlsasuaEntities", "AlsasuaManifa", "Niagara",
            "AIModule", "NavigationSystem"
        });
    }
}
