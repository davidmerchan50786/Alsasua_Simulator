#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "VehicleTuningData.generated.h"

UCLASS()
class ALSASUAMANIFA_API UVehicleTuningData : public UPrimaryDataAsset {
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category="Physics") float MaxSpeed = 3000.f;
    UPROPERTY(EditAnywhere, Category="Physics") float Acceleration = 2.5f;
    UPROPERTY(EditAnywhere, Category="Physics") float HandlingScale = 1.0f;
};