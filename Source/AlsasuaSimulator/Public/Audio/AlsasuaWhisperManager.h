#pragma once
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "AlsasuaWhisperManager.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALSASUASIMULATOR_API UAlsasuaWhisperManager : public USceneComponent {
    GENERATED_BODY()
public:
    UAlsasuaWhisperManager();
    UFUNCTION(BlueprintCallable, Category = "Alsasua|Audio")
    void PlaySpatialWhisper(USoundBase* WhisperSound, float Intensity);
};
