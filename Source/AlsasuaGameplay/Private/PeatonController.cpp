// PeatonController.cpp
#include "PeatonController.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "AlsasuaNPC.h"
#include "Components/AlsasuaParanoiaComponent.h"
#include "WantedSubsystem.h"
#include "PoblacionSubsystem.h"
#include "Engine/GameInstance.h"

AAlsasuaPeatonController::AAlsasuaPeatonController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AAlsasuaPeatonController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (GetMoveStatus() == EPathFollowingStatus::Moving) return;

	if (Espera > 0.f) { Espera -= DeltaTime; return; }

	// Paranoid civilians investigate and patrol like guards.
	TickParanoidBehavior(DeltaTime);

	NuevoDestino();
}

void AAlsasuaPeatonController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);
	Espera = FMath::FRandRange(EsperaMin, EsperaMax);
}

void AAlsasuaPeatonController::NuevoDestino()
{
	const APawn* Yo = GetPawn();
	UWorld* W = GetWorld();
	if (!Yo || !W) return;

	UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(W);
	if (!Nav) return;

	// Paranoid civilians patrol wider radius.
	const float Radius = (GetParanoiaLevel() >= GuardBehaviorThreshold) ? ParanoidPatrolRadius : RadioPaseo;

	FNavLocation Dest;
	if (Nav->GetRandomReachablePointInRadius(Yo->GetActorLocation(), Radius, Dest))
		MoveToLocation(Dest.Location, 60.f);
	else
		Espera = 1.f;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Paranoid behavior — civilian acts like guard
// ─────────────────────────────────────────────────────────────────────────────
float AAlsasuaPeatonController::GetParanoiaLevel() const
{
	if (const AAlsasuaNPC* NPC = Cast<AAlsasuaNPC>(GetPawn()))
		if (NPC->ParanoiaComp)
			return NPC->ParanoiaComp->GetParanoiaLevel();
	return 0.f;
}

void AAlsasuaPeatonController::TickParanoidBehavior(float DeltaTime)
{
	const float Paranoia = GetParanoiaLevel();
	if (Paranoia < GuardBehaviorThreshold) return;

	// Paranoid civilians report suspicious activity (loud noises, weapons, crimes).
	// They call cops more aggressively than normal civilians.
	if (FMath::FRand() < CrimeReportChance * DeltaTime)
	{
		UWorld* W = GetWorld();
		if (!W) return;

		APawn* Player = W->GetFirstPlayerController() ? W->GetFirstPlayerController()->GetPawn() : nullptr;
		if (!Player) return;

		const float DistToPlayer = FVector::Dist(GetPawn()->GetActorLocation(), Player->GetActorLocation());

		// Only report if player is within 2000cm and has wanted level.
		if (DistToPlayer < 2000.f)
		{
			if (UGameInstance* GI = W->GetGameInstance())
			{
				if (UWantedSubsystem* Wanted = GI->GetSubsystem<UWantedSubsystem>())
				{
					if (Wanted->NivelBusqueda > 0)
					{
						// Paranoid civilian reports crime with higher urgency.
						Wanted->AumentarBusqueda(1);
					}
				}
			}
		}
	}
}
