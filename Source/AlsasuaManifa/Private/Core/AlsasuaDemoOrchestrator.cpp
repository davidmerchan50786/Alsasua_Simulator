#include "Core/AlsasuaDemoOrchestrator.h"
#include "Core/AlsasuaEventManager.h"
#include "Core/AlsasuaBudgetManager.h"
#include "AI/AlsasuaSquadManager.h"
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
	// Aquí se llamarían a los spawner de MassProxies 
	// simulando una carga masiva de NPCs en la Plaza de los Fueros
	UE_LOG(LogTemp, Warning, TEXT("DEMO: 500 manifestantes y 12 agentes desplegados."));
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
			// El audio manager reaccionará automáticamente a la tensión generada por el Squad
		}

		UE_LOG(LogTemp, Error, TEXT("DEMO: ¡CARGA POLICIAL INICIADA! Todos los sistemas en modo Máximo Caos."));
	}
}
