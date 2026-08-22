using UnrealBuildTool;

public class GF_Trafico : ModuleRules
{
	public GF_Trafico(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;
		bEnableExceptions = true;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"AlsasuaCore",
			"AlsasuaKernel",
			// La red viaria (RedViaria) y las direcciones de fachada
			// (AlsasuaDirecciones) viven en estos dos plugins.
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
