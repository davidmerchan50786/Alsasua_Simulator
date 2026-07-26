#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
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
    TArray<FVector> Puntos;
    int32 IndiceActual = 0;
    float Velocidad = 500.0f;
    ETipoVehiculo Tipo = ETipoVehiculo::Coche;
    FString Calle;
    FString Direccion;
    bool bEnMarcha = true;
    TWeakObjectPtr<AStaticMeshActor> ActorAsociado;
    FLinearColor ColorCarroceria = FLinearColor::White;
};

UCLASS()
class ALSASUAMANIFA_API UAlsasuaDynamicTrafficSystem : public UGameInstanceSubsystem
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
    TArray<TArray<FVector>> CallesDisponibles;
    float TiempoDesdeUltimoSpawn = 0.0f;
    bool bInicializado = false;

    void CargarCallejero();
    void SpawnVehiculoEnCalle();
    FVector ObtenerPuntoInicio() const;
    FLinearColor ObtenerColorAleatorio() const;
};
