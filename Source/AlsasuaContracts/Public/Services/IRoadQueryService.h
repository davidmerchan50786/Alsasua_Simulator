#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IRoadQueryService.generated.h"

UINTERFACE(MinimalAPI, BlueprintType, NotBlueprintable)
class URoadQueryService : public UInterface
{
    GENERATED_BODY()
};

class ALSASUACONTRACTS_API IRoadQueryService
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|Roads")
    virtual bool IsRoadAt(const FVector& Location, float Radius = 200.f) const = 0;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|Roads")
    virtual FVector GetNearestRoadPoint(const FVector& Location) const = 0;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|Roads")
    virtual bool GetRoadDirection(const FVector& Location, FVector& OutDirection) const = 0;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|Roads")
    virtual float GetSpeedLimitAt(const FVector& Location) const = 0;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|Roads")
    virtual bool IsOneWayAt(const FVector& Location) const = 0;
};
