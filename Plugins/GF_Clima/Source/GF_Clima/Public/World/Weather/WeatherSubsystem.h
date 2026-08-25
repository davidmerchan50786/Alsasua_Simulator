#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Services/IWeatherService.h"
#include "WeatherSubsystem.generated.h"

UENUM(BlueprintType)
enum class EWeatherSubsystemState : uint8 {
    Clear,
    Rainy,
    HeavyFog,
    Thunderstorm
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeatherChanged, EWeatherSubsystemState, NewState);

UCLASS()
class GF_CLIMA_API UWeatherSubsystem : public UWorldSubsystem, public IWeatherService
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UPROPERTY(BlueprintReadOnly, Category="AAA|Weather")
    EWeatherSubsystemState CurrentWeather = EWeatherSubsystemState::Clear;

    UPROPERTY(BlueprintReadOnly, Category="AAA|Weather")
    float RainIntensity = 0.f;

    UPROPERTY(BlueprintReadOnly, Category="AAA|Weather")
    float WindSpeed = 0.f;

    UPROPERTY(BlueprintReadOnly, Category="AAA|Weather")
    FVector WindDirection = FVector::ForwardVector;

    UPROPERTY(BlueprintReadOnly, Category="AAA|Weather")
    float Temperature = 15.f;

    UPROPERTY(BlueprintAssignable, Category="AAA|Weather")
    FOnWeatherChanged OnWeatherChanged;

    UFUNCTION(BlueprintCallable, Category="AAA|Weather")
    void SetWeather(EWeatherSubsystemState NewState);

    // IWeatherService
    virtual float GetRainIntensity() const override { return RainIntensity; }
    virtual float GetWindSpeed() const override { return WindSpeed; }
    virtual FVector GetWindDirection() const override { return WindDirection; }
    virtual float GetTemperature() const override { return Temperature; }
    virtual float GetVisibilityMultiplier() const override;
    virtual float GetTireGripMultiplier() const override;
    virtual float GetAIVisibilityMultiplier() const override;
    virtual float GetFootstepNoiseMultiplier() const override;
};
