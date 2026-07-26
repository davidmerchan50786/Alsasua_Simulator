#include "Abilities/Infiltration/AlsasuaAbility_SilentTakedown.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "AlsasuaTypes.h"

UAlsasuaAbility_SilentTakedown::UAlsasuaAbility_SilentTakedown() {
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UAlsasuaAbility_SilentTakedown::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) {
    if(!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    ACharacter* Player = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
    if (!Player)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    UWorld* W = GetWorld();
    if (!W)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // SphereOverlapActors es O(k) donde k = actores en radio, vs O(n) de GetAllActorsOfClass.
    TArray<FOverlapResult> Overlaps;
    const FVector Origin = Player->GetActorLocation();
    const float SearchRadius = 300.f;
    const FCollisionShape Sphere = FCollisionShape::MakeSphere(SearchRadius);

    W->OverlapMultiByChannel(
        Overlaps,
        Origin,
        FQuat::Identity,
        ECC_Pawn,
        Sphere
    );

    AActor* BestTarget = nullptr;
    float BestDist = SearchRadius;

    for (const FOverlapResult& Overlap : Overlaps)
    {
        AActor* A = Overlap.GetActor();
        if (!IsValid(A) || A == Player)
        {
            continue;
        }

        ACharacter* C = Cast<ACharacter>(A);
        if (!C)
        {
            continue;
        }

        const float Dist = FVector::Dist(Origin, C->GetActorLocation());
        if (Dist > BestDist)
        {
            continue;
        }

        // Solo targets que están mirando hacia el jugador (detection cone).
        const FVector ToTarget = (C->GetActorLocation() - Origin).GetSafeNormal2D();
        const FVector TargetFwd = C->GetActorForwardVector();
        const float Dot = FVector::DotProduct(ToTarget, TargetFwd);

        if (Dot > 0.3f)
        {
            BestDist = Dist;
            BestTarget = A;
        }
    }

    if (BestTarget)
    {
        IDamageable* Damageable = Cast<IDamageable>(BestTarget);
        if (Damageable)
        {
            Damageable->Execute_RecibirDano(BestTarget, 100, Player->GetActorLocation(), ETipoDano::Impacto);
        }
    }

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
