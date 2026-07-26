#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlsasuaTrafficLightSystem.generated.h"

USTRUCT(BlueprintType)
struct FTrafficLight
{
    GENERATED_BODY()
    FVector Posicion = FVector::ZeroVector;
    float Rotacion = 0.0f;
    FString Calle;
    FString Barrio;
    bool bActivo = true;
};

UCLASS()
class ALSASUAMANIFA_API UAlsasuaTrafficLightSystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|TrafficLights")
    int32 ColocarSemaforos();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|TrafficLights")
    int32 MaxSemaforos = 12;

    const TArray<FTrafficLight>& GetSemaforos() const { return Semaforos; }

private:
    TArray<FTrafficLight> Semaforos;
};
