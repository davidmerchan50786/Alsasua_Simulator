#include "AI/AlsasuaCrowdSentiment.h"
#include "Core/AlsasuaSpatialGrid.h"
#include "Core/AlsasuaProfiling.h"

void UAlsasuaCrowdSentiment::Tick(float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_AlsasuaAI_CrowdSentimentTick);
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
	const int32 CenterX = FMath::FloorToInt(Location.X / 1000.f);
	const int32 CenterY = FMath::FloorToInt(Location.Y / 1000.f);
	const int32 RadiusCells = FMath::Max(1, FMath::CeilToInt(Radius / 1000.f));

	// Inyectar tensión en todas las celdas dentro del radio
	const float Falloff = 1.0f / FMath::Max(1.f, static_cast<float>(RadiusCells));
	for (int32 DX = -RadiusCells; DX <= RadiusCells; ++DX)
	{
		for (int32 DY = -RadiusCells; DY <= RadiusCells; ++DY)
		{
			if (DX * DX + DY * DY > RadiusCells * RadiusCells) continue;

			FIntPoint Cell(CenterX + DX, CenterY + DY);
			const float Dist = FMath::Sqrt(static_cast<float>(DX * DX + DY * DY));
			const float Factor = FMath::Clamp(1.0f - Dist * Falloff, 0.1f, 1.0f);
			TensionMap.FindOrAdd(Cell) += Intensity * Factor;
		}
	}
	UE_LOG(LogTemp, Log, TEXT("Incidente Social en %s (radio %.0fcm, %d celdas). Tensión max: %.2f"),
		*Location.ToString(), Radius, (RadiusCells * 2 + 1) * (RadiusCells * 2 + 1),
		TensionMap.FindOrAdd(FIntPoint(CenterX, CenterY)));
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
