#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Tickable.h"
#include "AlsasuaCinemaDirector.generated.h"

UCLASS()
class ALSASUAMANIFA_API UAlsasuaCinemaDirector : public UWorldSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Tick(float DeltaTime) override;
    virtual bool IsAllowedToTick() const override { return true; }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(AlsasuaCinemaDirector, STATGROUP_Game); }
    virtual bool IsTickable() const override { return !IsTemplate(); }   // no ticar el CDO (CLAUDE.md §9)

    // Registra un evento de interés visual para que la cámara lo "sienta"
    UFUNCTION(BlueprintCallable, Category = "AAA|Camera")
    void RegisterVisualInterest(FVector Location, float Importance, float Duration);

    // Obtiene la intensidad de vibración de cámara actual basada en el caos cercano
    float GetCurrentCameraShakeIntensity() const { return ShakeIntensity; }

private:
    float ShakeIntensity = 0.0f;
    float ChromaticAberration = 0.0f;
    float FilmGrain = 0.0f;
    TArray<FVector> ActiveInterests;

    UPROPERTY()
    TObjectPtr<UMaterialParameterCollection> CachedMPC = nullptr;

    void UpdatePostProcessing(float DeltaTime);
    void CalculateCameraFocus(float DeltaTime);
};
