using UnrealBuildTool;

public class AlsasuaWorld : ModuleRules
{
    public AlsasuaWorld(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "AlsasuaCore", "ProceduralMeshComponent", "NavigationSystem", "Json", "JsonUtilities" });

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "AssetTools", "MaterialEditor" });
        }
    }
}
