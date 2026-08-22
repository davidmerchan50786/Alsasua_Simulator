using UnrealBuildTool;

public class AlsasuaWorld : ModuleRules
{
    public AlsasuaWorld(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "AlsasuaCore", "AlsasuaManifa", "GF_Vegetacion", "GF_Clima", "GF_Trafico", "GF_Edificios", "GF_Carreteras", "GF_Audio", "GF_Ferrocarril", "ProceduralMeshComponent", "NavigationSystem", "Json", "JsonUtilities" });

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "AssetTools", "MaterialEditor", "EditorScriptingUtilities" });
        }
    }
}
