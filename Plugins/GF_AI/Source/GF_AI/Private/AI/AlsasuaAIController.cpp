#include "AI/AlsasuaAIController.h"

AAlsasuaAIController::AAlsasuaAIController()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AAlsasuaAIController::OnPerceptionUpdated(const TArray<AActor*>& UpdatedActors)
{
    for (AActor* Actor : UpdatedActors)
    {
        if (Actor && Actor->ActorHasTag("Player"))
        {
            CurrentAIState = 2;
        }
    }
}

void AAlsasuaAIController::HandleNoiseEvent(FVector NoiseLocation, float Intensity)
{
    if (Intensity > 0.5f)
    {
        LastNoiseLocation = NoiseLocation;
        MoveToLocation(LastNoiseLocation, 50.f);
        CurrentAIState = 1;
    }
}
