#include "Core/Manifa/ManifaManager.h"
#include "AI/AlsasuaCrowdAgentComponent.h"
#include "AI/AlsasuaCrowdSentiment.h"
#include "AI/Crowd/AlsasuaManifestacionManager.h"
#include "CrowdAudioManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameStateBase.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

void UManifaManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UManifaManager::TriggerManifestation(FVector CenterLocation)
{
    // Mark existing crowd agents as following
    TArray<AActor*> NPCs;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("Civilian"), NPCs);

    ActiveProtesters = 0;
    for (AActor* Actor : NPCs)
    {
        if (UAlsasuaCrowdAgentComponent* Crowd = Actor->FindComponentByClass<UAlsasuaCrowdAgentComponent>())
        {
            Crowd->CurrentState = ECrowdAgentState::Following;
            ActiveProtesters++;
        }
    }
    bMegaActiva = ActiveProtesters > 0;
    ManifestacionCentro = CenterLocation;
    OnManifaStateChanged.Broadcast(bMegaActiva);

    // Launch ISMC mega-crowd via ManifestacionManager (GF_AI)
    if (UAlsasuaManifestacionManager* Mgr = GetWorld()->GetSubsystem<UAlsasuaManifestacionManager>())
    {
        if (Mgr->GetTensionActual() == EManifestacionTension::Pacifica /* not already active */)
        {
            FManifestacionConfig Cfg;
            Cfg.TotalManifestantes = 500;
            Cfg.NumLeaders = 10;
            Cfg.SpawnRadius = 2000.f;
            Cfg.MarchSpeed = 120.f;
            Cfg.RouteWaypoints = { CenterLocation, CenterLocation + FVector(5000, 0, 0), CenterLocation + FVector(5000, 5000, 0) };
            Mgr->IniciarManifestacion(Cfg);
        }
    }
}

void UManifaManager::Tick(float DeltaTime)
{
    Momentum = FMath::Clamp(Momentum, 0.f, 100.f);

    if (!GetWorld()) return;

    // Sync CrowdSentiment → GameState.CrowdTension
    if (UAlsasuaCrowdSentiment* Sent = GetWorld()->GetSubsystem<UAlsasuaCrowdSentiment>())
    {
        AGameStateBase* GS = GetWorld()->GetGameState();
        if (GS)
        {
            UFunction* Fn = GS->FindFunction(FName("SetCrowdTension"));
            if (Fn)
            {
                struct { float NewTension; } Params;
                Params.NewTension = FMath::Clamp(Sent->GlobalTension, 0.f, 1.f);
                GS->ProcessEvent(Fn, &Params);
            }
        }

        // Drive CrowdAudioManager layers based on tension + protester count
        if (UCrowdAudioManager* Audio = GetWorld()->GetSubsystem<UCrowdAudioManager>())
        {
            float Dist = 10000.f;
            if (APawn* P = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
            {
                Dist = FVector::Dist(P->GetActorLocation(), ManifestacionCentro);
            }
            Audio->UpdateCrowdAudio(ActiveProtesters, Sent->GlobalTension, Dist);
        }
    }
}

void UManifaManager::UpdateManifestationStrength(float DeltaPopularSupport)
{
    Momentum = FMath::Clamp(Momentum + DeltaPopularSupport, 0.f, 100.f);
}
