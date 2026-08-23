using UnrealBuildTool;

public class AlsasuaGameplay : ModuleRules
{
    public AlsasuaGameplay(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine", "InputCore", "Json", "JsonUtilities",
            "AlsasuaCore", "AlsasuaWorld", "AlsasuaEntities", "AlsasuaKernel", "GF_Clima", "Niagara",
            "AIModule", "NavigationSystem"
        });
    }
}
