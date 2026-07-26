#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlsasuaVFXManager.generated.h"

UCLASS()
class ALSASUAMANIFA_API UAlsasuaVFXManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|VFX")
    void SpawnRainParticles(float Intensity);

    UFUNCTION(BlueprintCallable, Category = "Alsasua|VFX")
    void SpawnLeafParticles(float WindSpeed);

    UFUNCTION(BlueprintCallable, Category = "Alsasua|VFX")
    void SpawnDustParticles();

    UFUNCTION(BlueprintCallable, Category = "Alsasua|VFX")
    void StopAllParticles();

private:
    UPROPERTY() class UNiagaraSystem* RainSystem = nullptr;
    UPROPERTY() class UNiagaraSystem* LeafSystem = nullptr;
    UPROPERTY() class UNiagaraSystem* DustSystem = nullptr;
    UPROPERTY() class UNiagaraComponent* ActiveRain = nullptr;
    UPROPERTY() class UNiagaraComponent* ActiveLeaf = nullptr;
};
