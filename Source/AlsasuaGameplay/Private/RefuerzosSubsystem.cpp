// RefuerzosSubsystem.cpp
#include "RefuerzosSubsystem.h"
#include "WantedSubsystem.h"
#include "PoliciaActor.h"
#include "Character/Stealth/GuardDetectionComponent.h"
#include "ManifestacionSubsystem.h"
#include "Vehicles/AlsasuaPoliceVan.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Logging/LogMacros.h"

void URefuerzosSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (UGameInstance* GI = InWorld.GetGameInstance())
		if (UWantedSubsystem* Wn = GI->GetSubsystem<UWantedSubsystem>())
			Wn->OnEstrellasCambia.AddDynamic(this, &URefuerzosSubsystem::OnWanted);

	UGuardDetectionComponent::OnAnyGuardEnterCombat.AddDynamic(this, &URefuerzosSubsystem::OnGuardCombat);
}

void URefuerzosSubsystem::OnGuardCombat(AActor* Guard)
{
	if (UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
		if (UWantedSubsystem* Wn = GI->GetSubsystem<UWantedSubsystem>())
			Wn->AumentarBusqueda(1);
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

	// Apply multipliers (set externally by tension systems before wanted fires).
	const int32 FinalCount = FMath::CeilToInt(Cantidad * SpawnCountMultiplier);
	const float FinalRadius = RadioSpawn * SpawnRadiusMultiplier;

	for (int32 i = 0; i < FinalCount; ++i)
	{
		const float Ang = (2.f * PI / FinalCount) * i;
		const FVector Off(FMath::Cos(Ang) * FinalRadius, FMath::Sin(Ang) * FinalRadius, 0.f);
		const FVector Pos = Centro + Off + FVector(0, 0, 120.f);
		W->SpawnActor<APoliciaActor>(APoliciaActor::StaticClass(), Pos, (Off * -1).Rotation(), P);
	}

	if (Cantidad >= NivelMinimoVan)
	{
		const float AngV = FMath::FRand() * 2.f * PI;
		const FVector OffV(FMath::Cos(AngV) * (FinalRadius + 500.f), FMath::Sin(AngV) * (FinalRadius + 500.f), 0.f);
		SpawnVehiculoPolicia(Centro + OffV, (Centro - (Centro + OffV)).Rotation());

		// Roadblock — deploy spike strips ahead of the player.
		DesplegarReten(Centro);
	}

	// Helicopter surveillance at wanted 4+.
	if (Cantidad >= 4)
	{
		DesplegarHelicoptero(Centro);
	}

	// Reset after use.
	SpawnCountMultiplier = 1.f;
	SpawnRadiusMultiplier = 1.f;
}

void URefuerzosSubsystem::DesplegarReten(FVector Centro)
{
	if (ClaseSpikeStrip.IsNull()) return;
	UWorld* W = GetWorld();
	if (!W) return;

	APawn* Jugador = UGameplayStatics::GetPlayerPawn(W, 0);
	if (!Jugador) return;

	// Deploy spike strips in a line ahead of the player's facing direction.
	const FVector PlayerFwd = Jugador->GetActorForwardVector();
	const float BlockDist = 2500.f;
	const FVector BlockCenter = Jugador->GetActorLocation() + PlayerFwd * BlockDist;

	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// Line of strips perpendicular to travel direction.
	for (int32 i = 0; i < SpikeStripsPerBlock; ++i)
	{
		const float Offset = (i - (SpikeStripsPerBlock - 1) * 0.5f) * 300.f;
		const FVector Side = FVector(-PlayerFwd.Y, PlayerFwd.X, 0.f).GetSafeNormal();
		const FVector Pos = BlockCenter + Side * Offset + FVector(0, 0, 20.f);
		W->SpawnActor<AActor>(ClaseSpikeStrip.TryLoadClass<AActor>(), Pos, PlayerFwd.Rotation(), P);
	}
}

void URefuerzosSubsystem::DesplegarHelicoptero(FVector Centro)
{
	if (ClaseHelicoptero.IsNull()) return;
	UWorld* W = GetWorld();
	if (!W) return;

	UClass* Clase = ClaseHelicoptero.TryLoadClass<AActor>();
	if (!Clase) return;

	// Spawn helicopter above and behind the player, facing inward.
	const FVector Dir = FMath::RandBool() ? FVector::ForwardVector : -FVector::ForwardVector;
	const FVector Pos = Centro + Dir * 4000.f + FVector(0, 0, 5000.f);
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	if (AActor* Helo = W->SpawnActor<AActor>(Clase, Pos, (Centro - Pos).Rotation(), P))
	{
		// Set the helicopter's target to the player so it follows/searchlights.
		AActor* Target = UGameplayStatics::GetPlayerPawn(W, 0);
		// Generic target assignment via property — set by matching a common property name.
		if (Target)
		{
			if (UClass* HeloClass = Helo->GetClass())
			{
				if (FObjectProperty* TargetProp = FindFProperty<FObjectProperty>(HeloClass, TEXT("Target")))
				{
					TargetProp->SetPropertyValue_InContainer(Helo, Target);
				}
			}
		}
	}
}

void URefuerzosSubsystem::SpawnVehiculoPolicia(FVector Centro, FRotator Rotacion)
{
	UWorld* W = GetWorld();
	if (!W) return;

	// BP override si hay una asignada; si no, el AAlsasuaPoliceVan concreto
	// (malla y comportamiento táctico ya resueltos, sin depender de contenido).
	UClass* Clase = ClaseVehiculoPolicia.IsNull() ? nullptr : ClaseVehiculoPolicia.TryLoadClass<APawn>();
	if (!Clase)
	{
		SpawnPoliceVan();
		return;
	}

	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	if (APawn* Vehiculo = W->SpawnActor<APawn>(Clase, Centro, Rotacion, P))
	{
		// Order the spawned police vehicle to pursue the player. Call the
		// BlueprintCallable StartPursuit reflectively (GF_Vehiculos is not a
		// compile-time dependency of this module).
		if (AActor* Target = UGameplayStatics::GetPlayerPawn(W, 0))
			if (AController* C = Vehiculo->GetController())
			{
				const FString Cmd = FString::Printf(TEXT("StartPursuit %s"), *Target->GetName());
				C->CallFunctionByNameWithArguments(*Cmd, *GLog, nullptr);
			}
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
