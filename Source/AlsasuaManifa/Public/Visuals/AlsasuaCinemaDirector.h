#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "AlsasuaCinemaDirector.generated.h"

UCLASS()
class ALSASUAMANIFA_API UAlsasuaCinemaDirector : public UWorldSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual bool IsAllowedToTick() const { return true; }
    virtual TStatId GetStatId() const { return TStatId(); }

    // Registra un evento de interés visual para que la cámara lo "sienta"
    UFUNCTION(BlueprintCallable, Category = "AAA|Camera")
    void RegisterVisualInterest(FVector Location, float Importance, float Duration);

    // Obtiene la intensidad de vibración de cámara actual basada en el caos cercano
    float GetCurrentCameraShakeIntensity() const { return ShakeIntensity; }

private:
    float ShakeIntensity = 0.0f;
    TArray<FVector> ActiveInterests;

    void UpdatePostProcessing(float DeltaTime);
    void CalculateCameraFocus(float DeltaTime);
};
