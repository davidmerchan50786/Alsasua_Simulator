#include "Core/AlsasuaSpatialGrid.h"

void UAlsasuaSpatialGrid::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

FIntPoint UAlsasuaSpatialGrid::WorldToGrid(FVector Location) const
{
	return FIntPoint(
		FMath::FloorToInt(Location.X / CellSize),
		FMath::FloorToInt(Location.Y / CellSize)
	);
}

void UAlsasuaSpatialGrid::UpdateActorInGrid(AActor* Actor)
{
	if (!Actor) return;

	const FIntPoint NewCoords = WorldToGrid(Actor->GetActorLocation());

	// Si el actor ya está registrado, verificar si cambió de celda.
	if (FIntPoint* OldCoords = ActorToCell.Find(Actor))
	{
		if (*OldCoords == NewCoords)
		{
			return; // Misma celda, nada que hacer.
		}

		// Migrar: eliminar de celda vieja.
		if (FGridCell* OldCell = Grid.Find(*OldCoords))
		{
			OldCell->RegisteredActors.Remove(Actor);
			if (OldCell->RegisteredActors.Num() == 0)
			{
				Grid.Remove(*OldCoords);
			}
		}
	}

	// Insertar en celda nueva.
	Grid.FindOrAdd(NewCoords).RegisteredActors.AddUnique(Actor);
	ActorToCell.Add(Actor) = NewCoords;
}

void UAlsasuaSpatialGrid::RemoveActorFromGrid(AActor* Actor)
{
	if (!Actor) return;

	if (FIntPoint* CellCoords = ActorToCell.Find(Actor))
	{
		if (FGridCell* Cell = Grid.Find(*CellCoords))
		{
			Cell->RegisteredActors.Remove(Actor);
			if (Cell->RegisteredActors.Num() == 0)
			{
				Grid.Remove(*CellCoords);
			}
		}
		ActorToCell.Remove(Actor);
	}
}

void UAlsasuaSpatialGrid::GetNearbyActors(FVector Location, float Radius, TArray<AActor*>& OutActors) const
{
	const FIntPoint CenterCoords = WorldToGrid(Location);
	const int32 CellRadius = FMath::CeilToInt(Radius / CellSize);
	const float RadiusSq = Radius * Radius;

	for (int32 x = -CellRadius; x <= CellRadius; ++x)
	{
		for (int32 y = -CellRadius; y <= CellRadius; ++y)
		{
			const FIntPoint TargetCoords = CenterCoords + FIntPoint(x, y);
			if (const FGridCell* Cell = Grid.Find(TargetCoords))
			{
				for (AActor* Actor : Cell->RegisteredActors)
				{
					if (IsValid(Actor) && FVector::DistSquared(Location, Actor->GetActorLocation()) <= RadiusSq)
					{
						OutActors.Add(Actor);
					}
				}
			}
		}
	}
}

void UAlsasuaSpatialGrid::ClearGrid()
{
	Grid.Empty();
	ActorToCell.Empty();
}
