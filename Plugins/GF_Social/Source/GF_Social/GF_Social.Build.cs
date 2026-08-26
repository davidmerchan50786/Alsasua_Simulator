using UnrealBuildTool;
public class GF_Social : ModuleRules
{
    public GF_Social(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	 bUseUnity = false;
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PublicDependencyModuleNames.AddRange(new string[] {
			"AlsasuaKernel", "Core", "CoreUObject", "Engine", "AlsasuaCore", "AlsasuaGameplay", "GF_Politica" });
        PrivateDependencyModuleNames.AddRange(new string[] { "Json", "JsonUtilities" });
    }
}
