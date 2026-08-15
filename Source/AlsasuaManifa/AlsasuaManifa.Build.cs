using UnrealBuildTool;

public class AlsasuaManifa : ModuleRules
{
    public AlsasuaManifa(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // Harden build: enable exceptions for safer C++ semantics
        // Note: bTreatWarningsAsErrors is not available in ModuleRules on this engine version
        bEnableExceptions = true;

        // Estándar AAA: C++20 y sin Unity Builds para detectar errores de include
        CppStandard = CppStandardVersion.Cpp20;
        bUseUnity = false;

        // Ciclo heredado del port Unity: AlsasuaGameplay/AlsasuaUI dependen de este módulo
        // y este de AlsasuaUI (widgets de pausa). UBT 5.8 lo marca como error; se confiesa
        // la arista Manifa->UI (los 3 ciclos pasan por ella) como hacen los módulos del engine.
        CircularlyReferencedDependentModules.Add("AlsasuaUI");

	PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"EnhancedInput", 
			"GameplayAbilities", 
			"GameplayTags", 
			"GameplayTasks",
			"AIModule",
			"NavigationSystem",
			"MotionTrajectory",
			"Landscape",
			"Niagara",
			"ProceduralMeshComponent",
			"AlsasuaCore",
			"AlsasuaUI"
		});

        PrivateDependencyModuleNames.AddRange(new string[] {
            "UMG",
            "Slate",
            "SlateCore",
            "AudioMixer",
            "Json",
            "JsonUtilities",
            // AlsasuaMallaFab busca en el registro las mallas bajadas de Fab.
            "AssetRegistry"
        });
    }
}
