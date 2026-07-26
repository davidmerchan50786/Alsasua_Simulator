#include "Core/AlsasuaChainReactionSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "AI/AlsasuaCrowdAgentComponent.h"
#include "Engine/OverlapResult.h"

void UAlsasuaChainReactionSubsystem::Tick(float DeltaTime)
{
    UWorld* W = GetWorld();
    if (!W) return;

    for (int32 i = ActiveEvents.Num()-1; i >=0; --i)
    {
        if (W->GetTimeSeconds() - ActiveEvents[i].TimeStamp > 5.0f)
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
    UWorld* W = GetWorld();
    if (!W) return;
    E.TimeStamp = W->GetTimeSeconds();
    ActiveEvents.Add(E);
    PropagateEvent(E);
}

void UAlsasuaChainReactionSubsystem::PropagateEvent(const FChainEvent& Event)
{
    // OverlapMultiByChannel es O(k) donde k = actores en radio, vs O(n) de GetAllActorsOfClass.
    TArray<FOverlapResult> Overlaps;
    const FCollisionShape Sphere = FCollisionShape::MakeSphere(Event.Radius);

    UWorld* W = GetWorld();
    if (!W) return;

    W->OverlapMultiByChannel(
        Overlaps,
        Event.Origin,
        FQuat::Identity,
        ECC_Pawn,
        Sphere
    );

    for (const FOverlapResult& Overlap : Overlaps)
    {
        AActor* A = Overlap.GetActor();
        if (!IsValid(A))
        {
            continue;
        }

        ACharacter* C = Cast<ACharacter>(A);
        if (!C)
        {
            continue;
        }

        UAlsasuaCrowdAgentComponent* Comp = C->FindComponentByClass<UAlsasuaCrowdAgentComponent>();
        if (Comp)
        {
            const float Dist = FVector::Dist(A->GetActorLocation(), Event.Origin);
            const float Influence = FMath::Clamp(1.0f - (Dist / Event.Radius), 0.f, 1.f) * Event.Strength;
            Comp->ReceiveExternalPanic(Influence);
        }
    }
}
