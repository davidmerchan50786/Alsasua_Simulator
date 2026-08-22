#include "AI/AlsasuaTacticManager.h"
#include "AI/AlsasuaCrowdSentiment.h"
#include "AlsasuaCore.h"
#include "Engine/World.h"

void UAlsasuaTacticManager::Tick(float DeltaTime)
{
    AccumulatedTime += DeltaTime;
    if (AccumulatedTime < EvaluationInterval) return;
    AccumulatedTime = 0.f;

    UAlsasuaCrowdSentiment* Sentiment = GetWorld() ? GetWorld()->GetSubsystem<UAlsasuaCrowdSentiment>() : nullptr;
    if (!Sentiment) return;

    const float Tension = Sentiment->GlobalTension;
    const float Support = Sentiment->PopularSupport;

    EAlsasuaTactic Suggested;

    if (Tension > 0.8f)
    {
        Suggested = EAlsasuaTactic::Scatter;
    }
    else if (Tension > 0.5f)
    {
        Suggested = EAlsasuaTactic::Blockade;
    }
    else if (Support > 50.f && Tension < 0.3f)
    {
        Suggested = EAlsasuaTactic::SitIn;
    }
    else
    {
        Suggested = EAlsasuaTactic::March;
    }

    if (Suggested != CurrentTactic)
    {
        SetGlobalTactic(Suggested);
    }
}

void UAlsasuaTacticManager::SetGlobalTactic(EAlsasuaTactic NewTactic)
{
    if(CurrentTactic == NewTactic) return;

    CurrentTactic = NewTactic;
    OnTacticChanged.Broadcast(CurrentTactic);

    UE_LOG(LogAlsasuaAI, Warning, TEXT("Táctica Global Cambiada a: %d"), (int32)NewTactic);
}
