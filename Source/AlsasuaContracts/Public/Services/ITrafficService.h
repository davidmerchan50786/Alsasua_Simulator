#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ITrafficService.generated.h"

UINTERFACE(MinimalAPI, BlueprintType, NotBlueprintable)
class UTrafficService : public UInterface
{
    GENERATED_BODY()
};

class ALSASUACONTRACTS_API ITrafficService
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|Traffic")
    virtual float GetDensity() const = 0;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|Traffic")
    virtual void SetDensity(float NewDensity) = 0;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|Traffic")
    virtual int32 GetVehicleCount() const = 0;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|Traffic")
    virtual bool IsGreenLight(const FVector& Location) const = 0;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|Traffic")
    virtual float GetTrafficLightTimeRemaining(const FVector& Location) const = 0;
};
