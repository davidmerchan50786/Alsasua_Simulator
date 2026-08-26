using UnrealBuildTool;

public class AlsasuaKernel : ModuleRules
{
	public AlsasuaKernel(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;
		bUseUnity = false;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			// Capa personaje/GAS absorbida de AlsasuaManifa.
			"AlsasuaCore",
			// Interfaces de servicio (IWeatherService, etc.) sin depender del
			// plugin que las implementa — se piden por UAlsasuaServiceRegistry.
			"AlsasuaContracts",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"EnhancedInput",
			"InputCore",
			"UMG",
			"AIModule",
			"Niagara",
			"MotionTrajectory",
			"NavigationSystem",
			// UI nativa del minimapa (NativePaint) y carga de datos JSON.
			"Slate",
			"SlateCore",
			"Json",
			"JsonUtilities"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"GameFeatures",
			"Projects",
			"Sockets"
		});
	}
}
