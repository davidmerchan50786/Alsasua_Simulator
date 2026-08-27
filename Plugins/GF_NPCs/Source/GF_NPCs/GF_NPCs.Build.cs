using UnrealBuildTool;
public class GF_NPCs : ModuleRules
{
    public GF_NPCs(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	 bUseUnity = false;
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PublicDependencyModuleNames.AddRange(new string[] {
			"AlsasuaKernel", "Core", "CoreUObject", "Engine", "AlsasuaCore", "NavigationSystem", "AIModule", "GF_Social", "GF_Politica", "GF_Carreteras", "Niagara" });
        PrivateDependencyModuleNames.AddRange(new string[] { "Json", "JsonUtilities" });
    }
}
