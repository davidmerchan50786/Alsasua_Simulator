// ParanoiaVisualSubsystem.cpp
#include "ParanoiaVisualSubsystem.h"
#include "ApoyoPopularSubsystem.h"
#include "AlsasuaCore.h"
#include "AlsasuaNPC.h"
#include "Components/AlsasuaParanoiaComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/AudioComponent.h"
#include "Character/GameplayPostProcessComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Sound/SoundBase.h"
#include "Components/CapsuleComponent.h"
#include "AlsasuaTypes.h"

// ─────────────────────────────────────────────────────────────────────────────
//  BeginPlay wiring
// ─────────────────────────────────────────────────────────────────────────────
void UParanoiaVisualSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Load default materials if none assigned.
	if (MaterialesGuardia.Num() == 0)
	{
		if (UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr,
			TEXT("/Game/Materials/MI_GuardiaCivil_Body")))
			MaterialesGuardia.Add(M);
	}
	if (!MaterialPatrol)
	{
		MaterialPatrol = LoadObject<UMaterialInterface>(nullptr,
			TEXT("/Game/Materials/MI_Patrol_Coche"));
	}
	if (!SirenaSound)
	{
		SirenaSound = LoadObject<USoundBase>(nullptr,
			TEXT("/Game/Audio/Sirena_Police"));
	}
	if (!ParanoiaHeartbeat)
	{
		ParanoiaHeartbeat = LoadObject<USoundBase>(nullptr,
			TEXT("/Game/Audio/SC_Heartbeat.SB_Heartbeat"));
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  Tick — master update
// ─────────────────────────────────────────────────────────────────────────────
void UParanoiaVisualSubsystem::Tick(float DeltaTime)
{
	UWorld* W = GetWorld();
	if (!W) return;

	// Read global paranoia.
	float NewParanoia = 0.f;
	if (UGameInstance* GI = W->GetGameInstance())
	{
		if (UApoyoPopularSubsystem* Ap = GI->GetSubsystem<UApoyoPopularSubsystem>())
			NewParanoia = Ap->Paranoia;
	}

	const bool bChanged = !FMath::IsNearlyEqual(NivelParanoiaActual, NewParanoia, 0.5f);
	NivelParanoiaActual = NewParanoia;

	if (bChanged)
	{
		OnParanoiaVisualCambia.Broadcast(NivelParanoiaActual);
		UpdateVisualOverrides();
	}

	UpdateParanoiaPostProcess(DeltaTime);
	TickAlucinaciones(DeltaTime);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Material overrides — NPCs → GuardiaCivil, Vehicles → Patrol
// ─────────────────────────────────────────────────────────────────────────────
void UParanoiaVisualSubsystem::UpdateVisualOverrides()
{
	UWorld* W = GetWorld();
	if (!W) return;

	// Probability that any given NPC looks like a guard, scaled between thresholds.
	const float GuardiaAlpha = (NivelParanoiaActual >= TodosGuardiaThreshold) ? 1.f :
		(NivelParanoiaActual >= GuardiaThreshold) ?
		(NivelParanoiaActual - GuardiaThreshold) / (TodosGuardiaThreshold - GuardiaThreshold) : 0.f;

	const bool bNeedPatrol = NivelParanoiaActual >= PatrolThreshold;

	// NPCs
	for (TActorIterator<AAlsasuaNPC> It(W); It; ++It)
	{
		AAlsasuaNPC* NPC = *It;
		if (!NPC || NPC->bMuerto) continue;

		// Skip real guardias — they already look like guardias.
		if (NPC->bEsPolicia) continue;

		// Stable per-NPC decision: use actor address as pseudo-random seed.
		// Same NPC always flips at the same paranoia level (no flickering).
		const uint32 Seed = PointerHash(NPC) & 0xFFFF;
		const float StableRoll = (Seed % 1000) / 1000.f;
		const bool bShouldSwap = StableRoll < GuardiaAlpha;
		SwapNPCMaterials(NPC, bShouldSwap);
	}

	// Vehicles (pawns with "Vehicle" in name or tagged).
	for (TActorIterator<APawn> It(W); It; ++It)
	{
		APawn* Pawn = *It;
		if (!Pawn) continue;
		if (!Pawn->ActorHasTag(FName("Vehicle")) &&
			!Pawn->GetClass()->GetFName().ToString().Contains(TEXT("Vehicle")))
			continue;

		SwapVehicleMaterials(Pawn, bNeedPatrol);
	}
}

void UParanoiaVisualSubsystem::SwapNPCMaterials(AActor* NPC, bool bGuardiaLook)
{
	if (!NPC) return;

	USkeletalMeshComponent* Mesh = NPC->FindComponentByClass<USkeletalMeshComponent>();
	if (!Mesh) return;

	FOriginalMaterialState& State = MaterialStates.FindOrAdd(Mesh);

	if (bGuardiaLook && !State.bSwapped && MaterialesGuardia.Num() > 0)
	{
		// Store originals on first swap.
		if (State.Originales.Num() == 0)
		{
			const int32 NumSlots = Mesh->GetNumMaterials();
			State.Originales.SetNum(NumSlots);
			for (int32 i = 0; i < NumSlots; ++i)
				State.Originales[i] = Mesh->GetMaterial(i);
		}

		// Apply GuardiaCivil materials.
		const int32 NumOverride = FMath::Min(MaterialesGuardia.Num(), Mesh->GetNumMaterials());
		for (int32 i = 0; i < NumOverride; ++i)
			Mesh->SetMaterial(i, MaterialesGuardia[i]);

		State.bSwapped = true;
	}
	else if (!bGuardiaLook && State.bSwapped)
	{
		// Revert originals.
		for (int32 i = 0; i < State.Originales.Num(); ++i)
		{
			if (State.Originales[i])
				Mesh->SetMaterial(i, State.Originales[i]);
		}
		State.bSwapped = false;
	}
}

void UParanoiaVisualSubsystem::SwapVehicleMaterials(AActor* Vehicle, bool bPatrolLook)
{
	if (!Vehicle || !MaterialPatrol) return;

	UStaticMeshComponent* Mesh = Vehicle->FindComponentByClass<UStaticMeshComponent>();
	if (!Mesh) return;

	FOriginalMaterialState& State = MaterialStates.FindOrAdd(Mesh);

	if (bPatrolLook && !State.bSwapped)
	{
		if (State.Originales.Num() == 0)
		{
			const int32 NumSlots = Mesh->GetNumMaterials();
			State.Originales.SetNum(NumSlots);
			for (int32 i = 0; i < NumSlots; ++i)
				State.Originales[i] = Mesh->GetMaterial(i);
		}

		const int32 NumSlots = Mesh->GetNumMaterials();
		for (int32 i = 0; i < NumSlots; ++i)
			Mesh->SetMaterial(i, MaterialPatrol);

		// Sirena visual: point light azul/rojo en el techo.
		if (UPointLightComponent* Existing = Vehicle->FindComponentByClass<UPointLightComponent>())
		{
			Existing->SetVisibility(true);
			Existing->SetIntensity(5000.f);
			Existing->SetLightColor(FLinearColor(0.f, 0.3f, 1.f)); // azul
		}
		else
		{
			UPointLightComponent* Light = NewObject<UPointLightComponent>(Vehicle, TEXT("SirenaLight"));
			Light->SetupAttachment(Vehicle->GetRootComponent());
			Light->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
			Light->SetIntensity(5000.f);
			Light->SetLightColor(FLinearColor(0.f, 0.3f, 1.f));
			Light->SetAttenuationRadius(800.f);
			Light->RegisterComponent();
		}

		// Sirena audio.
		if (SirenaSound && !Vehicle->FindComponentByClass<UAudioComponent>())
		{
			UAudioComponent* Audio = UGameplayStatics::SpawnSoundAttached(
				SirenaSound, Vehicle->GetRootComponent(), NAME_None,
				FVector::ZeroVector, EAttachLocation::SnapToTarget,
				true, 1.f, 1.f, 0.f, nullptr, nullptr, true);
		}

		State.bSwapped = true;
	}
	else if (!bPatrolLook && State.bSwapped)
	{
		for (int32 i = 0; i < State.Originales.Num(); ++i)
		{
			if (State.Originales[i])
				Mesh->SetMaterial(i, State.Originales[i]);
		}

		// Hide sirena light.
		if (UPointLightComponent* Light = Vehicle->FindComponentByClass<UPointLightComponent>())
			Light->SetVisibility(false);

		// Stop sirena audio.
		if (UAudioComponent* Audio = Vehicle->FindComponentByClass<UAudioComponent>())
			Audio->Stop();

		State.bSwapped = false;
	}
}

void UParanoiaVisualSubsystem::RevertAllMaterials()
{
	for (auto& Pair : MaterialStates)
	{
		FOriginalMaterialState& State = Pair.Value;
		if (State.bSwapped)
		{
			USceneComponent* Comp = Pair.Key;
			if (USkeletalMeshComponent* SkelMesh = Cast<USkeletalMeshComponent>(Comp))
			{
				for (int32 i = 0; i < State.Originales.Num(); ++i)
					if (State.Originales[i])
						SkelMesh->SetMaterial(i, State.Originales[i]);
			}
			else if (UStaticMeshComponent* StatMesh = Cast<UStaticMeshComponent>(Comp))
			{
				for (int32 i = 0; i < State.Originales.Num(); ++i)
					if (State.Originales[i])
						StatMesh->SetMaterial(i, State.Originales[i]);
			}
			State.bSwapped = false;
		}
	}
	MaterialStates.Empty();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Alucinaciones — guardias fantasma que se acercan al jugador
// ─────────────────────────────────────────────────────────────────────────────
void UParanoiaVisualSubsystem::RegistrarVictimaCivil(FVector Ubicacion, bool bEsPolicia)
{
	FVictimaRecord R;
	R.Ubicacion = Ubicacion;
	R.TiempoMuerte = GetWorld()->GetTimeSeconds();
	R.bEsPolicia = bEsPolicia;
	VictimRegistradas.Add(R);
}

void UParanoiaVisualSubsystem::SpawnAlucinacion(FVector Ubicacion)
{
	UWorld* W = GetWorld();
	if (!W) return;

	// Spawn a ghost guardia — translucent red/blue outline.
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AAlsasuaNPC* Ghost = W->SpawnActor<AAlsasuaNPC>(
		AAlsasuaNPC::StaticClass(), Ubicacion, FRotator::ZeroRotator, Params);
	if (!Ghost) return;

	// Make it look like a guardia with ghostly tint.
	if (USkeletalMeshComponent* Mesh = Ghost->FindComponentByClass<USkeletalMeshComponent>())
	{
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCastShadow(false);
		Mesh->SetRelativeScale3D(FVector(1.1f)); // slightly larger

		// Apply a translucent material if available, otherwise use first guardia material.
		static UMaterialInterface* GhostMat = nullptr;
		if (!GhostMat)
			GhostMat = LoadObject<UMaterialInterface>(nullptr,
				TEXT("/Game/Materials/MI_GhostGuardia"));
		if (GhostMat)
		{
			const int32 NumSlots = Mesh->GetNumMaterials();
			for (int32 i = 0; i < NumSlots; ++i)
				Mesh->SetMaterial(i, GhostMat);
		}
		else if (MaterialesGuardia.Num() > 0)
		{
			const int32 NumSlots = Mesh->GetNumMaterials();
			for (int32 i = 0; i < NumSlots; ++i)
				Mesh->SetMaterial(i, MaterialesGuardia[0]);
		}
	}

	// Disable capsule collision (ghosts don't block).
	if (UCapsuleComponent* Cap = Ghost->FindComponentByClass<UCapsuleComponent>())
		Cap->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Ghost->bEsPolicia = true; // looks like a cop
	AlucinacionesActivas.Add(Ghost);
}

void UParanoiaVisualSubsystem::TickAlucinaciones(float DeltaTime)
{
	UWorld* W = GetWorld();
	if (!W) return;

	// Clean dead/null alucinaciones.
	AlucinacionesActivas.RemoveAll([](TObjectPtr<AActor>& A) { return !A || A->IsPendingKillPending(); });

	const bool bShouldSpawn = NivelParanoiaActual >= AlucinacionThreshold;

	if (bShouldSpawn)
	{
		TimerSpawnAlucinacion -= DeltaTime;
		if (TimerSpawnAlucinacion <= 0.f)
		{
			// Spawn near a recorded victim location, or random near player.
			FVector SpawnLoc = FVector::ZeroVector;
			if (VictimRegistradas.Num() > 0)
			{
				const int32 Idx = FMath::RandRange(0, VictimRegistradas.Num() - 1);
				SpawnLoc = VictimRegistradas[Idx].Ubicacion;
			}
			else
			{
				// Random offset from player.
				if (APawn* Player = UGameplayStatics::GetPlayerPawn(W, 0))
				{
					const float Angle = FMath::FRand() * 360.f;
					const float Dist = FMath::FRandRange(2000.f, 5000.f);
					SpawnLoc = Player->GetActorLocation() +
						FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 0.f);
				}
			}

			if (!SpawnLoc.IsZero())
				SpawnAlucinacion(SpawnLoc);

			// Spawn interval decreases with paranoia (more paranoia = more ghosts).
			const float ParanoiaFactor = FMath::Clamp(
				(NivelParanoiaActual - AlucinacionThreshold) / (100.f - AlucinacionThreshold), 0.f, 1.f);
			TimerSpawnAlucinacion = FMath::Lerp(8.f, 2.f, ParanoiaFactor);
		}
	}

	// Move alucinaciones toward player.
	for (TObjectPtr<AActor>& Al : AlucinacionesActivas)
	{
		if (!Al) continue;
		AAlsasuaNPC* Ghost = Cast<AAlsasuaNPC>(Al);
		if (!Ghost || Ghost->bMuerto) continue;

		APawn* Player = UGameplayStatics::GetPlayerPawn(W, 0);
		if (!Player) continue;

		const FVector ToPlayer = Player->GetActorLocation() - Ghost->GetActorLocation();
		const float Dist = ToPlayer.Size();

		// Walk toward player at guard speed.
		if (Dist > 200.f)
		{
			const FVector Dir = ToPlayer.GetSafeNormal();
			const float Speed = 400.f * DeltaTime;
			Ghost->AddMovementInput(Dir, Speed / 400.f);
			Ghost->SetActorRotation(Dir.Rotation());
		}

		// If very close, damage player (paranoia burns).
		if (Dist < 300.f)
		{
			if (APawn* P = UGameplayStatics::GetPlayerPawn(W, 0))
			{
				if (IDamageable* Dmg = Cast<IDamageable>(P))
				{
					Dmg->RecibirDano(2, Ghost->GetActorLocation(), ETipoDano::Impacto);
				}
			}
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  Post-process — paranoia vision
// ─────────────────────────────────────────────────────────────────────────────
UGameplayPostProcessComponent* UParanoiaVisualSubsystem::FindPlayerPostProcess() const
{
	UWorld* W = GetWorld();
	if (!W) return nullptr;
	APawn* Player = UGameplayStatics::GetPlayerPawn(W, 0);
	if (!Player) return nullptr;
	return Player->FindComponentByClass<UGameplayPostProcessComponent>();
}

void UParanoiaVisualSubsystem::UpdateParanoiaPostProcess(float DeltaTime)
{
	UGameplayPostProcessComponent* PP = FindPlayerPostProcess();
	if (!PP) return;

	// Delegate paranoia level to post-process component (handles desaturation, CA, vignette).
	PP->SetParanoiaLevel(NivelParanoiaActual / 100.f);

	// Heartbeat sound at high paranoia.
	if (NivelParanoiaActual >= AlucinacionThreshold && ParanoiaHeartbeat)
	{
		HeartbeatTimer -= DeltaTime;
		if (HeartbeatTimer <= 0.f)
		{
			const float P = FMath::Clamp(NivelParanoiaActual / 100.f, 0.f, 1.f);
			HeartbeatTimer = FMath::Lerp(1.2f, 0.5f, P);
			if (APawn* Player = GetWorld() ? UGameplayStatics::GetPlayerPawn(GetWorld(), 0) : nullptr)
			{
				UGameplayStatics::SpawnSoundAtLocation(GetWorld(), ParanoiaHeartbeat,
					Player->GetActorLocation(), FRotator::ZeroRotator,
					1.f, FMath::Lerp(0.6f, 1.f, P));
			}
		}
	}
	else
	{
		HeartbeatTimer = 0.f;
	}
}
