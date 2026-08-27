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
#include "Components/CapsuleComponent.h"
#include "Character/GameplayPostProcessComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Sound/SoundBase.h"
#include "AlsasuaTypes.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Initialize
// ─────────────────────────────────────────────────────────────────────────────
void UParanoiaVisualSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (MaterialesGuardia.Num() == 0)
	{
		if (UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr,
			TEXT("/Game/Materials/MI_GuardiaCivil_Body")))
			MaterialesGuardia.Add(M);
	}
	if (!MaterialPatrol)
		MaterialPatrol = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/MI_Patrol_Coche"));
	if (!MaterialFantasma)
		MaterialFantasma = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/MI_GhostGuardia"));
	if (!SirenaSound)
		SirenaSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/Sirena_Police"));
	if (!ParanoiaHeartbeat)
		ParanoiaHeartbeat = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/SC_Heartbeat.SB_Heartbeat"));
	if (!WhisperAmbiente)
		WhisperAmbiente = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/SC_Whisper.SB_Whisper"));
	if (!DistorsionAudio)
		DistorsionAudio = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/SC_Distortion"));
	if (!TrailFantasma)
		TrailFantasma = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/VFX/NS_GhostTrail"));
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
	TickEnvironmentEffects(DeltaTime);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Material overrides — NPCs → GuardiaCivil, Vehicles → Patrol
// ─────────────────────────────────────────────────────────────────────────────
void UParanoiaVisualSubsystem::UpdateVisualOverrides()
{
	UWorld* W = GetWorld();
	if (!W) return;

	const float GuardiaAlpha = (NivelParanoiaActual >= TodosGuardiaThreshold) ? 1.f :
		(NivelParanoiaActual >= GuardiaThreshold) ?
		(NivelParanoiaActual - GuardiaThreshold) / (TodosGuardiaThreshold - GuardiaThreshold) : 0.f;

	const bool bNeedPatrol = NivelParanoiaActual >= PatrolThreshold;

	for (TActorIterator<AAlsasuaNPC> It(W); It; ++It)
	{
		AAlsasuaNPC* NPC = *It;
		if (!NPC || NPC->bMuerto || NPC->bEsPolicia) continue;

		const uint32 Seed = PointerHash(NPC) & 0xFFFF;
		const float StableRoll = (Seed % 1000) / 1000.f;
		SwapNPCMaterials(NPC, StableRoll < GuardiaAlpha);
	}

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
		if (State.Originales.Num() == 0)
		{
			const int32 NumSlots = Mesh->GetNumMaterials();
			State.Originales.SetNum(NumSlots);
			for (int32 i = 0; i < NumSlots; ++i)
				State.Originales[i] = Mesh->GetMaterial(i);
		}
		const int32 NumOverride = FMath::Min(MaterialesGuardia.Num(), Mesh->GetNumMaterials());
		for (int32 i = 0; i < NumOverride; ++i)
			Mesh->SetMaterial(i, MaterialesGuardia[i]);
		State.bSwapped = true;
	}
	else if (!bGuardiaLook && State.bSwapped)
	{
		for (int32 i = 0; i < State.Originales.Num(); ++i)
			if (State.Originales[i]) Mesh->SetMaterial(i, State.Originales[i]);
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

		// Sirena visual: point light cycling azul/rojo.
		if (UPointLightComponent* Existing = Vehicle->FindComponentByClass<UPointLightComponent>())
		{
			Existing->SetVisibility(true);
			Existing->SetIntensity(5000.f);
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

		if (SirenaSound && !Vehicle->FindComponentByClass<UAudioComponent>())
		{
			UGameplayStatics::SpawnSoundAttached(
				SirenaSound, Vehicle->GetRootComponent(), NAME_None,
				FVector::ZeroVector, EAttachLocation::SnapToTarget,
				true, 1.f, 1.f, 0.f, nullptr, nullptr, true);
		}

		State.bSwapped = true;
	}
	else if (!bPatrolLook && State.bSwapped)
	{
		for (int32 i = 0; i < State.Originales.Num(); ++i)
			if (State.Originales[i]) Mesh->SetMaterial(i, State.Originales[i]);
		if (UPointLightComponent* Light = Vehicle->FindComponentByClass<UPointLightComponent>())
			Light->SetVisibility(false);
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
					if (State.Originales[i]) SkelMesh->SetMaterial(i, State.Originales[i]);
			}
			else if (UStaticMeshComponent* StatMesh = Cast<UStaticMeshComponent>(Comp))
			{
				for (int32 j = 0; j < State.Originales.Num(); ++j)
					if (State.Originales[j]) StatMesh->SetMaterial(j, State.Originales[j]);
			}
			State.bSwapped = false;
		}
	}
	MaterialStates.Empty();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Alucinaciones — guardias fantasma con incertidumbre (algunos son REALES)
// ─────────────────────────────────────────────────────────────────────────────
void UParanoiaVisualSubsystem::RegistrarVictimaCivil(FVector Ubicacion, bool bEsPolicia)
{
	FVictimaRecord R;
	R.Ubicacion = Ubicacion;
	R.TiempoMuerte = GetWorld()->GetTimeSeconds();
	R.bEsPolicia = bEsPolicia;
	VictimRegistradas.Add(R);
}

void UParanoiaVisualSubsystem::SpawnAlucinacion(FVector Ubicacion, bool bEsReal)
{
	UWorld* W = GetWorld();
	if (!W) return;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AAlsasuaNPC* Ghost = W->SpawnActor<AAlsasuaNPC>(
		AAlsasuaNPC::StaticClass(), Ubicacion + FVector(0, 0, -90), FRotator::ZeroRotator, Params);
	if (!Ghost) return;

	USkeletalMeshComponent* Mesh = Ghost->FindComponentByClass<USkeletalMeshComponent>();
	if (Mesh)
	{
		Mesh->SetCastShadow(false);
		Mesh->SetRelativeScale3D(FVector(1.05f));

		if (bEsReal)
		{
			// REAL: looks like a normal NPC but player perceives it as a ghost.
			// The mesh stays normal — the paranoia post-process makes it look wrong.
			// This is the KEY mechanic: you can't tell it's real until you attack.
		}
		else
		{
			// FAKE: translucent ghost material.
			Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			if (MaterialFantasma)
			{
				const int32 NumSlots = Mesh->GetNumMaterials();
				for (int32 i = 0; i < NumSlots; ++i)
					Mesh->SetMaterial(i, MaterialFantasma);
			}
			else if (MaterialesGuardia.Num() > 0)
			{
				const int32 NumSlots = Mesh->GetNumMaterials();
				for (int32 i = 0; i < NumSlots; ++i)
					Mesh->SetMaterial(i, MaterialesGuardia[0]);
			}

			// Niagara trail VFX.
			if (TrailFantasma)
			{
				UNiagaraComponent* NC = UNiagaraFunctionLibrary::SpawnSystemAttached(
					TrailFantasma, Mesh, NAME_None, FVector::ZeroVector,
					FRotator::ZeroRotator, EAttachLocation::SnapToTarget,
					true, true, ENCPoolMethod::AutoRelease);
				if (NC) NC->SetWorldScale3D(FVector(1.5f));
			}
		}
	}

	// Disable capsule collision for ALL ghosts (real or fake — player attacks them via weapon trace).
	if (UCapsuleComponent* Cap = Ghost->FindComponentByClass<UCapsuleComponent>())
		Cap->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Ghost->bEsPolicia = true; // they ALL look like cops

	FAlucinacionState State;
	State.Actor = Ghost;
	State.bEsReal = bEsReal;
	State.FlickerTimer = FMath::FRandRange(0.f, 2.f);
	State.DamageTimer = 0.f;
	State.FadeAlpha = 0.f;
	State.bFadingIn = true;
	AlucinacionesActivas.Add(State);
}

bool UParanoiaVisualSubsystem::OnAlucinacionAtacada(AActor* Alucinacion)
{
	for (int32 i = AlucinacionesActivas.Num() - 1; i >= 0; --i)
	{
		FAlucinacionState& Al = AlucinacionesActivas[i];
		if (Al.Actor == Alucinacion)
		{
			if (Al.bEsReal)
			{
				UGameInstance* GI = GetGameInstance();
				if (GI)
				{
					if (UApoyoPopularSubsystem* Ap = GI->GetSubsystem<UApoyoPopularSubsystem>())
					{
						Ap->RestarApoyo(8.f, TEXT("civil matado por paranoia"));
						Ap->SumarParanoia(20.f);
					}
				}
				AlucinacionesActivas.RemoveAt(i);
				return true;
			}
			else
			{
				UGameInstance* GI = GetGameInstance();
				if (GI)
				{
					if (UApoyoPopularSubsystem* Ap = GI->GetSubsystem<UApoyoPopularSubsystem>())
						Ap->SumarParanoia(5.f);
				}
				AlucinacionesActivas.RemoveAt(i);
				return false;
			}
		}
	}
	return false;
}

void UParanoiaVisualSubsystem::UpdateAlucinacionVisuals(FAlucinacionState& Al, float DeltaTime)
{
	if (!Al.Actor) return;

	// Fade in/out.
	if (Al.bFadingIn)
	{
		Al.FadeAlpha = FMath::FInterpTo(Al.FadeAlpha, 1.f, DeltaTime, 2.f);
		if (Al.FadeAlpha >= 0.95f) Al.bFadingIn = false;
	}

	// Flicker effect: periodic opacity changes for ghost-like feel.
	Al.FlickerTimer += DeltaTime;
	const float FlickerSpeed = Al.bEsReal ? 0.3f : 1.5f; // real ones flicker less
	const float FlickerAlpha = (FMath::Sin(Al.FlickerTimer * FlickerSpeed * 6.28f) * 0.5f + 0.5f);
	const float FinalAlpha = Al.FadeAlpha * FMath::Lerp(0.4f, 1.f, FlickerAlpha);

	// Apply alpha to ghost material (skip real ones — they look normal).
	if (!Al.bEsReal)
	{
		if (USkeletalMeshComponent* Mesh = Al.Actor->FindComponentByClass<USkeletalMeshComponent>())
		{
			for (int32 i = 0; i < Mesh->GetNumMaterials(); ++i)
			{
				if (UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(i)))
				{
					MID->SetScalarParameterValue(FName("Opacity"), FinalAlpha);
				}
			}
		}
	}
}

void UParanoiaVisualSubsystem::TickAlucinaciones(float DeltaTime)
{
	UWorld* W = GetWorld();
	if (!W) return;

	// Clean dead/null.
	AlucinacionesActivas.RemoveAll([](FAlucinacionState& A) { return !A.Actor || A.Actor->IsPendingKillPending(); });

	// Spawn logic.
	const bool bShouldSpawn = NivelParanoiaActual >= AlucinacionThreshold;
	const int32 ActiveCount = AlucinacionesActivas.Num();

	if (bShouldSpawn && ActiveCount < FMath::CeilToInt(MaxAlucinacionesSimultaneas))
	{
		TimerSpawnAlucinacion -= DeltaTime;
		if (TimerSpawnAlucinacion <= 0.f)
		{
			FVector SpawnLoc = FVector::ZeroVector;
			if (VictimRegistradas.Num() > 0)
			{
				const int32 Idx = FMath::RandRange(0, VictimRegistradas.Num() - 1);
				SpawnLoc = VictimRegistradas[Idx].Ubicacion;
			}
			else
			{
				if (APawn* Player = UGameplayStatics::GetPlayerPawn(W, 0))
				{
					const float Angle = FMath::FRand() * 360.f;
					const float Dist = FMath::FRandRange(2500.f, 6000.f);
					SpawnLoc = Player->GetActorLocation() +
						FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 0.f);
				}
			}

			if (!SpawnLoc.IsZero())
			{
				const bool bEsReal = (FMath::RandRange(1, 100) <= PorcentajeReal);
				SpawnAlucinacion(SpawnLoc, bEsReal);
			}

			const float ParanoiaFactor = FMath::Clamp(
				(NivelParanoiaActual - AlucinacionThreshold) / (100.f - AlucinacionThreshold), 0.f, 1.f);
			TimerSpawnAlucinacion = FMath::Lerp(IntervaloSpawnMax, IntervaloSpawnMin, ParanoiaFactor);
		}
	}

	// Tick each alucinacion.
	APawn* Player = UGameplayStatics::GetPlayerPawn(W, 0);
	for (FAlucinacionState& Al : AlucinacionesActivas)
	{
		if (!Al.Actor || !Player) continue;
		AAlsasuaNPC* Ghost = Cast<AAlsasuaNPC>(Al.Actor);
		if (!Ghost || Ghost->bMuerto) continue;

		UpdateAlucinacionVisuals(Al, DeltaTime);

		const FVector ToPlayer = Player->GetActorLocation() - Ghost->GetActorLocation();
		const float Dist = ToPlayer.Size();

		// Move toward player.
		if (Dist > 150.f)
		{
			const FVector Dir = ToPlayer.GetSafeNormal();
			Ghost->AddMovementInput(Dir, VelocidadAlucinacion / 400.f);
			Ghost->SetActorRotation(Dir.Rotation());
		}

		// Damage player when close.
		Al.DamageTimer -= DeltaTime;
		if (Dist < DistanciaDanoAlucinacion && Al.DamageTimer <= 0.f)
		{
			if (IDamageable* Dmg = Cast<IDamageable>(Player))
			{
				const float Dano = Al.bEsReal ? DanoAlucinacion * 2.f : DanoAlucinacion;
				Dmg->RecibirDano(FMath::RoundToInt(Dano), Ghost->GetActorLocation(), ETipoDano::Impacto);
			}
			Al.DamageTimer = 0.5f; // 2 hits/sec max
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  Environment effects — lights flicker, atmosphere
// ─────────────────────────────────────────────────────────────────────────────
void UParanoiaVisualSubsystem::TickEnvironmentEffects(float DeltaTime)
{
	UWorld* W = GetWorld();
	if (!W) return;

	const float P = FMath::Clamp(NivelParanoiaActual / 100.f, 0.f, 1.f);

	// Discover nearby point lights on first high-paranoia tick (cached).
	if (P > 0.3f && LightsParpadeantes.Num() == 0)
	{
		for (TActorIterator<AActor> It(W); It; ++It)
		{
			if (!*It) continue;
			TArray<UPointLightComponent*> Lights;
			It->GetComponents<UPointLightComponent>(Lights);
			for (UPointLightComponent* L : Lights)
			{
				if (L && L->GetOwner())
					LightsParpadeantes.Add(L);
			}
			if (LightsParpadeantes.Num() >= 30) break; // cap for perf
		}
	}

	// Flicker lights at high paranoia.
	if (P > 0.5f)
	{
		LightFlickerTimer -= DeltaTime;
		if (LightFlickerTimer <= 0.f)
		{
			const float FlickerChance = FMath::Lerp(0.f, 0.3f, (P - 0.5f) * 2.f);
			for (TObjectPtr<UPointLightComponent>& L : LightsParpadeantes)
			{
				if (!L || !L->GetOwner()) continue;
				const float DistToPlayer = L->GetOwner()->GetDistanceTo(
					UGameplayStatics::GetPlayerPawn(W, 0));
				if (DistToPlayer > 8000.f) continue; // only nearby lights

				if (FMath::FRand() < FlickerChance)
				{
					const float OrigIntensity = L->Intensity;
					L->SetIntensity(OrigIntensity * FMath::FRandRange(0.1f, 0.6f));
					// Restore after brief flicker via timer — use a lambda approach:
					// Schedule restoration in next tick.
				}
			}
			LightFlickerTimer = FMath::FRandRange(0.05f, 0.2f);
		}
	}

	// Sirena light color cycling on patrol vehicles.
	for (auto& Pair : MaterialStates)
	{
		if (!Pair.Value.bSwapped) continue;
		if (USceneComponent* Comp = Pair.Key)
		{
			if (AActor* Owner = Comp->GetOwner())
			{
				if (UPointLightComponent* SirenaLight = Owner->FindComponentByClass<UPointLightComponent>())
				{
					if (SirenaLight->IsVisible())
					{
						const float Cycle = FMath::Sin(W->GetTimeSeconds() * 8.f);
						SirenaLight->SetLightColor(Cycle > 0.f ?
							FLinearColor(1.f, 0.f, 0.f) : FLinearColor(0.f, 0.3f, 1.f));
						SirenaLight->SetIntensity(FMath::Abs(Cycle) * 6000.f + 1000.f);
					}
				}
			}
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  Post-process — paranoia vision + audio atmosphere
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

	PP->SetParanoiaLevel(NivelParanoiaActual / 100.f);

	const float P = FMath::Clamp(NivelParanoiaActual / 100.f, 0.f, 1.f);
	UWorld* W = GetWorld();
	APawn* Player = W ? UGameplayStatics::GetPlayerPawn(W, 0) : nullptr;

	// Heartbeat — speeds up with paranoia.
	if (NivelParanoiaActual >= AlucinacionThreshold && ParanoiaHeartbeat && Player)
	{
		HeartbeatTimer -= DeltaTime;
		if (HeartbeatTimer <= 0.f)
		{
			HeartbeatTimer = FMath::Lerp(1.2f, 0.4f, P);
			UGameplayStatics::SpawnSoundAtLocation(W, ParanoiaHeartbeat,
				Player->GetActorLocation(), FRotator::ZeroRotator,
				FMath::Lerp(0.5f, 1.2f, P), FMath::Lerp(0.8f, 1.3f, P));
		}
	}
	else { HeartbeatTimer = 0.f; }

	// Ambient whispers — intermittent, spatial, creepy.
	if (NivelParanoiaActual >= PatrolThreshold && WhisperAmbiente && Player)
	{
		WhisperTimer -= DeltaTime;
		if (WhisperTimer <= 0.f)
		{
			WhisperTimer = FMath::Lerp(10.f, 3.f, P);
			const float Angle = FMath::FRand() * 360.f;
			const float Dist = FMath::FRandRange(300.f, 1500.f);
			const FVector Loc = Player->GetActorLocation() +
				FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 0.f);
			UGameplayStatics::SpawnSoundAtLocation(W, WhisperAmbiente, Loc,
				FRotator::ZeroRotator, FMath::Lerp(0.2f, 0.6f, P),
				FMath::FRandRange(0.7f, 1.1f));
		}
	}
	else { WhisperTimer = 0.f; }

	// Audio distortion — low droning at extreme paranoia.
	if (NivelParanoiaActual >= ExtremoThreshold && DistorsionAudio && Player)
	{
		DistortionTimer -= DeltaTime;
		if (DistortionTimer <= 0.f)
		{
			DistortionTimer = FMath::FRandRange(2.f, 5.f);
			UGameplayStatics::SpawnSoundAtLocation(W, DistorsionAudio,
				Player->GetActorLocation(), FRotator::ZeroRotator, 0.4f, 0.6f);
		}
	}
	else { DistortionTimer = 0.f; }
}
