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
			"AlsasuaKernel",
			"GF_Clima",
			"GF_Trafico",
			"GF_Audio",
			"GF_NPCs",
			"GF_Dialogos"
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
