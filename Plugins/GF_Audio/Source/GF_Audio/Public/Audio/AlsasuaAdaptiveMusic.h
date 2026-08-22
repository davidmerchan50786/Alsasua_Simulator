#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaAdaptiveMusic.generated.h"

class USoundBase;
class UAudioComponent;

USTRUCT(BlueprintType)
struct FMusicLayer {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) USoundBase* MusicAsset = nullptr;
    UPROPERTY(BlueprintReadOnly) float CurrentVolume = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float TargetVolume = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float FadeSpeed = 2.0f;
    UPROPERTY() UAudioComponent* AudioComp = nullptr;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GF_AUDIO_API UAlsasuaAdaptiveMusic : public UActorComponent
{
    GENERATED_BODY()
public:
    UAlsasuaAdaptiveMusic();

    // Layers configurables desde el editor.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AdaptiveMusic")
    FMusicLayer AmbientLayer;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AdaptiveMusic")
    FMusicLayer TensionLayer;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AdaptiveMusic")
    FMusicLayer CombatLayer;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AdaptiveMusic")
    FMusicLayer SocialLayer;

    // Configuración de transición.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AdaptiveMusic|Config")
    float TensionThreshold = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AdaptiveMusic|Config")
    int32 CombatWantedLevel = 2;

    // Control manual.
    UFUNCTION(BlueprintCallable, Category="AdaptiveMusic")
    void ForceLayer(FName LayerName, float Volume);

    UFUNCTION(BlueprintCallable, Category="AdaptiveMusic")
    void ReleaseAllLayers();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    void StartLayer(FMusicLayer& Layer, float Volume);
    void StopLayer(FMusicLayer& Layer);
    void UpdateLayerVolume(FMusicLayer& Layer, float DeltaTime);
    bool bForced = false;
};
