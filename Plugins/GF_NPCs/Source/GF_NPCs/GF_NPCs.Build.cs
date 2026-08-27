using UnrealBuildTool;
public class GF_NPCs : ModuleRules
{
    public GF_NPCs(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "AlsasuaCore", "AlsasuaKernel", "NavigationSystem", "AIModule", "GF_Social", "GF_Politica", "GF_Carreteras", "Niagara" });
        PrivateDependencyModuleNames.AddRange(new string[] { "Json", "JsonUtilities" });
    }
}
