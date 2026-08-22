#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ITimeOfDayService.generated.h"

UINTERFACE(MinimalAPI, BlueprintType, NotBlueprintable)
class UTimeOfDayService : public UInterface
{
    GENERATED_BODY()
};

class ALSASUACONTRACTS_API ITimeOfDayService
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|Time")
    virtual float GetHour() const = 0;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|Time")
    virtual float GetSunPitch() const = 0;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|Time")
    virtual FRotator GetSunDirection() const = 0;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|Time")
    virtual bool IsNight() const = 0;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|Time")
    virtual FLinearColor GetSunColor() const = 0;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|Time")
    virtual float GetSunIntensity() const = 0;
};
