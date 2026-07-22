#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AlsasuaSpatialGrid.generated.h"

USTRUCT()
struct FGridCell
{
	GENERATED_BODY()

	// Lista de IDs de actores o proxies en esta celda específica
	TArray<AActor*> RegisteredActors;
};

UCLASS()
class ALSASUAMANIFA_API UAlsasuaSpatialGrid : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// Tamaño de cada celda (ej: 10 metros = 1000 unidades Unreal)
	UPROPERTY(EditAnywhere, Category = "Optimization")
	float CellSize = 1000.f;

	// Registra o actualiza la posición de un actor en la cuadrícula
	void UpdateActorInGrid(AActor* Actor);

	// Obtiene todos los actores cercanos a una posición consultando solo celdas adyacentes
	void GetNearbyActors(FVector Location, float Radius, TArray<AActor*>& OutActors);

private:
	// Mapa de coordenadas de celda (X,Y) a contenido de la misma
	TMap<FIntPoint, FGridCell> Grid;

	FIntPoint WorldToGrid(FVector Location) const;
};
