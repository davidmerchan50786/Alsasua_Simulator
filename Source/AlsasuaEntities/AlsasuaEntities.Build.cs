using UnrealBuildTool;

public class AlsasuaEntities : ModuleRules
{
    public AlsasuaEntities(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "AIModule", "AlsasuaCore", "AlsasuaKernel", "AlsasuaSimulator", "EnhancedInput", "NavigationSystem", "InputCore" });
    }
}
