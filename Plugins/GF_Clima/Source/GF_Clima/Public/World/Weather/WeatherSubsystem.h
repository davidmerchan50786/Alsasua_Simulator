#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ContratosClima.h"
#include "WeatherSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeatherChanged, EAlsasuaWeatherState, NewState);

UCLASS()
class GF_CLIMA_API UWeatherSubsystem : public UWorldSubsystem, public IWeatherService
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** El enum vive en AlsasuaContracts: es parte del contrato, no del plugin. */
    UPROPERTY(BlueprintReadOnly, Category="AAA|Weather")
    EAlsasuaWeatherState CurrentWeather = EAlsasuaWeatherState::Clear;

    UPROPERTY(BlueprintAssignable, Category="AAA|Weather")
    FOnWeatherChanged OnWeatherChanged;

    UFUNCTION(BlueprintCallable, Category="AAA|Weather")
    void SetWeather(EAlsasuaWeatherState NewState);

    // Impacto en la jugabilidad
    UFUNCTION(BlueprintPure, Category="AAA|Weather")
    float GetTireGripMultiplier() const override;

    UFUNCTION(BlueprintPure, Category="AAA|Weather")
    float GetAIVisibilityMultiplier() const override;

    UFUNCTION(BlueprintPure, Category="AAA|Weather")
    float GetFootstepNoiseMultiplier() const override;

    // ── Contrato IWeatherService (publicado como "Clima.Meteorologia") ──
    virtual EAlsasuaWeatherState GetWeatherState() const override { return CurrentWeather; }
    virtual float GetRainIntensity() const override;
    virtual float GetWindSpeedKmh() const override;
    virtual float GetTemperatureCelsius() const override;
};
