using UnrealBuildTool;
public class GF_Audio : ModuleRules
{
    public GF_Audio(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "AlsasuaCore", "AlsasuaContracts", "AlsasuaKernel", "Niagara" });
    }
}
