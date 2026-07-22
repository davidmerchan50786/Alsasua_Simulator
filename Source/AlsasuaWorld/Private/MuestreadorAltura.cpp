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
	if (!Terreno) return 0.f;
	return Terreno->AlturaEnMundo(Pos.X, Pos.Y);
}

FVector UMuestreadorAltura::NormalMundo(const FVector& Pos) const
{
	if (!Terreno) return FVector::UpVector;
	return Terreno->NormalEnMundo(Pos.X, Pos.Y);
}
