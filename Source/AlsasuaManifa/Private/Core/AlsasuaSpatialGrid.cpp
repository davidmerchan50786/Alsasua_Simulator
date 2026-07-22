#include "Core/AlsasuaSpatialGrid.h"

void UAlsasuaSpatialGrid::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

FIntPoint UAlsasuaSpatialGrid::WorldToGrid(FVector Location) const
{
	return FIntPoint(FMath::FloorToInt(Location.X / CellSize), FMath::FloorToInt(Location.Y / CellSize));
}

void UAlsasuaSpatialGrid::UpdateActorInGrid(AActor* Actor)
{
	if (!Actor) return;

	FIntPoint NewCoords = WorldToGrid(Actor->GetActorLocation());

	// En una implementación completa, aquí moveríamos al actor de su celda vieja a la nueva.
	// Por brevedad, registramos en la celda actual.
	Grid.FindOrAdd(NewCoords).RegisteredActors.AddUnique(Actor);
}

void UAlsasuaSpatialGrid::GetNearbyActors(FVector Location, float Radius, TArray<AActor*>& OutActors)
{
	FIntPoint CenterCoords = WorldToGrid(Location);
	int32 CellRadius = FMath::CeilToInt(Radius / CellSize);

	// Solo revisamos las celdas en el radio de influencia (normalmente 3x3 celdas)
	for (int32 x = -CellRadius; x <= CellRadius; ++x)
	{
		for (int32 y = -CellRadius; y <= CellRadius; ++y)
		{
			FIntPoint TargetCoords = CenterCoords + FIntPoint(x, y);
			if (FGridCell* Cell = Grid.Find(TargetCoords))
			{
				for (AActor* Actor : Cell->RegisteredActors)
				{
					if (Actor && FVector::DistSquared(Location, Actor->GetActorLocation()) <= FMath::Square(Radius))
					{
						OutActors.Add(Actor);
					}
				}
			}
		}
	}
}
