#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlsasuaDoorEntranceSystem.generated.h"

USTRUCT(BlueprintType)
struct FDoorEntry
{
    GENERATED_BODY()
    int32 BuildingId = 0;
    FVector Posicion = FVector::ZeroVector;
    float Rotacion = 0.0f;
    FString Tipo;
    FString Color;
    FString Barrio;
    /** Dirección de OSM (Datos/direcciones_osm.json); vacías si no tiene. */
    FString Calle;
    FString Portal;
};

UCLASS()
class ALSASUAMANIFA_API UAlsasuaDoorEntranceSystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Doors")
    int32 ColocarPuertas();

    const TArray<FDoorEntry>& GetPuertas() const { return Puertas; }

private:
    TArray<FDoorEntry> Puertas;
};
