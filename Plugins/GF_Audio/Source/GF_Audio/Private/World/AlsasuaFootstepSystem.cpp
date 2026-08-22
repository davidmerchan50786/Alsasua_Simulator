#include "World/AlsasuaFootstepSystem.h"
#include "Core/AlsasuaProfiling.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Sound/SoundBase.h"

// Inicialización estática
TMap<FString, TWeakObjectPtr<USoundBase>> UAlsasuaFootstepSystem::MaterialAudioMap;

static void EnsureFootstepMapPopulated()
{
    if (UAlsasuaFootstepSystem::MaterialAudioMap.Num() > 0) return;

    auto LoadSound = [](const TCHAR* Path) -> TWeakObjectPtr<USoundBase>
    {
        USoundBase* S = LoadObject<USoundBase>(nullptr, Path);
        return TWeakObjectPtr<USoundBase>(S);
    };

    UAlsasuaFootstepSystem::MaterialAudioMap.Add("Concrete",  LoadSound(TEXT("/Game/Audio/Footsteps/SC_FootstepConcrete.SC_FootstepConcrete")));
    UAlsasuaFootstepSystem::MaterialAudioMap.Add("Dirt",      LoadSound(TEXT("/Game/Audio/Footsteps/SC_FootstepDirt.SC_FootstepDirt")));
    UAlsasuaFootstepSystem::MaterialAudioMap.Add("Grass",     LoadSound(TEXT("/Game/Audio/Footsteps/SC_FootstepGrass.SC_FootstepGrass")));
    UAlsasuaFootstepSystem::MaterialAudioMap.Add("Metal",     LoadSound(TEXT("/Game/Audio/Footsteps/SC_FootstepMetal.SC_FootstepMetal")));
    UAlsasuaFootstepSystem::MaterialAudioMap.Add("Wood",      LoadSound(TEXT("/Game/Audio/Footsteps/SC_FootstepWood.SC_FootstepWood")));
    UAlsasuaFootstepSystem::MaterialAudioMap.Add("Tile",      LoadSound(TEXT("/Game/Audio/Footsteps/SC_FootstepTile.SC_FootstepTile")));
    UAlsasuaFootstepSystem::MaterialAudioMap.Add("Water",     LoadSound(TEXT("/Game/Audio/Footsteps/SC_FootstepWater.SC_FootstepWater")));
    UAlsasuaFootstepSystem::MaterialAudioMap.Add("Gravel",    LoadSound(TEXT("/Game/Audio/Footsteps/SC_FootstepGravel.SC_FootstepGravel")));
}

USoundBase* UAlsasuaFootstepSystem::GetFootstepSoundForMaterial(const FString& MaterialTag)
{
    EnsureFootstepMapPopulated();

    if (MaterialTag.IsEmpty())
    {
        return nullptr;
    }

    if (MaterialAudioMap.Contains(MaterialTag))
    {
        return MaterialAudioMap[MaterialTag].Get();
    }

    return nullptr;
}
