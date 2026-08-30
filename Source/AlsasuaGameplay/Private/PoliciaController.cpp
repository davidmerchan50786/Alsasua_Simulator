// PoliciaController.cpp
#include "PoliciaController.h"
#include "AlsasuaTypes.h"
#include "PoliciaActor.h"
#include "AlsasuaNPC.h"
#include "WantedSubsystem.h"
#include "DiaNocheSubsystem.h"
#include "DisfrazSubsystem.h"
#include "ManifestacionSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"

AAlsasuaPoliciaController::AAlsasuaPoliciaController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AAlsasuaPoliciaController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	APawn* Yo = GetPawn();
	if (!Yo) return;

	// Prioridad: dispersar manifestación si hay 3+ policías cerca.
	if (Estado != EEstado::Dispersion)
	{
		FVector CentroManifestacion;
		if (DetectarManifestacion(CentroManifestacion))
		{
			Estado = EEstado::Dispersion;
			TimerDispersion = 0.f;
			MoveToLocation(CentroManifestacion, 200.f);
			return;
		}
	}

	if (Estado == EEstado::Dispersion)
	{
		TimerDispersion += DeltaTime;
		if (GetMoveStatus() != EPathFollowingStatus::Moving)
		{
			if (UWorld* W = GetWorld())
				if (UGameInstance* GI = W->GetGameInstance())
					if (UManifestacionSubsystem* Mf = GI->GetSubsystem<UManifestacionSubsystem>())
						if (Mf->Activa())
						{
							Mf->Disolver(true);
							UE_LOG(LogTemp, Log, TEXT("[Policía] disolvió manifestación"));
						}
			Estado = EEstado::Patrulla;
			StopMovement();
		}
		else if (TimerDispersion > 15.f)
		{
			Estado = EEstado::Patrulla;
			StopMovement();
		}
		return;
	}

	APawn* Jugador = nullptr;
	const bool bVe = VeJugador(Jugador);

	if (bVe && Jugador)
	{
		// Report sighting to wanted system (updates search zone to current pos).
		if (UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
			if (UWantedSubsystem* Wn = GI->GetSubsystem<UWantedSubsystem>())
				Wn->ReportSighting(Jugador->GetActorLocation());

		UltimaPosVista = Jugador->GetActorLocation();
		const float Dist = FVector::Dist(Yo->GetActorLocation(), Jugador->GetActorLocation());
		if (Dist > RadioAtaque)
		{
			Estado = EEstado::Persigue;
			const float Ang = (Yo->GetUniqueID() % 360) * (PI / 180.f);
			const FVector Flanco = Jugador->GetActorLocation() + FVector(FMath::Cos(Ang), FMath::Sin(Ang), 0.f) * (RadioAtaque * 0.85f);
			MoveToLocation(Flanco, 120.f);
		}
		else
		{
			Estado = EEstado::Ataca;
			StopMovement();
			FVector Dir = Jugador->GetActorLocation() - Yo->GetActorLocation(); Dir.Z = 0.f;
			if (!Dir.IsNearlyZero()) Yo->SetActorRotation(Dir.Rotation());
			TimerAtaque -= DeltaTime;
			if (TimerAtaque <= 0.f) { TimerAtaque = Cadencia; Disparar(Jugador); }
			// GTA-style: wounded officers strafe to a new firing position.
			MaybeTakeCover(Jugador, DeltaTime);
		}
	}
	else if (Estado == EEstado::Persigue || Estado == EEstado::Ataca)
	{
		Estado = EEstado::Busca;
		TimerBusqueda = TiempoBusqueda;
		MoveToLocation(UltimaPosVista, 100.f);
	}
	else if (Estado == EEstado::Busca)
	{
		TimerBusqueda -= DeltaTime;
		if (GetMoveStatus() != EPathFollowingStatus::Moving)
			if (UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
			{
				FNavLocation L;
				if (Nav->GetRandomReachablePointInRadius(UltimaPosVista, 800.f, L)) MoveToLocation(L.Location, 100.f);
			}
		if (TimerBusqueda <= 0.f) { Estado = EEstado::Patrulla; StopMovement(); }
	}
}

bool AAlsasuaPoliciaController::VeJugador(APawn*& OutJugador) const
{
	OutJugador = UGameplayStatics::GetPlayerPawn(this, 0);
	const APawn* Yo = GetPawn();
	if (!OutJugador || !Yo) return false;

	// factores de sigilo (día-noche × disfraz)
	float Factor = 1.f;
	if (const UWorld* W = GetWorld())
		if (const UGameInstance* GI = W->GetGameInstance())
		{
			if (const UDiaNocheSubsystem* Dn = GI->GetSubsystem<UDiaNocheSubsystem>()) Factor *= Dn->DeteccionSigilo();
			if (const UDisfrazSubsystem* Dis = GI->GetSubsystem<UDisfrazSubsystem>()) Factor *= Dis->FactorReconocimiento();
		}
	const float Radio = RadioVision * Factor;

	const FVector Ojo = Yo->GetActorLocation() + FVector(0, 0, 80);
	const FVector AJug = OutJugador->GetActorLocation() - Ojo;
	if (AJug.Size() > Radio) return false;
	if (FVector::DotProduct(Yo->GetActorForwardVector(), AJug.GetSafeNormal()) < FMath::Cos(FMath::DegreesToRadians(AnguloVision * 0.5f))) return false;

	// línea de visión
	FHitResult Hit;
	FCollisionQueryParams Q; Q.AddIgnoredActor(Yo); Q.AddIgnoredActor(OutJugador);
	if (UWorld* W = GetWorld())
	{
		if (W->LineTraceSingleByChannel(Hit, Ojo, OutJugador->GetActorLocation() + FVector(0,0,80), ECC_Visibility, Q))
			return false;   // algo bloquea
	}
	return true;
}

void AAlsasuaPoliciaController::Disparar(APawn* Jugador)
{
	if (IDamageable* D = Cast<IDamageable>(Jugador))
		if (!D->EstaMuerto())
			D->RecibirDano(Dano, GetPawn() ? GetPawn()->GetActorLocation() : FVector::ZeroVector, ETipoDano::Bala);
}

bool AAlsasuaPoliciaController::DetectarManifestacion(FVector& OutCentro) const
{
	const UWorld* W = GetWorld();
	if (!W) return false;
	const UGameInstance* GI = W->GetGameInstance();
	if (!GI) return false;
	const UManifestacionSubsystem* Mf = GI->GetSubsystem<UManifestacionSubsystem>();
	if (!Mf || !Mf->Activa()) return false;

	const float Dist = FVector::Dist(GetPawn() ? GetPawn()->GetActorLocation() : FVector::ZeroVector, Mf->GetCentroActual());
	if (Dist > 5000.f) return false;

	OutCentro = Mf->GetCentroActual();
	return true;
}

void AAlsasuaPoliciaController::MaybeTakeCover(APawn* Jugador, float DeltaTime)
{
	APawn* Yo = GetPawn();
	if (!Yo || !Jugador) return;

	// Officers take cover (strafe) once wounded, moving on a timer so they
	// don't stand still in the open and eat fire.
	const AAlsasuaNPC* Npc = Cast<AAlsasuaNPC>(Yo);
	if (!Npc || Npc->EstaMuerto()) { LastHealth = -1; return; }

	if (Npc->GetVida() != LastHealth || LastHealth < 0)
	{
		LastHealth = Npc->GetVida();

		const float Pct = (float)Npc->GetVida() / (float)FMath::Max(1, Npc->GetVidaMax());
		// Reset the timer whenever wounded so it repositions promptly.
		if (Pct < CoverHealthPct) TimerStrafe = FMath::Max(TimerStrafe, StrafeInterval * 0.4f);
	}

	if (LastHealth < 0) return;
	const float Pct = (float)LastHealth / (float)FMath::Max(1, Npc->GetVidaMax());
	if (Pct >= CoverHealthPct) return;

	TimerStrafe -= DeltaTime;
	if (TimerStrafe > 0.f) return;
	TimerStrafe = StrafeInterval;

	// Sidestep to a flank angle around the player (fresh firing position).
	const float Ang = FMath::FRandRange(0.f, 2.f * PI);
	const FVector Off(FMath::Cos(Ang), FMath::Sin(Ang), 0.f);
	const FVector Rel = Jugador->GetActorLocation() + Off * (RadioAtaque * 0.6f);
	MoveToLocation(Rel, 80.f);
}
