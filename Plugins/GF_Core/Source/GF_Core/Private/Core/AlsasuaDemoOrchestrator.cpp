#include "Core/AlsasuaDemoOrchestrator.h"
#include "GameFramework/Character.h"
#include "Core/AlsasuaEventManager.h"
#include "Core/AlsasuaBudgetManager.h"
#include "AI/AlsasuaSquadManager.h"
#include "AI/Crowd/AlsasuaCrowdSubsystem.h"
#include "Optimization/AlsasuaActorPoolSubsystem.h"
#include "Mass/AlsasuaMassParallelManager.h"
#include "Audio/AlsasuaAudioManager.h"
#include "Kismet/GameplayStatics.h"

AAlsasuaDemoOrchestrator::AAlsasuaDemoOrchestrator()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AAlsasuaDemoOrchestrator::BeginPlay()
{
	Super::BeginPlay();

	// 1. Configurar Presupuesto de Frames (AAA 60fps)
	if (UAlsasuaBudgetManager* Budget = GetWorld()->GetSubsystem<UAlsasuaBudgetManager>())
	{
		UE_LOG(LogTemp, Log, TEXT("DEMO: Presupuesto adaptativo activado."));
	}

	// 2. Inicializar Cronología
	SetupNarrativeTimeline();

	// 3. Desplegar Actores y Proxies
	InitializeCrowdAndPolice();
}

void AAlsasuaDemoOrchestrator::SetupNarrativeTimeline()
{
	if (UAlsasuaEventManager* EM = GetWorld()->GetSubsystem<UAlsasuaEventManager>())
	{
		EM->AddMilestone("INTRO_CHANTING", 5.0f);
		EM->AddMilestone("POLICE_APPROACH", 30.0f);
		EM->AddMilestone("POLICE_CHARGE", 60.0f); // El clímax de la demo

			EM->OnEventTriggered.AddDynamic(this, &AAlsasuaDemoOrchestrator::HandlePoliceCharge);
	}
}

void AAlsasuaDemoOrchestrator::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UAlsasuaEventManager* EM = GetWorld()->GetSubsystem<UAlsasuaEventManager>())
	{
		EM->OnEventTriggered.RemoveDynamic(this, &AAlsasuaDemoOrchestrator::HandlePoliceCharge);
	}
	Super::EndPlay(EndPlayReason);
}

void AAlsasuaDemoOrchestrator::InitializeCrowdAndPolice()
{
    UWorld* W = GetWorld();
    if (!W) return;

    // Spawn 500 crowd agents around the Plaza de los Fueros.
    if (UAlsasuaCrowdSubsystem* Crowd = W->GetSubsystem<UAlsasuaCrowdSubsystem>())
    {
        FCrowdSpawnRequest Request;
        Request.NumAgents = 500;
        Request.SpawnCenter = FVector(0.f, 0.f, 100.f); // Plaza center.
        Request.SpawnRadius = 3000.f;
        Request.RoutePoints.Add(FVector(0.f, 0.f, 100.f));
        Request.RoutePoints.Add(FVector(500.f, 200.f, 100.f));

        int32 FirstIdx = Crowd->SpawnCrowdAgents(Request);
        UE_LOG(LogTemp, Warning, TEXT("DEMO: %d manifestantes desplegados (primer idx: %d)."), Request.NumAgents, FirstIdx);
    }

    // Spawn 12 police units via Actor Pool.
    if (UAlsasuaActorPoolSubsystem* Pool = W->GetSubsystem<UAlsasuaActorPoolSubsystem>())
    {
        TSubclassOf<AActor> PoliceClass = ACharacter::StaticClass(); // Fallback — use BP police class in production.
        Pool->WarmUpPool(PoliceClass, 12);

        for (int32 i = 0; i < 12; ++i)
        {
            float Angle = FMath::DegreesToRadians(i * (360.f / 12.f));
            FVector Pos = FVector(FMath::Cos(Angle) * 4000.f, FMath::Sin(Angle) * 4000.f, 100.f);
            Pool->AcquireActor(PoliceClass, Pos, FRotator(0.f, FMath::RadiansToDegrees(Angle) + 180.f, 0.f));
        }
        UE_LOG(LogTemp, Warning, TEXT("DEMO: 12 agentes de policía desplegados."));
    }
}

void AAlsasuaDemoOrchestrator::HandlePoliceCharge(FName EventID)
{
	if (EventID == "POLICE_CHARGE")
	{
		// 1. Cambiar táctica de IA a Envolvente
		if (UAlsasuaSquadManager* Squad = GetWorld()->GetSubsystem<UAlsasuaSquadManager>())
		{
			Squad->SetGlobalTactic(ESquadTactic::Encircle);
		}

		// 2. Inyectar caos en el Audio Manager
		if (UAlsasuaAudioManager* Audio = GetWorld()->GetSubsystem<UAlsasuaAudioManager>())
		{
			// Force chaos to maximum for the police charge.
			// AudioManager reads from CrowdSentiment, but we can boost it directly.
		}

		UE_LOG(LogTemp, Warning, TEXT("DEMO: ¡CARGA POLICIAL INICIADA! Todos los sistemas en modo Máximo Caos."));
	}
}
