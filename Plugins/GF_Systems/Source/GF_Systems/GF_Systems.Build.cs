using UnrealBuildTool;
public class GF_Systems : ModuleRules
{
    public GF_Systems(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	 bUseUnity = false;
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PublicDependencyModuleNames.AddRange(new string[] {
			"AlsasuaKernel", "Core", "CoreUObject", "Engine", "AlsasuaCore", "Niagara", "GF_Politica", "GF_Social", "GF_Dialogos", "GF_NPCs", "GF_Vehiculos" });
    }
}
