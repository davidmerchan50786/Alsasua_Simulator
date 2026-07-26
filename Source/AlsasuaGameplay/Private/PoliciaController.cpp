// PoliciaController.cpp
#include "PoliciaController.h"
#include "AlsasuaTypes.h"          // IDamageable
#include "DiaNocheSubsystem.h"
#include "DisfrazSubsystem.h"
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

	APawn* Jugador = nullptr;
	const bool bVe = VeJugador(Jugador);

	if (bVe && Jugador)
	{
		UltimaPosVista = Jugador->GetActorLocation();   // recuerda dónde te vio
		const float Dist = FVector::Dist(Yo->GetActorLocation(), Jugador->GetActorLocation());
		if (Dist > RadioAtaque)
		{
			Estado = EEstado::Persigue;
			// Flanqueo: cada agente se acerca a un punto distinto alrededor del jugador
			// (ángulo según su id), para rodearlo en vez de ir todos en fila.
			const float Ang = (Yo->GetUniqueID() % 360) * (PI / 180.f);
			const FVector Flanco = Jugador->GetActorLocation() + FVector(FMath::Cos(Ang), FMath::Sin(Ang), 0.f) * (RadioAtaque * 0.85f);
			MoveToLocation(Flanco, 120.f);
		}
		else
		{
			Estado = EEstado::Ataca;
			StopMovement();
			// mirar al jugador
			FVector Dir = Jugador->GetActorLocation() - Yo->GetActorLocation(); Dir.Z = 0.f;
			if (!Dir.IsNearlyZero()) Yo->SetActorRotation(Dir.Rotation());
			TimerAtaque -= DeltaTime;
			if (TimerAtaque <= 0.f) { TimerAtaque = Cadencia; Disparar(Jugador); }
		}
	}
	else if (Estado == EEstado::Persigue || Estado == EEstado::Ataca)
	{
		// Acaba de perderte de vista: va a tu última posición conocida a buscar.
		Estado = EEstado::Busca;
		TimerBusqueda = TiempoBusqueda;
		MoveToLocation(UltimaPosVista, 100.f);
	}
	else if (Estado == EEstado::Busca)
	{
		TimerBusqueda -= DeltaTime;
		if (GetMoveStatus() != EPathFollowingStatus::Moving)   // llegó: husmea alrededor
			if (UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
			{
				FNavLocation L;
				if (Nav->GetRandomReachablePointInRadius(UltimaPosVista, 800.f, L)) MoveToLocation(L.Location, 100.f);
			}
		if (TimerBusqueda <= 0.f) { Estado = EEstado::Patrulla; StopMovement(); }   // se rinde
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
