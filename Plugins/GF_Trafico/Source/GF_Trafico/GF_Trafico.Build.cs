using UnrealBuildTool;

public class GF_Trafico : ModuleRules
{
	public GF_Trafico(ReadOnlyTargetRules Target) : base(Target)
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
			"AlsasuaKernel",
			"AlsasuaContracts",
			"GF_Carreteras",
			"GF_World"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Json",
			"JsonUtilities",
			"AssetRegistry"
		});
	}
}
