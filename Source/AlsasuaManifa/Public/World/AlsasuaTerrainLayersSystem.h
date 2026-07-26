#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlsasuaTerrainLayersSystem.generated.h"

USTRUCT(BlueprintType)
struct FBarrioTerrainLayer
{
    GENERATED_BODY()
    FString Barrio;
    FString MaterialPath;
    float BlendDistance = 2000.0f;
    float HeightOffset = 0.0f;
};

UCLASS()
class ALSASUAMANIFA_API UAlsasuaTerrainLayersSystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Terrain")
    void AplicarMaterialesPorBarrio();

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Terrain")
    void GenerarSueloCiudad();

private:
    TArray<FBarrioTerrainLayer> Layers;
    void CrearLayers();
};
