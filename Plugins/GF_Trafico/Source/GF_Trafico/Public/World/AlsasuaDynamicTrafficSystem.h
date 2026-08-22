#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "AlsasuaDynamicTrafficSystem.generated.h"

class UAlsasuaStreetGraph;

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
    TArray<FVector> Puntos;
    int32 IndiceActual = 0;
    float Velocidad = 500.0f;
    ETipoVehiculo Tipo = ETipoVehiculo::Coche;
    FString Calle;
    FString Direccion;
    bool bEnMarcha = true;
    TWeakObjectPtr<AActor> ActorAsociado;
    FLinearColor ColorCarroceria = FLinearColor::White;
};

UCLASS()
class GF_TRAFICO_API UAlsasuaDynamicTrafficSystem : public UGameInstanceSubsystem, public FTickableGameObject
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

    /** Grafo de calles para rutas A*; se carga solo al primer uso. */
    UPROPERTY(BlueprintReadWrite, Category = "Alsasua|Traffic")
    TObjectPtr<UAlsasuaStreetGraph> StreetGraph = nullptr;

    /** Aristas máximas por ruta al elegir destino aleatorio. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Traffic")
    int32 MaxRouteDistance = 50;

    const TArray<FVehiclePath>& GetVehiculos() const { return Vehiculos; }

    /** Devuelve StreetGraph, cargándolo desde roads_unity.json si es nulo. */
    UFUNCTION(BlueprintCallable, Category = "Alsasua|Traffic")
    UAlsasuaStreetGraph* ObtenerGrafo();

    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UAlsasuaDynamicTrafficSystem, STATGROUP_Tickables); }
    virtual bool IsTickable() const override { return !IsTemplate() && bInicializado; }

private:
    TArray<FVehiclePath> Vehiculos;
    TArray<TArray<FVector>> CallesDisponibles;
    float TiempoDesdeUltimoSpawn = 0.0f;
    bool bInicializado = false;

    void CargarCallejero();
    void SpawnVehiculoEnCalle();
    FVector ObtenerPuntoInicio() const;
    FLinearColor ObtenerColorAleatorio() const;
};
