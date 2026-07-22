// TraficoSubsystem.cpp
#include "TraficoSubsystem.h"
#include "VehiculoAmbiente.h"
#include "CargadorCalles.h"     // ejes viarios (capa World)
#include "ArranqueMundo.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

void UTraficoSubsystem::Tick(float DeltaTime)
{
	if (!ArranqueMundo::BaselineListo) return;
	Acum += DeltaTime;
	if (Acum < PeriodoMantenimiento) return;
	Acum = 0.f;
	Mantener();
}

void UTraficoSubsystem::Mantener()
{
	UWorld* W = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!W) return;
	APawn* Jug = UGameplayStatics::GetPlayerPawn(W, 0);
	if (!Jug) return;
	const FVector2D P(Jug->GetActorLocation());

	// Recicla terminados / lejanos.
	for (int32 i = Vehiculos.Num() - 1; i >= 0; --i)
	{
		AVehiculoAmbiente* V = Vehiculos[i];
		if (!IsValid(V) || V->Terminado() || FVector2D::Distance(P, FVector2D(V->GetActorLocation())) > RadioCull)
		{
			if (IsValid(V)) V->Destroy();
			Vehiculos.RemoveAtSwap(i);
		}
	}

	UCargadorCalles* Calles = W->GetSubsystem<UCargadorCalles>();
	if (!Calles || Calles->EjesViarios.Num() == 0) return;

	const int32 NEjes = Calles->EjesViarios.Num();
	int32 intentos = 0;
	while (Vehiculos.Num() < MaxVehiculos && intentos < 24)
	{
		++intentos;
		const FEjeVial& Eje = Calles->EjesViarios[FMath::RandRange(0, NEjes - 1)];
		if (Eje.Puntos.Num() < 2) continue;

		// busca un punto del eje dentro del anillo del jugador
		int32 idx = INDEX_NONE;
		for (int32 k = 0; k < Eje.Puntos.Num(); ++k)
		{
			const float d = FVector2D::Distance(P, Eje.Puntos[k]);
			if (d >= RadioMin && d <= RadioMax) { idx = k; break; }
		}
		if (idx == INDEX_NONE || idx >= Eje.Puntos.Num() - 1) continue;

		const int32 Sentido = FMath::RandBool() ? 1 : -1;   // carril de ida o de vuelta

		FActorSpawnParameters SP;
		SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AVehiculoAmbiente* V = W->SpawnActor<AVehiculoAmbiente>(
			AVehiculoAmbiente::StaticClass(), FVector(Eje.Puntos[idx].X, Eje.Puntos[idx].Y, 0.f), FRotator::ZeroRotator, SP);
		if (V)
		{
			V->Iniciar(Eje.Puntos, Eje.AnchoCm, idx, Sentido, FMath::FRandRange(VelMin, VelMax));
			Vehiculos.Add(V);
		}
	}
}
