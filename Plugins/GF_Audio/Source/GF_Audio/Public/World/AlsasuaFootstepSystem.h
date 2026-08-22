#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AlsasuaFootstepSystem.generated.h"

UCLASS()
class GF_AUDIO_API UAlsasuaFootstepSystem : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Retorna el sonido/efecto adecuado según el material físico detectado
    UFUNCTION(BlueprintCallable, Category = "AAA|Audio")
    static class USoundBase* GetFootstepSoundForMaterial(const FString& MaterialTag);

    // Mapeo dinámico: se puede configurar en el Editor
    static TMap<FString, TWeakObjectPtr<class USoundBase>> MaterialAudioMap;
};
