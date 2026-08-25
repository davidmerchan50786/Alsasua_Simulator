using UnrealBuildTool;
public class GF_Core : ModuleRules
{
    public GF_Core(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	 bUseUnity = false;
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "AlsasuaCore", "AlsasuaKernel", "GF_AI", "GF_Audio", "GF_NPCs", "GF_Optimization" });
    }
}
