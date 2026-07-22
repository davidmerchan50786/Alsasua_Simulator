// PeatonController.cpp
#include "PeatonController.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

AAlsasuaPeatonController::AAlsasuaPeatonController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AAlsasuaPeatonController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (GetMoveStatus() == EPathFollowingStatus::Moving) return;

	if (Espera > 0.f) { Espera -= DeltaTime; return; }
	NuevoDestino();
}

void AAlsasuaPeatonController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);
	Espera = FMath::FRandRange(EsperaMin, EsperaMax);   // descansa antes del siguiente tramo
}

void AAlsasuaPeatonController::NuevoDestino()
{
	const APawn* Yo = GetPawn();
	UWorld* W = GetWorld();
	if (!Yo || !W) return;

	UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(W);
	if (!Nav) return;

	FNavLocation Dest;
	if (Nav->GetRandomReachablePointInRadius(Yo->GetActorLocation(), RadioPaseo, Dest))
		MoveToLocation(Dest.Location, 60.f);
	else
		Espera = 1.f;   // sin navmesh aún; reintenta pronto
}
