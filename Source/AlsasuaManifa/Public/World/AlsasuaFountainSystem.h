#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlsasuaFountainSystem.generated.h"

USTRUCT(BlueprintType)
struct FRealFountain
{
    GENERATED_BODY()
    FString Nombre;
    FVector Posicion = FVector::ZeroVector;
    FString Barrio;
    FString Calle;
    float Radio = 150.0f;
    float AlturaCazoleta = 80.0f;
    bool bFuncional = true;
};

UCLASS()
class ALSASUAMANIFA_API UAlsasuaFountainSystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Fountains")
    int32 ColocarFuentes();

    const TArray<FRealFountain>& GetFuentes() const { return Fuentes; }

private:
    TArray<FRealFountain> Fuentes;
};
