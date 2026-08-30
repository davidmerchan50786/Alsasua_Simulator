#include "AutonomousCommandsSubsystem.h"
#include "EdificioGenerado.h"
#include "AlsasuaNPC.h"
#include "AI/AlsasuaCrowdSentiment.h"
#include "ManifestacionSubsystem.h"
#include "WantedSubsystem.h"
#include "AlsasuaServiceRegistry.h"
#include "Services/INPCSocialService.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/OverlapResult.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

void UAutonomousCommandsSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
}

void UAutonomousCommandsSubsystem::Tick(float DeltaTime)
{
	TimerEval += DeltaTime;
	if (TimerEval < 2.f) return;  // Evaluate every 2s
	TimerEval = 0.f;

	EvaluarAcciones(DeltaTime);
}

void UAutonomousCommandsSubsystem::EvaluarAcciones(float DeltaTime)
{
	UWorld* W = GetWorld();
	if (!W) return;

	UAlsasuaCrowdSentiment* Sentiment = W->GetSubsystem<UAlsasuaCrowdSentiment>();
	if (!Sentiment) return;

	const float Tension = Sentiment->GlobalTension;
	const float Apoyo = Sentiment->PopularSupport;

	// Only act when tension is high enough
	if (Tension < TensionMinima) return;

	// Cooldowns
	TimerAsamblea = FMath::Max(0.f, TimerAsamblea - 2.f);
	TimerSecuestro = FMath::Max(0.f, TimerSecuestro - 2.f);
	TimerBomba = FMath::Max(0.f, TimerBomba - 2.f);
	TimerLucha = FMath::Max(0.f, TimerLucha - 2.f);

	// Asamblea: tension > 0.6, support > 30
	if (!bAsambleaActiva && TimerAsamblea <= 0.f && Tension > 0.6f && Apoyo > 30.f)
	{
		IniciarAsamblea();
	}

	// Secuestro: tension > 0.75, support > 50 (only when crowd is strong)
	if (!bSecuestroActivo && TimerSecuestro <= 0.f && Tension > 0.75f && Apoyo > 50.f)
	{
		IniciarSecuestro();
	}

	// Bombas: tension > 0.8 (desperate, high escalation)
	if (TimerBomba <= 0.f && Tension > 0.8f)
	{
		ColocarBomba();
	}

	// Lucha de clases: tension > 0.7
	if (!bLuchaActiva && TimerLucha <= 0.f && Tension > 0.7f)
	{
		IniciarLuchaClases();
	}

	// Stop actions when tension drops
	if (bAsambleaActiva && Tension < 0.4f) DetenerAsamblea();
	if (bSecuestroActivo && Tension < 0.5f) DetenerSecuestro();
	if (bLuchaActiva && Tension < 0.5f) DetenerLuchaClases();
}

// ── Asamblea ────────────────────────────────────────────────────────────────
void UAutonomousCommandsSubsystem::IniciarAsamblea()
{
	bAsambleaActiva = true;
	TimerAsamblea = CooldownAsamblea;
	OnAccionAutonoma.Broadcast(FName("Asamblea"), true);

	UE_LOG(LogTemp, Warning, TEXT("[Autonomia] ASAMBLEA: NPCs se reúnen a decidir"));

	// Visual: gathering marker at a random nearby location
	UWorld* W = GetWorld();
	if (!W) return;
	APawn* Player = UGameplayStatics::GetPlayerPawn(W, 0);
	if (!Player) return;

	const FVector Centro = Player->GetActorLocation();
	UNiagaraSystem* GatheringNS = LoadObject<UNiagaraSystem>(nullptr,
		TEXT("/Game/VFX/NS_Gathering.NS_Gathering"));
	if (GatheringNS)
	{
		UNiagaraComponent* NC = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			W, GatheringNS, Centro + FVector(0, 0, 200.f), FRotator::ZeroRotator,
			FVector(1.f), true, true);
		if (NC)
		{
			NC->SetFloatParameter(TEXT("Radius"), 500.f);
			NC->SetFloatParameter(TEXT("Intensity"), 1.0f);
			NC->SetAutoDestroy(true);
		}
	}

	// Convocar a los peatones reales con forma de protesta para que vengan
	// a asamblea; no es solo un marcador, la gente se reune a decidir.
	if (UGameInstance* GI = W->GetGameInstance())
		if (UAlsasuaServiceRegistry* Reg = GI->GetSubsystem<UAlsasuaServiceRegistry>())
			if (INPCSocialService* Peds = Reg->PedirComo<INPCSocialService>(FName("NPCPedestrians")))
				Peds->ReclutarPropensos(Centro, 800.f, NPCsAsamblea);
}

void UAutonomousCommandsSubsystem::DetenerAsamblea()
{
	bAsambleaActiva = false;
	OnAccionAutonoma.Broadcast(FName("Asamblea"), false);
	UE_LOG(LogTemp, Log, TEXT("[Autonomia] Asamblea finalizada"));
}

// ── Secuestro ───────────────────────────────────────────────────────────────
void UAutonomousCommandsSubsystem::IniciarSecuestro()
{
	UWorld* W = GetWorld();
	if (!W) return;

	AActor* Target = EncontrarNPCBanquero();
	if (!Target)
	{
		TimerSecuestro = 10.f;  // retry soon
		return;
	}

	bSecuestroActivo = true;
	TimerSecuestro = CooldownSecuestro;
	OnAccionAutonoma.Broadcast(FName("Secuestro"), true);

	// Spawn group of NPCs around the target (they "capture" the banker)
	const FVector TargetPos = Target->GetActorLocation();
	APawn* Player = UGameplayStatics::GetPlayerPawn(W, 0);
	const FVector Centro = Player ? Player->GetActorLocation() : TargetPos;

	// Reclutar primero los peatones reales con forma de protesta (Rebelde/
	// Sociable/Amable) que haya cerca; espawnea clones solo por lo que falte.
	int32 Reclutados = 0;
	if (UGameInstance* GI = W->GetGameInstance())
		if (UAlsasuaServiceRegistry* Reg = GI->GetSubsystem<UAlsasuaServiceRegistry>())
			if (INPCSocialService* Peds = Reg->PedirComo<INPCSocialService>(FName("NPCPedestrians")))
				Reclutados = Peds->ReclutarPropensos(TargetPos, 600.f, NpcSecuestro);

	const int32 AEspawnear = FMath::Max(0, NpcSecuestro - Reclutados);
	for (int32 i = 0; i < AEspawnear; ++i)
	{
		const float Ang = (2.f * PI / NpcSecuestro) * i;
		const FVector Off(FMath::Cos(Ang) * 300.f, FMath::Sin(Ang) * 300.f, 0.f);
		const FVector Pos = TargetPos + Off;

		FActorSpawnParameters SP;
		SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AAlsasuaNPC* NPC = W->SpawnActor<AAlsasuaNPC>(AAlsasuaNPC::StaticClass(), Pos, FRotator::ZeroRotator, SP);
		if (NPC)
		{
			NPC->Vida = 80.f;
			NPC->bEsManifestante = true;
		}
	}

	// Increase wanted level
	if (UGameInstance* GI = W->GetGameInstance())
		if (UWantedSubsystem* Wn = GI->GetSubsystem<UWantedSubsystem>())
			Wn->AumentarBusqueda(2);

	UE_LOG(LogTemp, Warning, TEXT("[Autonomia] SECUESTRO: Banquero capturado por %d/ %d NPCs (reclutados %d)"), NpcSecuestro, NpcSecuestro, Reclutados);

	// Auto-finish after 20s
	FTimerHandle Handle;
	W->GetTimerManager().SetTimer(Handle, [this]() { DetenerSecuestro(); }, 20.f, false);
}

void UAutonomousCommandsSubsystem::DetenerSecuestro()
{
	bSecuestroActivo = false;
	OnAccionAutonoma.Broadcast(FName("Secuestro"), false);
	UE_LOG(LogTemp, Log, TEXT("[Autonomia] Secuestro finalizado"));
}

// ── Bombas ──────────────────────────────────────────────────────────────────
void UAutonomousCommandsSubsystem::ColocarBomba()
{
	UWorld* W = GetWorld();
	if (!W) return;

	AActor* Target = EncontrarEdificioObjetivo();
	if (!Target)
	{
		TimerBomba = 15.f;
		return;
	}

	TimerBomba = CooldownBomba;
	BombasColocadas++;

	const FVector BombPos = Target->GetActorLocation() + FVector(0, 0, 50.f);

	// Spawn an incendiary charge (using existing system if available)
	// Fallback: direct damage after fuse
	UE_LOG(LogTemp, Warning, TEXT("[Autonomia] BOMBA colocada en edificio (%.0fs fuse)"), BombaTiempo);

	// Visual warning: pulsing red VFX on building
	UNiagaraSystem* BombNS = LoadObject<UNiagaraSystem>(nullptr,
		TEXT("/Game/VFX/NS_ExplosionWarning.NS_ExplosionWarning"));
	if (BombNS)
	{
		UNiagaraComponent* NC = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			W, BombNS, BombPos, FRotator::ZeroRotator,
			FVector(1.f), true, true);
		if (NC)
		{
			NC->SetFloatParameter(TEXT("Intensity"), 1.0f);
			NC->SetAutoDestroy(true);
		}
	}

	// Detonate after fuse
	FTimerHandle Handle;
	W->GetTimerManager().SetTimer(Handle, [this, W, Target]()
	{
		if (!IsValid(Target)) return;

		// Explosion VFX
		UNiagaraSystem* ExplodeNS = LoadObject<UNiagaraSystem>(nullptr,
			TEXT("/Game/VFX/NS_Explosion.NS_Explosion"));
		if (ExplodeNS)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				W, ExplodeNS, Target->GetActorLocation() + FVector(0, 0, 100.f),
				FRotator::ZeroRotator, FVector(2.f), true, true);
		}

		// Radial impulse to nearby objects
		TArray<FOverlapResult> Overlaps;
		FCollisionShape Shape = FCollisionShape::MakeSphere(1500.f);
		if (W->OverlapMultiByChannel(Overlaps, Target->GetActorLocation(), FQuat::Identity, ECC_WorldDynamic, Shape))
		{
			for (FOverlapResult& R : Overlaps)
			{
				if (UPrimitiveComponent* Prim = R.GetComponent())
				{
					if (Prim->IsSimulatingPhysics())
					{
						Prim->AddRadialImpulse(Target->GetActorLocation(), 1500.f, 5000.f, ERadialImpulseFalloff::RIF_Linear, true);
					}
				}
			}
		}

		// Damage the building (set a destructible object nearby)
		BombasColocadas = FMath::Max(0, BombasColocadas - 1);

		// Increase wanted
		if (UGameInstance* GI = W->GetGameInstance())
			if (UWantedSubsystem* Wn = GI->GetSubsystem<UWantedSubsystem>())
				Wn->AumentarBusqueda(3);

	}, BombaTiempo, false);

	OnAccionAutonoma.Broadcast(FName("Bomba"), true);
}

// ── Lucha de clases ─────────────────────────────────────────────────────────
void UAutonomousCommandsSubsystem::IniciarLuchaClases()
{
	bLuchaActiva = true;
	TimerLucha = CooldownLucha;
	OnAccionAutonoma.Broadcast(FName("LuchaDeClases"), true);

	UE_LOG(LogTemp, Warning, TEXT("[Autonomia] LUCHA DE CLASES: NPCs atacan símbolos del poder"));
}

void UAutonomousCommandsSubsystem::DetenerLuchaClases()
{
	bLuchaActiva = false;
	OnAccionAutonoma.Broadcast(FName("LuchaDeClases"), false);
	UE_LOG(LogTemp, Log, TEXT("[Autonomia] Lucha de clases finalizada"));
}

// ── Target finding ──────────────────────────────────────────────────────────
AActor* UAutonomousCommandsSubsystem::EncontrarEdificioObjetivo() const
{
	UWorld* W = GetWorld();
	if (!W) return nullptr;

	APawn* Player = UGameplayStatics::GetPlayerPawn(W, 0);
	const FVector Centro = Player ? Player->GetActorLocation() : FVector::ZeroVector;

	AActor* BestTarget = nullptr;
	float BestDist = 20000.f;  // 200m max

	for (TActorIterator<AEdificioGenerado> It(W); It; ++It)
	{
		AEdificioGenerado* E = *It;
		// Target: Commercial, Public, or Industrial (symbols of power/capitalism)
		if (E->TipoEdificio != ETipoEdificio::Comercial &&
			E->TipoEdificio != ETipoEdificio::Publico &&
			E->TipoEdificio != ETipoEdificio::Industrial)
			continue;

		const float Dist = FVector::Dist(E->GetActorLocation(), Centro);
		if (Dist < BestDist)
		{
			BestDist = Dist;
			BestTarget = E;
		}
	}

	return BestTarget;
}

AActor* UAutonomousCommandsSubsystem::EncontrarNPCBanquero() const
{
	UWorld* W = GetWorld();
	if (!W) return nullptr;

	APawn* Player = UGameplayStatics::GetPlayerPawn(W, 0);
	const FVector Centro = Player ? Player->GetActorLocation() : FVector::ZeroVector;

	// Find NPCs near commercial buildings (bankers/businesspeople)
	AActor* BestTarget = nullptr;
	float BestDist = 15000.f;  // 150m max

	for (TActorIterator<AAlsasuaNPC> It(W); It; ++It)
	{
		AAlsasuaNPC* NPC = *It;
		if (NPC->bEsPolicia || NPC->bMuerto || NPC->bEsManifestante) continue;

		const float Dist = FVector::Dist(NPC->GetActorLocation(), Centro);
		if (Dist < BestDist)
		{
			BestDist = Dist;
			BestTarget = NPC;
		}
	}

	return BestTarget;
}
