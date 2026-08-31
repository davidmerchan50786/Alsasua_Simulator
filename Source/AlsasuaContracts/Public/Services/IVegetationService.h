#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IVegetationService.generated.h"

UINTERFACE(MinimalAPI, BlueprintType, NotBlueprintable)
class UVegetationService : public UInterface
{
    GENERATED_BODY()
};

class ALSASUACONTRACTS_API IVegetationService
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|Vegetation")
    virtual void SpawnAll() = 0;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|Vegetation")
    virtual void ClearAll() = 0;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|Vegetation")
    virtual int32 GetInstanceCount() const = 0;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|Vegetation")
    virtual float GetDensityAt(const FVector& Location) const = 0;
};
