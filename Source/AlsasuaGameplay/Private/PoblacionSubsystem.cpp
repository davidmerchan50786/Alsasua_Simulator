// PoblacionSubsystem.cpp
#include "PoblacionSubsystem.h"
#include "PeatonActor.h"
#include "ArranqueMundo.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

void UPoblacionSubsystem::Tick(float DeltaTime)
{
	if (!ArranqueMundo::BaselineListo) return;   // espera al mundo mínimo
	Acum += DeltaTime;
	if (Acum < PeriodoMantenimiento) return;
	Acum = 0.f;
	Mantener();
}

bool UPoblacionSubsystem::PuntoEnAnillo(const FVector& Centro, FVector& Out) const
{
	UWorld* W = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	UNavigationSystemV1* Nav = W ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(W) : nullptr;
	if (!Nav) return false;

	const float Ang = FMath::FRandRange(0.f, 2.f * PI);
	const float Dist = FMath::FRandRange(RadioMin, RadioMax);
	const FVector Cand = Centro + FVector(FMath::Cos(Ang) * Dist, FMath::Sin(Ang) * Dist, 0.f);

	FNavLocation Loc;
	if (Nav->ProjectPointToNavigation(Cand, Loc, FVector(200.f, 200.f, 1000.f)))
	{ Out = Loc.Location; return true; }
	return false;
}

void UPoblacionSubsystem::Mantener()
{
	UWorld* W = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!W) return;
	APawn* Jug = UGameplayStatics::GetPlayerPawn(W, 0);
	if (!Jug) return;
	const FVector P = Jug->GetActorLocation();

	// Limpia nulos/muertos y recicla los lejanos.
	for (int32 i = Peatones.Num() - 1; i >= 0; --i)
	{
		APeatonActor* Pe = Peatones[i];
		if (!IsValid(Pe) || FVector::Dist(P, Pe->GetActorLocation()) > RadioCull)
		{
			if (IsValid(Pe)) Pe->Destroy();
			Peatones.RemoveAtSwap(i);
		}
	}

	// Rellena hasta el máximo, con presupuesto por tick.
	int32 spawns = 0;
	while (Peatones.Num() < MaxPeatones && spawns < SpawnsPorTick)
	{
		FVector Punto;
		if (!PuntoEnAnillo(P, Punto)) break;

		FActorSpawnParameters SP;
		SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		APeatonActor* Pe = W->SpawnActor<APeatonActor>(
			APeatonActor::StaticClass(), Punto + FVector(0, 0, 90.f), FRotator(0, FMath::FRandRange(0.f, 360.f), 0), SP);
		if (Pe) Peatones.Add(Pe);
		++spawns;
	}
}
