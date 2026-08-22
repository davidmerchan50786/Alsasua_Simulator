#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AlsasuaSpatialGrid.generated.h"

USTRUCT()
struct FGridCell
{
	GENERATED_BODY()

	TArray<AActor*> RegisteredActors;
};

/**
 * Spatial hash grid para queries de proximidad O(1).
 * Cada actor se asocia a una celda; al moverse, se migra automáticamente.
 */
UCLASS()
class ALSASUACORE_API UAlsasuaSpatialGrid : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Tamaño de cada celda (cm). */
	UPROPERTY(EditAnywhere, Category = "Optimization")
	float CellSize = 1000.f;

	/** Registra o actualiza la posición de un actor en la cuadrícula. */
	void UpdateActorInGrid(AActor* Actor);

	/** Elimina un actor de la cuadrícula. */
	void RemoveActorFromGrid(AActor* Actor);

	/** Obtiene todos los actores cercanos a una posición. */
	void GetNearbyActors(FVector Location, float Radius, TArray<AActor*>& OutActors) const;

	/** Limpia toda la cuadrícula. */
	void ClearGrid();

private:
	TMap<FIntPoint, FGridCell> Grid;

	/** Mapa inverso: actor → celda en la que está registrado. */
	TMap<TWeakObjectPtr<AActor>, FIntPoint> ActorToCell;

	FIntPoint WorldToGrid(FVector Location) const;
};
