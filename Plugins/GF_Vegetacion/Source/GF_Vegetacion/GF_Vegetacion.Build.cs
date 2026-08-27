using UnrealBuildTool;

public class GF_Vegetacion : ModuleRules
{
	public GF_Vegetacion(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;
		bEnableExceptions = true;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"Landscape",
			"ProceduralMeshComponent",
			"AlsasuaCore",
			"AlsasuaKernel",
			"AlsasuaContracts"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AssetRegistry",
			"Json",
			"JsonUtilities",
			"NavigationSystem"
		});
	}
}
