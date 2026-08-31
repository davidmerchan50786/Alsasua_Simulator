#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IBuildingQueryService.generated.h"

UINTERFACE(MinimalAPI, BlueprintType, NotBlueprintable)
class UBuildingQueryService : public UInterface
{
    GENERATED_BODY()
};

class ALSASUACONTRACTS_API IBuildingQueryService
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|Buildings")
    virtual int32 GetBuildingCount() const = 0;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|Buildings")
    virtual FName GetBarrioAt(const FVector& Location) const = 0;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|Buildings")
    virtual bool GetBuildingAt(const FVector& Location, float Radius, FVector& OutCenter, float& OutHeight) const = 0;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|Buildings")
    virtual bool IsInteriorAt(const FVector& Location) const = 0;
};
