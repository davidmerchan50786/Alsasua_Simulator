#include "Core/AlsasuaChainReactionSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "AI/AlsasuaCrowdAgentComponent.h"

void UAlsasuaChainReactionSubsystem::Tick(float DeltaTime)
{
    for (int32 i = ActiveEvents.Num()-1; i >=0; --i)
    {
        if (GetWorld()->GetTimeSeconds() - ActiveEvents[i].TimeStamp > 5.0f)
        {
            ActiveEvents.RemoveAt(i);
        }
    }
}

void UAlsasuaChainReactionSubsystem::EmitChainEvent(FVector Origin, float Radius, float Strength)
{
    FChainEvent E;
    E.Origin = Origin;
    E.Radius = Radius;
    E.Strength = Strength;
    E.TimeStamp = GetWorld()->GetTimeSeconds();
    ActiveEvents.Add(E);
    PropagateEvent(E);
}

void UAlsasuaChainReactionSubsystem::PropagateEvent(const FChainEvent& Event)
{
    TArray<AActor*> Characters;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), Characters);

    for (AActor* A : Characters)
    {
        float Dist = FVector::Dist(A->GetActorLocation(), Event.Origin);
        if (Dist <= Event.Radius)
        {
            ACharacter* C = Cast<ACharacter>(A);
            if (!C) continue;
            UAlsasuaCrowdAgentComponent* Comp = Cast<UAlsasuaCrowdAgentComponent>(C->GetComponentByClass(UAlsasuaCrowdAgentComponent::StaticClass()));
            if (Comp)
            {
                float Influence = FMath::Clamp(1.0f - (Dist / Event.Radius), 0.f, 1.f) * Event.Strength;
                Comp->ReceiveExternalPanic(Influence);
            }
        }
    }
}
