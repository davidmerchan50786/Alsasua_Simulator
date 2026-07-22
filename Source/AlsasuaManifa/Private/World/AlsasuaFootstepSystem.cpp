#include "World/AlsasuaFootstepSystem.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Sound/SoundBase.h"

// Inicialización estática
TMap<FString, TWeakObjectPtr<USoundBase>> UAlsasuaFootstepSystem::MaterialAudioMap;

USoundBase* UAlsasuaFootstepSystem::GetFootstepSoundForMaterial(const FString& MaterialTag)
{
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
