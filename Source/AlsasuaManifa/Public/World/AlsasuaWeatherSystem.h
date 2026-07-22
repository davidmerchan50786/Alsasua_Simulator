#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AlsasuaCore.h"
#include "AlsasuaWeatherSystem.generated.h"

UCLASS()
class ALSASUAMANIFA_API UAlsasuaWeatherSystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime);

    // Valor maestro que puede ser leído por materiales en todo el mundo
    UPROPERTY(BlueprintReadOnly, Category = "AAA|Weather")
    float GlobalWetness = 0.0f;

private:
    void UpdateDynamicParameters(float Intensity);
};
