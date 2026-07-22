using UnrealBuildTool;
public class AlsasuaSimulator : ModuleRules {
    public AlsasuaSimulator(ReadOnlyTargetRules Target) : base(Target) {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bUseUnity = false;
        CppStandard = CppStandardVersion.Cpp20;
        PublicDependencyModuleNames.AddRange(new string[] { 
            "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", 
            "GameplayAbilities", "GameplayTags", "AIModule", "HTTP", "Json", 
            "JsonUtilities", "AudioMixer", "AudioExtensions", "ProceduralMeshComponent",
            "NavigationSystem", "Niagara", "Landscape", "RHI", "RenderCore"
        });
    }
}
