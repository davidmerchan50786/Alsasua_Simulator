// ManifestanteController.cpp
#include "ManifestanteController.h"
#include "ManifestanteActor.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "Engine/World.h"

AManifestanteController::AManifestanteController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AManifestanteController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (GetMoveStatus() == EPathFollowingStatus::Moving) return;
	if (Espera > 0.f) { Espera -= DeltaTime; return; }
	Decidir();
}

void AManifestanteController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);
	Espera = FMath::FRandRange(0.3f, 1.5f);
}

void AManifestanteController::Decidir()
{
	AManifestanteActor* Yo = Cast<AManifestanteActor>(GetPawn());
	UWorld* W = GetWorld();
	if (!Yo || !W) return;
	UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(W);
	if (!Nav) { Espera = 1.f; return; }

	FNavLocation Dest;
	if (Yo->bDispersar)
	{
		// huye en dirección opuesta al punto (carga policial).
		const FVector Dir = (Yo->GetActorLocation() - Yo->PuntoObjetivo).GetSafeNormal2D();
		const FVector Meta = Yo->GetActorLocation() + Dir * 2500.f + FVector(FMath::FRandRange(-400.f, 400.f), FMath::FRandRange(-400.f, 400.f), 0.f);
		if (Nav->ProjectPointToNavigation(Meta, Dest, FVector(300, 300, 1000)))
			MoveToLocation(Dest.Location, 50.f);
		Espera = 0.2f;
		return;
	}

	// milling: punto alcanzable cerca del centro de la protesta.
	if (Nav->GetRandomReachablePointInRadius(Yo->PuntoObjetivo, Yo->RadioMilling, Dest))
		MoveToLocation(Dest.Location, 50.f);
	else
		Espera = 1.f;
}
