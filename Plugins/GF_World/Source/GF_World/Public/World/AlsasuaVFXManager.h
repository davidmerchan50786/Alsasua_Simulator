#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlsasuaVFXManager.generated.h"

UCLASS()
class GF_WORLD_API UAlsasuaVFXManager : public UGameInstanceSubsystem
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
    /** Carga perezosa y una sola vez: si la ruta no está, se dice y no se
     *  reintenta en cada llamada. */
    class UNiagaraSystem* Resolver(TObjectPtr<class UNiagaraSystem>& Cache,
                                   const TCHAR* Ruta, bool& bIntentado);

    UPROPERTY() TObjectPtr<class UNiagaraSystem> RainSystem = nullptr;
    UPROPERTY() TObjectPtr<class UNiagaraSystem> LeafSystem = nullptr;
    UPROPERTY() TObjectPtr<class UNiagaraSystem> DustSystem = nullptr;
    UPROPERTY() class UNiagaraComponent* ActiveRain = nullptr;
    UPROPERTY() class UNiagaraComponent* ActiveLeaf = nullptr;

    bool bRainIntentado = false;
    bool bLeafIntentado = false;
    bool bDustIntentado = false;
};
