using UnrealBuildTool;

public class GF_Clima : ModuleRules
{
	public GF_Clima(ReadOnlyTargetRules Target) : base(Target)
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
			// Los contratos que este plugin implementa, y el registro donde los
			// publica. Kernel no depende de GF_Clima: sin ciclo.
			"AlsasuaContracts",
			"AlsasuaKernel"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Json",
			"JsonUtilities"
		});
	}
}
