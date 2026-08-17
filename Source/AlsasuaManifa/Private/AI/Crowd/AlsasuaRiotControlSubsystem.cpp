// AlsasuaRiotControlSubsystem.cpp
// ═══════════════════════════════════════════════════════════════════════════
//  Implementación del subsistema de control de disturbios.
//  Port del ControladorDisturbios de Unity a UE 5.4 C++.
// ═══════════════════════════════════════════════════════════════════════════

#include "AI/Crowd/AlsasuaRiotControlSubsystem.h"
#include "AlsasuaCore.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/GameplayStatics.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Initialize: se llama al registrar el subsistema.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaRiotControlSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// No intentar acceder a GetWorld() ni crear componentes aquí;
	// el World no está completamente asignado durante Initialize().
	// La creación del ISMC y el timer se difieren a PostInitialize() o OnWorldBeginPlay().
}

void UAlsasuaRiotControlSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	// Crear ISMC para marcadores de bengala.
	UWorld* World = &InWorld;

	FlareMarkerISMC = NewObject<UInstancedStaticMeshComponent>(World);
	FlareMarkerISMC->RegisterComponentWithWorld(World);
	FlareMarkerISMC->SetMobility(EComponentMobility::Movable);
	FlareMarkerISMC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FlareMarkerISMC->CastShadow = false;
	FlareMarkerISMC->SetCanEverAffectNavigation(false);

	// La primera ruta era /Engine/EngineMeshes/SM_Cube, que no existe: siempre
	// devolvía null y siempre acababa aquí. Las formas básicas del motor están
	// en /Engine/BasicShapes/, y el nombre del asset se repite en la ruta.
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(
		nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"), nullptr, LOAD_None);
	if (Mesh != nullptr)
	{
		FlareMarkerISMC->SetStaticMesh(Mesh);
	}

	// Registrar timer de actualización (~30Hz).
	const float TickInterval = 1.f / 30.f;
	World->GetTimerManager().SetTimer(TickTimerHandle, this,
		&UAlsasuaRiotControlSubsystem::InternalTick, TickInterval, true);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Deinitialize: limpia todos los disturbios y efectos.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaRiotControlSubsystem::Deinitialize()
{
	// Limpiar el timer antes de destruir.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TickTimerHandle);
	}

	DetenerTodosDisturbios();

	if (FlareMarkerISMC != nullptr)
	{
		FlareMarkerISMC->ClearInstances();
	}

	Super::Deinitialize();
}

// ─────────────────────────────────────────────────────────────────────────────
//  ForzarDisturbio: inicia manualmente un disturbio.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaRiotControlSubsystem::ForzarDisturbio(const FVector& Epicenter)
{
	StartRiot(Epicenter, ERiotEscalation::Gathering);
}

// ─────────────────────────────────────────────────────────────────────────────
//  DetenerTodosDisturbios: finaliza todos los disturbios activos.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaRiotControlSubsystem::DetenerTodosDisturbios()
{
	for (int32 i = ActiveRiots.Num() - 1; i >= 0; --i)
	{
		EndRiot(i);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  GetHighestEscalation: devuelve el nivel más alto de todos los disturbios.
// ─────────────────────────────────────────────────────────────────────────────
ERiotEscalation UAlsasuaRiotControlSubsystem::GetHighestEscalation() const
{
	ERiotEscalation Highest = ERiotEscalation::Patrol;

	for (const FRiotInstance& Riot : ActiveRiots)
	{
		if (static_cast<uint8>(Riot.Escalation) > static_cast<uint8>(Highest))
		{
			Highest = Riot.Escalation;
		}
	}

	return Highest;
}

// ─────────────────────────────────────────────────────────────────────────────
//  SetRiotProbability: ajusta la probabilidad base en tiempo real.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaRiotControlSubsystem::SetRiotProbability(float NewProbability)
{
	Config.BaseRiotProbability = FMath::Clamp(NewProbability, 0.f, 1.f);
}

// ─────────────────────────────────────────────────────────────────────────────
//  ProbabilisticRiotCheck: chequeo probabilístico cada N segundos.
//  Port directo del Update() del ControladorDisturbios de Unity.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaRiotControlSubsystem::ProbabilisticRiotCheck()
{
	const float ModifiedProb = GetModifiedProbability();

	if (FMath::FRand() < ModifiedProb)
	{
		// Buscar una posición aleatoria para el epicentro.
		UWorld* World = GetWorld();
		if (World != nullptr)
		{
			// Usar un punto aleatorio en el mundo (centro del nivel ± offset).
			const FVector RandomOffset = FVector(
				FMath::RandRange(-10000.f, 10000.f),
				FMath::RandRange(-10000.f, 10000.f),
				0.f);

			// Proyectar sobre el suelo.
			FHitResult Hit;
			const FVector Start = RandomOffset + FVector(0.f, 0.f, 50000.f);
			const FVector End = RandomOffset - FVector(0.f, 0.f, 50000.f);

			FVector Epicenter = RandomOffset;
			if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility))
			{
				Epicenter = Hit.ImpactPoint;
			}

			StartRiot(Epicenter, ERiotEscalation::Gathering);
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  StartRiot: crea un nuevo disturbio con bengala y reclutamiento.
//  Port de IniciarDisturbioEnCalle + InstanciarBengala de Unity.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaRiotControlSubsystem::StartRiot(const FVector& Epicenter, ERiotEscalation InitialEscalation)
{
	FRiotInstance NewRiot;
	NewRiot.Epicenter = Epicenter;
	NewRiot.Escalation = InitialEscalation;
	NewRiot.ElapsedTime = 0.f;
	NewRiot.MaxDuration = Config.FlareDuration;
	NewRiot.bHasFlare = true;
	NewRiot.bIsActive = true;

	// Crear efectos de bengala.
	SpawnFlareEffects(NewRiot);

	// Reclutar NPCs cercanos.
	RecruitNearbyNPCs(NewRiot);

	ActiveRiots.Add(NewRiot);

	OnRiotStarted.Broadcast(Epicenter, InitialEscalation);

	UE_LOG(LogAlsasuaAI, Log, TEXT("[RiotControl] Disturbio iniciado en %s (Escalada: %d, Reclutas: %d)"),
		*Epicenter.ToString(), static_cast<int32>(InitialEscalation), NewRiot.Recruits.Num());
}

// ─────────────────────────────────────────────────────────────────────────────
//  EndRiot: finaliza un disturbio y limpia sus efectos.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaRiotControlSubsystem::EndRiot(int32 RiotIndex)
{
	if (!ActiveRiots.IsValidIndex(RiotIndex))
	{
		return;
	}

	FRiotInstance& Riot = ActiveRiots[RiotIndex];

	// Limpiar efectos de bengala.
	CleanupFlareEffects(Riot);

	OnRiotEnded.Broadcast(Riot.Epicenter);

	UE_LOG(LogAlsasuaAI, Log, TEXT("[RiotControl] Disturbio finalizado en %s tras %.1fs"),
		*Riot.Epicenter.ToString(), Riot.ElapsedTime);

	ActiveRiots.RemoveAt(RiotIndex);

	// Activar cooldown.
	CooldownTimer = Config.RiotCooldown;
}

// ─────────────────────────────────────────────────────────────────────────────
//  SpawnFlareEffects: crea luz puntual + partículas de humo rojo.
//  Port directo de InstanciarBengala del Unity script.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaRiotControlSubsystem::SpawnFlareEffects(FRiotInstance& Riot)
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	// ── Luz puntual roja ─────────────────────────────────────────────────────
	Riot.FlareLight = NewObject<UPointLightComponent>(World);
	if (Riot.FlareLight != nullptr)
	{
		Riot.FlareLight->RegisterComponentWithWorld(World);
		Riot.FlareLight->SetWorldLocation(Riot.Epicenter + FVector(0.f, 0.f, 20.f));
		Riot.FlareLight->SetLightColor(FLinearColor(1.f, 0.f, 0.f));
		Riot.FlareLight->SetIntensity(Config.FlareLightIntensity);
		Riot.FlareLight->SetAttenuationRadius(Config.FlareLightRange);
		Riot.FlareLight->SetCastShadows(false);
	}

	// ── Partículas de humo rojo ──────────────────────────────────────────────
	// Buscar sistema de partículas por defecto.
	UParticleSystem* PSFlare = LoadObject<UParticleSystem>(
		nullptr,
		TEXT("/Game/Effects/P_Fire"),  // Fallback
		nullptr,
		LOAD_None);

	if (PSFlare != nullptr)
	{
		Riot.FlareParticles = UGameplayStatics::SpawnEmitterAtLocation(
			World,
			PSFlare,
			Riot.Epicenter + FVector(0.f, 0.f, 20.f),
			FRotator::ZeroRotator,
			true,
			EPSCPoolMethod::AutoRelease);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  CleanupFlareEffects: destruye la luz y las partículas de una bengala.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaRiotControlSubsystem::CleanupFlareEffects(FRiotInstance& Riot)
{
	if (Riot.FlareLight != nullptr && IsValid(Riot.FlareLight))
	{
		Riot.FlareLight->DestroyComponent();
	}
	Riot.FlareLight = nullptr;

	if (Riot.FlareParticles != nullptr)
	{
		Riot.FlareParticles->DeactivateSystem();
		Riot.FlareParticles = nullptr;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  RecruitNearbyNPCs: recluta NPCs en un radio alrededor del epicentro.
//  Port del loop de FindObjectsOfType + NavMeshAgent del Unity script.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaRiotControlSubsystem::RecruitNearbyNPCs(FRiotInstance& Riot)
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	// Buscar actores con pawn en un radio alrededor del epicentro.
	TArray<FOverlapResult> Overlaps;
	const FCollisionShape Sphere = FCollisionShape::MakeSphere(Config.RecruitmentRadius);

	if (World->OverlapMultiByChannel(
		Overlaps,
		Riot.Epicenter,
		FQuat::Identity,
		ECC_Pawn,
		Sphere))
	{
		int32 RecruitCount = 0;

		for (const FOverlapResult& Overlap : Overlaps)
		{
			if (RecruitCount >= Config.MaxRecruitsPerRiot)
			{
				break;
			}

			AActor* HitActor = Overlap.GetActor();
			APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
			APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;
			if (HitActor == nullptr || HitActor == PlayerPawn)
			{
				continue;
			}

			// Verificar que es un NPC (no el jugador).
			APawn* Pawn = Cast<APawn>(HitActor);
			if (Pawn != nullptr && Pawn->GetController() != nullptr &&
				!Pawn->GetController()->IsPlayerController())
			{
				FRiotRecruitData Recruit;
				Recruit.Position = HitActor->GetActorLocation();
				Recruit.Velocity = FVector::ZeroVector;
				Recruit.RiotEpicenter = Riot.Epicenter;
				Recruit.bIsActive = true;
				Recruit.Lifetime = 0.f;

				Riot.Recruits.Add(Recruit);
				++RecruitCount;
			}
		}
	}

	// Añadir reclutas ficticios si no hay suficientes NPCs reales (para efecto visual).
	const int32 ExtraNeeded = Config.MinNPCsForRiot - Riot.Recruits.Num();
	for (int32 i = 0; i < ExtraNeeded; ++i)
	{
		const float Angle = (2.f * PI * i) / FMath::Max(1, Config.MinNPCsForRiot);
		const FVector Offset = FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * 200.f;

		FRiotRecruitData Recruit;
		Recruit.Position = Riot.Epicenter + Offset;
		Recruit.Velocity = FVector::ZeroVector;
		Recruit.RiotEpicenter = Riot.Epicenter;
		Recruit.bIsActive = true;
		Recruit.Lifetime = 0.f;

		Riot.Recruits.Add(Recruit);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  TickRiot: actualiza la lógica de un disturbio activo.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaRiotControlSubsystem::TickRiot(FRiotInstance& Riot, float DeltaTime)
{
	Riot.ElapsedTime += DeltaTime;

	// Verificar si el disturbio ha expirado.
	if (Riot.ElapsedTime >= Riot.MaxDuration)
	{
		Riot.bIsActive = false;
		return;
	}

	// Escalada probabilística.
	if (Riot.Escalation != ERiotEscalation::FullRiot)
	{
		if (FMath::FRand() < Config.EscalationProbability * DeltaTime)
		{
			const ERiotEscalation OldEscalation = Riot.Escalation;
			const int32 CurrentLevel = static_cast<int32>(Riot.Escalation);
			Riot.Escalation = static_cast<ERiotEscalation>(
				FMath::Min(CurrentLevel + 1, static_cast<int32>(ERiotEscalation::FullRiot)));

			if (OldEscalation != Riot.Escalation)
			{
				OnEscalationChanged.Broadcast(Riot.Escalation);

				UE_LOG(LogAlsasuaAI, Log, TEXT("[RiotControl] Escalada en %s: nivel %d → %d"),
					*Riot.Epicenter.ToString(), CurrentLevel, static_cast<int32>(Riot.Escalation));
			}
		}
	}

	// Oscilar la intensidad de la luz de bengala.
	if (Riot.FlareLight != nullptr && IsValid(Riot.FlareLight))
	{
		const float FlickerIntensity = Config.FlareLightIntensity *
			(0.7f + 0.3f * FMath::Sin(Riot.ElapsedTime * 8.f));
		Riot.FlareLight->SetIntensity(FlickerIntensity);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  TickRecruits: mueve los reclutas en círculo alrededor del epicentro.
//  Port del loop de NavMeshAgent.SetDestination del Unity script.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaRiotControlSubsystem::TickRecruits(FRiotInstance& Riot, float DeltaTime)
{
	for (int32 i = 0; i < Riot.Recruits.Num(); ++i)
	{
		FRiotRecruitData& Recruit = Riot.Recruits[i];
		if (!Recruit.bIsActive)
		{
			continue;
		}

		Recruit.Lifetime += DeltaTime;

		// Mover en círculo alrededor del epicentro.
		const float Angle = Recruit.Lifetime * 0.5f + i * (2.f * PI / Riot.Recruits.Num());
		const float CircleRadius = 200.f;
		const FVector Target = Riot.Epicenter +
			FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * CircleRadius;

		const FVector MoveDir = (Target - Recruit.Position).GetSafeNormal2D();
		const float Speed = 100.f; // Velocidad lenta de disturbio.

		Recruit.Velocity = FMath::VInterpTo(
			Recruit.Velocity,
			MoveDir * Speed,
			DeltaTime,
			3.f);

		Recruit.Position += Recruit.Velocity * DeltaTime;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  SyncFlareMarkers: actualiza el ISMC de marcadores de bengala.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaRiotControlSubsystem::SyncFlareMarkers()
{
	if (FlareMarkerISMC == nullptr)
	{
		return;
	}

	FlareMarkerISMC->ClearInstances();

	for (const FRiotInstance& Riot : ActiveRiots)
	{
		if (!Riot.bIsActive || !Riot.bHasFlare)
		{
			continue;
		}

		// Crear un marcador cúbico rojo sobre la bengala.
		const FVector Loc = Riot.Epicenter + FVector(0.f, 0.f, 300.f);
		const FTransform T(FRotator::ZeroRotator, Loc, FVector(1.f, 1.f, 1.f));
		FlareMarkerISMC->AddInstance(T, true);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  GetModifiedProbability: ajusta la probabilidad según la escalada actual.
// ─────────────────────────────────────────────────────────────────────────────
float UAlsasuaRiotControlSubsystem::GetModifiedProbability() const
{
	float Probability = Config.BaseRiotProbability;

	// Aumentar la probabilidad si ya hay disturbios activos (efecto contagio).
	const int32 ActiveCount = ActiveRiots.Num();
	if (ActiveCount > 0)
	{
		Probability *= (1.f + ActiveCount * 0.5f);
	}

	// Reducir la probabilidad durante el cooldown.
	if (CooldownTimer > 0.f)
	{
		Probability *= FMath::Max(0.f, 1.f - (CooldownTimer / Config.RiotCooldown));
	}

	return FMath::Clamp(Probability, 0.f, 1.f);
}

// ─────────────────────────────────────────────────────────────────────────────
//  InternalTick: método registrado en el TimerManager (~30Hz).
//  Port del Update() del ControladorDisturbios de Unity.
// ─────────────────────────────────────────────────────────────────────────────
void UAlsasuaRiotControlSubsystem::InternalTick()
{
	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown || World->GetNetMode() == NM_Client)
	{
		return;
	}

	const float DeltaTime = 1.f / 30.f; // Intervalo fijo del timer.

	// ── 1. Cooldown post-disturbio ──────────────────────────────────────────
	if (CooldownTimer > 0.f)
	{
		CooldownTimer = FMath::Max(0.f, CooldownTimer - DeltaTime);
	}

	// ── 2. Chequeo probabilístico (port del crono del Unity script) ─────────
	CheckTimer += DeltaTime;
	if (CheckTimer >= Config.CheckInterval)
	{
		CheckTimer = 0.f;
		ProbabilisticRiotCheck();
	}

	// ── 3. Actualizar disturbios activos ────────────────────────────────────
	TArray<int32> RiotesAFinalizar;
	for (int32 i = 0; i < ActiveRiots.Num(); ++i)
	{
		FRiotInstance& Riot = ActiveRiots[i];
		TickRiot(Riot, DeltaTime);
		TickRecruits(Riot, DeltaTime);

		if (!Riot.bIsActive)
		{
			RiotesAFinalizar.Add(i);
		}
	}
	// Finalizar fuera del bucle para evitar que RemoveAt invalide referencias.
	for (int32 i = RiotesAFinalizar.Num() - 1; i >= 0; --i)
	{
		EndRiot(RiotesAFinalizar[i]);
	}

	// ── 4. Sincronizar marcadores visuales ──────────────────────────────────
	SyncFlareMarkers();
}
