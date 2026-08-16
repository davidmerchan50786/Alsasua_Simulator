// AlsasuaRooftopDetailSystem.h (capa MANIFA)
// Antenas, depósitos, placas solares y parabólicas sobre las cubiertas.
//
// Iba a AStaticMeshActor por pieza —del orden de mil— y con un LoadObject de
// malla y otro de material dentro de la lambda que las creaba, o sea por pieza.
// Ahora una capa instanciada por tipo, con la malla resuelta una vez por
// AlsasuaMallaFab (si hay algo bajado para ese tipo, se prefiere a la primitiva
// del motor que se usaba).
//
// Las chimeneas no salen de aquí: las pone UAlsasuaTejadoModular con la pieza
// del kit apoyada en la cumbrera real.
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
    /** Actor que aloja las capas instanciadas, una por tipo de remate. */
    UPROPERTY() TObjectPtr<AActor> Host = nullptr;

    TArray<FRooftopItem> Items;
};
