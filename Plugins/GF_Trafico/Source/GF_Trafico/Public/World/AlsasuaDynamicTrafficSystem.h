#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Contratos/AlsasuaContratosUI.h"
#include "AlsasuaDynamicTrafficSystem.generated.h"

UENUM(BlueprintType)
enum class ETipoVehiculo : uint8
{
    Coche,
    Furgoneta,
    Camion,
    Autobus,
    Moto
};

USTRUCT(BlueprintType)
struct FVehiclePath
{
    GENERATED_BODY()
    /** Tramo del grafo por el que va, y cuánto lleva recorrido de él (cm).
     *  Antes eran ocho puntos copiados de UNA calle: al acabarlos el coche
     *  volvía de un salto al primero, porque no había forma de saber qué calle
     *  seguía. Con el grafo hay cruces y se puede girar. */
    int32 TramoActual = -1;
    float Avance = 0.f;
    /** Desplazamiento al carril, perpendicular a la marcha. */
    float CarrilCm = 0.f;
    /** Avanza en cada cruce, para que la elección de giro no sea la misma
     *  siempre y aun así sea reproducible entre arranques. */
    int32 Semilla = 0;

    float Velocidad = 500.0f;
    ETipoVehiculo Tipo = ETipoVehiculo::Coche;
    FString Calle;
    FString Direccion;
    bool bEnMarcha = true;
    TWeakObjectPtr<AActor> ActorAsociado;
    FLinearColor ColorCarroceria = FLinearColor::White;
};

UCLASS()
class GF_TRAFICO_API UAlsasuaDynamicTrafficSystem : public UGameInstanceSubsystem, public IAlsasuaPilarArranque, public IAlsasuaPilarTiquear
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Traffic")
    void IniciarTrafico();

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Traffic")
    void ActualizarTrafico(float DeltaTime);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Traffic")
    int32 MaxVehiculos = 25;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Traffic")
    float FrecuenciaSpawn = 10.0f;

    const TArray<FVehiclePath>& GetVehiculos() const { return Vehiculos; }

private:
    TArray<FVehiclePath> Vehiculos;
    /** El grafo. Lo construye UAlsasuaRedViaria y lo comparte con quien lo pida:
     *  este sistema y el de peatones leían roads_unity.json cada uno por su
     *  cuenta y se quedaban polilíneas sueltas. */
    UPROPERTY() TObjectPtr<class UAlsasuaRedViaria> Red = nullptr;
    float TiempoDesdeUltimoSpawn = 0.0f;
    bool bInicializado = false;

    void CargarCallejero();
    void SpawnVehiculoEnCalle();
    FVector ObtenerPuntoInicio() const;
    FLinearColor ObtenerColorAleatorio() const;
};
