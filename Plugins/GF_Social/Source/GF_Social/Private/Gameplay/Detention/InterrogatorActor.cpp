#include "Gameplay/Detention/InterrogatorActor.h"

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
    OnInterrogationStarted.Broadcast(Target, Duration, DifficultyMultiplier);
}
