using UnrealBuildTool;
public class GF_Misiones : ModuleRules
{
    public GF_Misiones(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
    }
}
