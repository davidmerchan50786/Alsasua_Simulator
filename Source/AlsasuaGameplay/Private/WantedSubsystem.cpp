// WantedSubsystem.cpp
#include "WantedSubsystem.h"
#include "DiaNocheSubsystem.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

void UWantedSubsystem::AumentarBusqueda(int32 Cantidad)
{
	const int32 Antes = NivelBusqueda;
	NivelBusqueda = FMath::Clamp(NivelBusqueda + Cantidad, 0, 5);
	if (Cantidad > 0) TimerBajar = TiempoBajarNivel;

	// A new hit resets the hidden-escape timer.
	if (Cantidad > 0) HiddenTimer = 0.f;

	// Raising wanted expands the active search zone.
	if (Cantidad > 0 && NivelBusqueda >= 1)
	{
		bSearchActive = true;
		if (APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
		{
			SearchCenter = Player->GetActorLocation();
		}
		CurrentSearchRadius = SearchRadiusBase + SearchRadiusPerStar * NivelBusqueda;
		OnSearchStarted.Broadcast(SearchCenter, CurrentSearchRadius);
	}

	if (NivelBusqueda != Antes) OnEstrellasCambia.Broadcast(NivelBusqueda);
}

void UWantedSubsystem::ReportSighting(FVector Location)
{
	if (NivelBusqueda <= 0) return;

	// Police re-acquire player — reset hidden timer, move search zone.
	HiddenTimer = 0.f;
	bSearchActive = true;
	SearchCenter = Location;
	CurrentSearchRadius = SearchRadiusBase + SearchRadiusPerStar * NivelBusqueda;
	OnSearchStarted.Broadcast(SearchCenter, CurrentSearchRadius);
}

void UWantedSubsystem::ClearWanted()
{
	if (NivelBusqueda <= 0) return;
	NivelBusqueda = 0;
	TimerBajar = 0.f;
	HiddenTimer = 0.f;
	bSearchActive = false;
	OnEstrellasCambia.Broadcast(0);
}

bool UWantedSubsystem::IsInSearchZone(FVector Location) const
{
	if (!bSearchActive || NivelBusqueda <= 0) return false;
	return FVector::Dist(Location, SearchCenter) < CurrentSearchRadius;
}

void UWantedSubsystem::Tick(float DeltaTime)
{
	if (NivelBusqueda <= 0)
	{
		HiddenTimer = 0.f;
		bSearchActive = false;
		return;
	}

	if (APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
	{
		const FVector PlayerLoc = Player->GetActorLocation();

		// GTA-style escape: if player is OUTSIDE the search zone and not seen,
		// accumulate hidden time. Enough hidden time → drop a star fast.
		if (!IsInSearchZone(PlayerLoc))
		{
			HiddenTimer += DeltaTime;
			if (HiddenTimer >= TimeToReduceStar)
			{
				HiddenTimer = 0.f;
				AumentarBusqueda(-1);
				if (NivelBusqueda <= 0)
				{
					bSearchActive = false;
					OnWantedEscaped.Broadcast();
				}
				return;
			}
		}
		else
		{
			HiddenTimer = 0.f; // In search zone — considered "seen area", no escape.
		}
	}

	// Police are slower to deescalate at night (1.5x timer).
	float TimeMult = 1.f;
	if (UGameInstance* GI = GetGameInstance())
		if (UDiaNocheSubsystem* DN = GI->GetSubsystem<UDiaNocheSubsystem>())
			if (DN->EsNoche()) TimeMult = 1.5f;

	TimerBajar -= DeltaTime / TimeMult;
	if (TimerBajar <= 0.f)
	{
		NivelBusqueda = FMath::Max(0, NivelBusqueda - 1);
		TimerBajar = TiempoBajarNivel;
		OnEstrellasCambia.Broadcast(NivelBusqueda);

		// Shrink search radius as wanted drops.
		if (NivelBusqueda > 0)
		{
			CurrentSearchRadius = SearchRadiusBase + SearchRadiusPerStar * NivelBusqueda;
			OnSearchStarted.Broadcast(SearchCenter, CurrentSearchRadius);
		}
		else
		{
			bSearchActive = false;
			OnWantedEscaped.Broadcast();
		}
	}
}