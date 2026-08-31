using UnrealBuildTool;

public class GF_Edificios : ModuleRules
{
    public GF_Edificios(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	 bUseUnity = false;
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "AlsasuaCore",
            "AlsasuaKernel", "AlsasuaWorld", "AlsasuaContracts",
            "ProceduralMeshComponent"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Json",
            "JsonUtilities"
        });
    }
}
