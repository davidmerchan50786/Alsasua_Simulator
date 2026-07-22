#include "AI/AlsasuaCrowdSentiment.h"
#include "Core/AlsasuaSpatialGrid.h"

void UAlsasuaCrowdSentiment::Tick(float DeltaTime)
{
	DecayTension(DeltaTime);
	PropagateTension();

	// El humor global se normaliza para efectos de post-procesado y audio
	float MaxTension = 0.0f;
	for (auto& Elem : TensionMap)
	{
		MaxTension = FMath::Max(MaxTension, Elem.Value);
	}
	GlobalTension = FMath::Lerp(GlobalTension, MaxTension, 0.05f);
}

void UAlsasuaCrowdSentiment::TriggerSocialEvent(FVector Location, float Intensity, float Radius)
{
	// Convertimos localización a coordenadas de celda (celdas de 10m)
	FIntPoint Cell = FIntPoint(FMath::FloorToInt(Location.X / 1000.f), FMath::FloorToInt(Location.Y / 1000.f));

	TensionMap.FindOrAdd(Cell) += Intensity;
	UE_LOG(LogTemp, Log, TEXT("Incidente Social en %s. Tensión Celda: %f"), *Cell.ToString(), TensionMap[Cell]);
}

void UAlsasuaCrowdSentiment::DecayTension(float DeltaTime)
{
	// La tensión baja con el tiempo si no hay incidentes
	for (auto& Elem : TensionMap)
	{
		Elem.Value = FMath::Max(0.0f, Elem.Value - (0.1f * DeltaTime));
	}
}

void UAlsasuaCrowdSentiment::PropagateTension()
{
	// Simulación de "Rumores": La tensión de una celda se filtra a las vecinas
	TMap<FIntPoint, float> NewTensions = TensionMap;

	for (auto& Elem : TensionMap)
	{
		if (Elem.Value > 0.5f)
		{
			FIntPoint C = Elem.Key;
			// Vecinos 4-way
			FIntPoint Neighbors[] = { C+FIntPoint(1,0), C+FIntPoint(-1,0), C+FIntPoint(0,1), C+FIntPoint(0,-1) };
			for(auto& N : Neighbors)
			{
				NewTensions.FindOrAdd(N) += Elem.Value * 0.01f; // Difusión lenta
			}
		}
	}
	TensionMap = NewTensions;
}

ECrowdMood UAlsasuaCrowdSentiment::GetMoodAtLocation(FVector Location) const
{
	FIntPoint Cell = FIntPoint(FMath::FloorToInt(Location.X / 1000.f), FMath::FloorToInt(Location.Y / 1000.f));
	float Tension = TensionMap.Contains(Cell) ? TensionMap[Cell] : 0.0f;

	if (Tension > 0.8f) return ECrowdMood::Hostile;
	if (Tension > 0.5f) return ECrowdMood::Agitated;
	if (Tension > 0.2f) return ECrowdMood::Calm;
	return ECrowdMood::Calm;
}
