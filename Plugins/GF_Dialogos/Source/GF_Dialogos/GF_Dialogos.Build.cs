using UnrealBuildTool;
public class GF_Dialogos : ModuleRules
{
    public GF_Dialogos(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	 bUseUnity = false;
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PublicDependencyModuleNames.AddRange(new string[] { "GF_NPCs",  "Core", "CoreUObject", "Engine", "AlsasuaCore", "GameplayTags" });
        PrivateDependencyModuleNames.AddRange(new string[] { "Json", "JsonUtilities" });
    }
}
