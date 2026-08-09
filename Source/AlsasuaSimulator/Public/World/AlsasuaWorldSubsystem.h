#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AlsasuaWorldSubsystem.generated.h"

UCLASS()
class ALSASUASIMULATOR_API UAlsasuaWorldSubsystem : public UWorldSubsystem {
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    // Controla la humedad del asfalto (Lluvia/Nieve) para el material AAA
    UFUNCTION(BlueprintCallable, Category = "Alsasua|Environment")
    void SetGlobalWetness(float Wetness);
};
