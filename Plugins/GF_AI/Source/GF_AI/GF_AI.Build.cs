using UnrealBuildTool;
public class GF_AI : ModuleRules
{
    public GF_AI(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PublicDependencyModuleNames.AddRange(new string[] {
			"AlsasuaKernel", "Core", "CoreUObject", "Engine", "AlsasuaCore", "GF_NPCs", "NavigationSystem", "AIModule", "Niagara", "GF_Social" });
    }
}
