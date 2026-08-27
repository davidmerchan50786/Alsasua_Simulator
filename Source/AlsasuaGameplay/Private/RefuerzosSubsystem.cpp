// RefuerzosSubsystem.cpp
#include "RefuerzosSubsystem.h"
#include "WantedSubsystem.h"
#include "PoliciaActor.h"
#include "ManifestacionSubsystem.h"
#include "Vehicles/AlsasuaPoliceVan.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

void URefuerzosSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (UGameInstance* GI = InWorld.GetGameInstance())
		if (UWantedSubsystem* Wn = GI->GetSubsystem<UWantedSubsystem>())
			Wn->OnEstrellasCambia.AddDynamic(this, &URefuerzosSubsystem::OnWanted);
}

void URefuerzosSubsystem::OnWanted(int32 Nivel)
{
	if (Nivel <= 0) return;
	const float Ahora = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	if (Ahora - UltimaOleada < Cooldown) return;
	UltimaOleada = Ahora;

	// Scale count with manifestation tension
	int32 ExtraCops = 0;
	if (UGameInstance* GI = GetWorld()->GetGameInstance())
		if (UManifestacionSubsystem* Mf = GI->GetSubsystem<UManifestacionSubsystem>())
		{
			const float Tension = Mf->GetTension();
			if (Tension > 0.5f)
				ExtraCops = FMath::RoundToInt((Tension - 0.5f) * 2.f * EscalacionTension);
		}

	Despachar(Nivel + ExtraCops);

	// Spawn police van at high wanted levels
	if (Nivel >= NivelMinimoVan)
	{
		SpawnPoliceVan();
	}
}

void URefuerzosSubsystem::Despachar(int32 Cantidad)
{
	UWorld* W = GetWorld();
	if (!W) return;
	APawn* Jugador = UGameplayStatics::GetPlayerPawn(W, 0);
	if (!Jugador) return;

	const FVector Centro = Jugador->GetActorLocation();
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	for (int32 i = 0; i < Cantidad; ++i)
	{
		const float Ang = (2.f * PI / Cantidad) * i;
		const FVector Off(FMath::Cos(Ang) * RadioSpawn, FMath::Sin(Ang) * RadioSpawn, 0.f);
		const FVector Pos = Centro + Off + FVector(0, 0, 120.f);
		W->SpawnActor<APoliciaActor>(APoliciaActor::StaticClass(), Pos, (Off * -1).Rotation(), P);
	}
}

void URefuerzosSubsystem::SpawnPoliceVan()
{
	UWorld* W = GetWorld();
	if (!W) return;
	APawn* Jugador = UGameplayStatics::GetPlayerPawn(W, 0);
	if (!Jugador) return;

	const FVector Centro = Jugador->GetActorLocation();
	// Spawn van 40m away, facing the player
	const float Ang = FMath::FRand() * 2.f * PI;
	const FVector Off(FMath::Cos(Ang) * 4000.f, FMath::Sin(Ang) * 4000.f, 0.f);
	const FVector Pos = Centro + Off;

	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AAlsasuaPoliceVan* Van = W->SpawnActor<AAlsasuaPoliceVan>(
		AAlsasuaPoliceVan::StaticClass(), Pos, (-Off).Rotation(), P);
	if (Van)
	{
		Van->MoveToLocationTactic(Centro);
	}
}
