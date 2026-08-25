// RefuerzosSubsystem.cpp
#include "RefuerzosSubsystem.h"
#include "WantedSubsystem.h"
#include "PoliciaActor.h"
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
	Despachar(Nivel);   // tamaño = nivel de búsqueda
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

	// Foot police (all wanted levels).
	for (int32 i = 0; i < Cantidad; ++i)
	{
		const float Ang = (2.f * PI / Cantidad) * i;
		const FVector Off(FMath::Cos(Ang) * RadioSpawn, FMath::Sin(Ang) * RadioSpawn, 0.f);
		const FVector Pos = Centro + Off + FVector(0, 0, 120.f);
		W->SpawnActor<APoliciaActor>(APoliciaActor::StaticClass(), Pos, (Off * -1).Rotation(), P);
	}

	// Vehicle reinforcements at wanted 3+.
	if (Cantidad >= 3)
	{
		const float AngV = FMath::FRand() * 2.f * PI;
		const FVector OffV(FMath::Cos(AngV) * (RadioSpawn + 500.f), FMath::Sin(AngV) * (RadioSpawn + 500.f), 0.f);
		SpawnVehiculoPolicia(Centro + OffV, (Centro - (Centro + OffV)).Rotation());
	}
}

void URefuerzosSubsystem::SpawnVehiculoPolicia(FVector Centro, FRotator Rotacion)
{
	if (ClaseVehiculoPolicia.IsNull()) return;
	UWorld* W = GetWorld();
	if (!W) return;

	UClass* Clase = ClaseVehiculoPolicia.TryLoadClass<APawn>();
	if (!Clase) return;

	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	W->SpawnActor<APawn>(Clase, Centro, Rotacion, P);
}
