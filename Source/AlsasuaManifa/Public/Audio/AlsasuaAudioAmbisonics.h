#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaAudioAmbisonics.generated.h"

/** Gestiona el paisaje sonoro envolvente de la manifestación */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALSASUAMANIFA_API UAlsasuaAudioAmbisonics : public UActorComponent
{
    GENERATED_BODY()

public:
    UAlsasuaAudioAmbisonics();

    // Actualiza el volumen y el filtro según la densidad de la multitud y ubicación
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // Dispara un cántico que se propaga por la masa
    UFUNCTION(BlueprintCallable, Category = "AAA|Audio")
    void TriggerDynamicChant(FString ChantID, FVector Origin);

    // Intensidad del murmullo ambiental (0.0 a 1.0)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA|Audio")
    float CrowdHumIntensity = 0.5f;

private:
    void HandleAcousticOcclusion(float DeltaTime);
};
