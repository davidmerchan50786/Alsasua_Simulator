#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SpatialGrid.generated.h"

UCLASS()
class ALSASUACORE_API USpatialGrid : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category = "Alsasua|Core")
    TArray<AActor*> GetActorsInRadius(FVector Center, float Radius) const;
};
