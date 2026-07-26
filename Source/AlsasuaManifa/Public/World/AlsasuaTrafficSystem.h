#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlsasuaTrafficSystem.generated.h"

USTRUCT(BlueprintType)
struct FParkedCar
{
    GENERATED_BODY()
    FString Calle;
    float X = 0.0f;
    float Z = 0.0f;
    float Rotacion = 0.0f;
    FString Color;
    FString Tipo;
};

USTRUCT(BlueprintType)
struct FTrafficSign
{
    GENERATED_BODY()
    FString Tipo;
    float X = 0.0f;
    float Z = 0.0f;
    float Rotacion = 0.0f;
    FString Texto;
};

UCLASS()
class ALSASUAMANIFA_API UAlsasuaTrafficSystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Traffic")
    int32 ColocarCocheAparcado();

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Traffic")
    int32 ColocarSenalesTrafico();

private:
    TArray<FParkedCar> Coches;
    TArray<FTrafficSign> SenalesTrafico;

    void GenerarCochesDesdeCalles();
    void GenerarSenalesDesdeCalles();
};
