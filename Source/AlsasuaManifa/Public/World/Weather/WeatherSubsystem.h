#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WeatherSubsystem.generated.h"

UENUM(BlueprintType)
enum class EWeatherState : uint8 {
    Clear,
    Rainy,
    HeavyFog,
    Thunderstorm
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeatherChanged, EWeatherState, NewState);

UCLASS()
class ALSASUAMANIFA_API UWeatherSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category="AAA|Weather")
    EWeatherState CurrentWeather = EWeatherState::Clear;

    UPROPERTY(BlueprintAssignable, Category="AAA|Weather")
    FOnWeatherChanged OnWeatherChanged;

    UFUNCTION(BlueprintCallable, Category="AAA|Weather")
    void SetWeather(EWeatherState NewState);

    // Impacto en la jugabilidad
    UFUNCTION(BlueprintPure, Category="AAA|Weather")
    float GetTireGripMultiplier() const;

    UFUNCTION(BlueprintPure, Category="AAA|Weather")
    float GetAIVisibilityMultiplier() const;

    UFUNCTION(BlueprintPure, Category="AAA|Weather")
    float GetFootstepNoiseMultiplier() const;
};
