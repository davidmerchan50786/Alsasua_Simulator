using UnrealBuildTool;
public class GF_World : ModuleRules
{
    public GF_World(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "AlsasuaCore", "Niagara", "ProceduralMeshComponent", "GF_Clima", "Json", "JsonUtilities" });
    }
}
