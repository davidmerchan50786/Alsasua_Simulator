using UnrealBuildTool;

public class AlsasuaGameplay : ModuleRules
{
    public AlsasuaGameplay(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine", "InputCore", "Json", "JsonUtilities",
            "AlsasuaCore", "AlsasuaKernel", "AlsasuaWorld", "AlsasuaEntities", "AlsasuaManifa", "GF_Clima", "GF_Vehiculos", "GF_NPCs", "Niagara",
            "AIModule", "NavigationSystem"
        });
    }
}
