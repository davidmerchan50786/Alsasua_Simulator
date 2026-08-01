#include "MuestreadorAltura.h"
#include "TerrenoGenerado.h"
#include "Engine/World.h"
#include "EngineUtils.h"

void UMuestreadorAltura::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	BuscarTerreno();
}

void UMuestreadorAltura::Deinitialize()
{
	Terreno = nullptr;
	bHayAgua = false;
	Super::Deinitialize();
}

void UMuestreadorAltura::BuscarTerreno()
{
	if (UWorld* W = GetWorld())
	{
		for (TActorIterator<ATerrenoGenerado> It(W); It; ++It)
		{
			Terreno = *It;
			return;
		}
	}
}

float UMuestreadorAltura::AlturaMundo(const FVector& Pos) const
{
	if (!Terreno)
		const_cast<UMuestreadorAltura*>(this)->BuscarTerreno();
	if (!Terreno) return 0.f;
	return Terreno->AlturaEnMundo(Pos.X, Pos.Y);
}

FVector UMuestreadorAltura::NormalMundo(const FVector& Pos) const
{
	if (!Terreno) return FVector::UpVector;
	return Terreno->NormalEnMundo(Pos.X, Pos.Y);
}

FLinearColor UMuestreadorAltura::DatosSuelo(const FVector& Pos) const
{
	if (!Terreno) return FLinearColor::Black;

	const float H = Terreno->AlturaEnMundo(Pos.X, Pos.Y);
	const float HMin = (float)Terreno->HeightMinCm;
	const float HMax = (float)Terreno->HeightMaxCm;
	const float HNorm = FMath::Clamp((H - HMin) / FMath::Max(HMax - HMin, 1.f), 0.f, 1.f);

	const FVector N = Terreno->NormalEnMundo(Pos.X, Pos.Y);
	const float Slope = FMath::Clamp(1.f - N.Z, 0.f, 1.f);

	const float WaterDist = H - AlturaNivelAgua;
	const float WaterBlend = FMath::Clamp(FMath::Clamp(WaterDist / 500.f, -1.f, 0.f) + 1.f, 0.f, 1.f);

	return FLinearColor(HNorm, Slope, WaterBlend, 1.f);
}
