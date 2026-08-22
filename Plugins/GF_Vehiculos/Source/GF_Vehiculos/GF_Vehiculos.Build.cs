using UnrealBuildTool;
public class GF_Vehiculos : ModuleRules
{
    public GF_Vehiculos(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "AlsasuaCore", "Niagara", "NavigationSystem", "AIModule" });
    }
}
