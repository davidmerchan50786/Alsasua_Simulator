#include "Gameplay/Detention/InterrogatorActor.h"
#include "Gameplay/Detention/DetentionMinigameComponent.h"

AInterrogatorActor::AInterrogatorActor()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AInterrogatorActor::BeginPlay()
{
    Super::BeginPlay();
}

void AInterrogatorActor::StartInterrogation(AActor* Target, float Duration)
{
    if (!Target) return;
    TargetActor = Target;
    UDetentionMinigameComponent* Minigame = Target->FindComponentByClass<UDetentionMinigameComponent>();
    if (Minigame)
    {
        Minigame->StartMinigame(Duration, DifficultyMultiplier);
    }
}
