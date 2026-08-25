using UnrealBuildTool;
public class GF_Debug : ModuleRules
{
    public GF_Debug(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	 bUseUnity = false;
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "AlsasuaCore", "UMG", "Slate", "SlateCore", "InputCore", "GF_Politica", "GF_NPCs", "GF_Social", "GF_Dialogos", "GF_Misiones", "GF_Systems", "GF_Core" });
    }
}
