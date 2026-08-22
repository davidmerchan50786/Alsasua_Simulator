using UnrealBuildTool;
public class GF_NPCs : ModuleRules
{
    public GF_NPCs(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "AlsasuaCore", "NavigationSystem" });
        PrivateDependencyModuleNames.AddRange(new string[] { "Json", "JsonUtilities" });
    }
}
