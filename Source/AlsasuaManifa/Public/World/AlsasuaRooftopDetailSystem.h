#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlsasuaRooftopDetailSystem.generated.h"

USTRUCT(BlueprintType)
struct FRooftopItem
{
    GENERATED_BODY()
    int32 BuildingId = 0;
    FString Tipo;
    FVector Posicion = FVector::ZeroVector;
    float Rotacion = 0.0f;
    float Escala = 1.0f;
    FString Barrio;
};

UCLASS()
class ALSASUAMANIFA_API UAlsasuaRooftopDetailSystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Rooftop")
    int32 ColocarDetallesCubierta();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Rooftop")
    float ProbAntena = 0.4f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Rooftop")
    float ProbChimenea = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Rooftop")
    float ProbDeposito = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Rooftop")
    float ProbPlacaSolar = 0.1f;

    const TArray<FRooftopItem>& GetItems() const { return Items; }

private:
    TArray<FRooftopItem> Items;
};
