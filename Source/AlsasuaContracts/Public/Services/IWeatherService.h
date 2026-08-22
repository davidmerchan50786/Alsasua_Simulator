#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IWeatherService.generated.h"

UINTERFACE(MinimalAPI, BlueprintType, NotBlueprintable)
class UWeatherService : public UInterface
{
    GENERATED_BODY()
};

class ALSASUACONTRACTS_API IWeatherService
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|Weather")
    virtual float GetRainIntensity() const = 0;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|Weather")
    virtual float GetWindSpeed() const = 0;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|Weather")
    virtual FVector GetWindDirection() const = 0;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|Weather")
    virtual float GetTemperature() const = 0;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|Weather")
    virtual float GetVisibilityMultiplier() const = 0;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|Weather")
    virtual float GetTireGripMultiplier() const = 0;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|Weather")
    virtual float GetAIVisibilityMultiplier() const = 0;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|Weather")
    virtual float GetFootstepNoiseMultiplier() const = 0;
};
