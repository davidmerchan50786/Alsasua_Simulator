using UnrealBuildTool;
public class GF_Abilities : ModuleRules
{
    public GF_Abilities(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	 bUseUnity = false;
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PublicDependencyModuleNames.AddRange(new string[] {
			"AlsasuaKernel", "Core", "CoreUObject", "Engine", "AlsasuaCore", "GameplayAbilities", "GameplayTags", "GameplayTasks", "GF_NPCs", "GF_Social" });
    }
}
