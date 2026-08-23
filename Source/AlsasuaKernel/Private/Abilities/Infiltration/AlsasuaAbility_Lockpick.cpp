#include "Abilities/Infiltration/AlsasuaAbility_Lockpick.h"
#include "Interaction/AlsasuaLockpickTargetInterface.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/OverlapResult.h"

UAlsasuaAbility_Lockpick::UAlsasuaAbility_Lockpick() {
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UAlsasuaAbility_Lockpick::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) {
    if (!ActorInfo)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    if(!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    AActor* Owner = ActorInfo->AvatarActor.Get();
    if (!Owner)
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

    // SphereOverlapActors en vez de GetAllActorsOfClass. El objetivo es
    // cualquier actor que implemente el contrato de cerradura (GF_Systems).
    AActor* BestObjetivo = nullptr;
    float BestDist = 250.f;

    TArray<FOverlapResult> Overlaps;
    const FCollisionShape Sphere = FCollisionShape::MakeSphere(BestDist);

    W->OverlapMultiByChannel(
        Overlaps,
        Owner->GetActorLocation(),
        FQuat::Identity,
        ECC_WorldDynamic,
        Sphere
    );

    for (const FOverlapResult& Overlap : Overlaps)
    {
        AActor* Candidato = Overlap.GetActor();
        if (!Candidato ||
            !Candidato->GetClass()->ImplementsInterface(UAlsasuaLockpickTarget::StaticClass()) ||
            !IAlsasuaLockpickTarget::Execute_EstaCerrado(Candidato))
        {
            continue;
        }

        const float Dist = FVector::Dist(Owner->GetActorLocation(), Candidato->GetActorLocation());
        if (Dist < BestDist)
        {
            BestDist = Dist;
            BestObjetivo = Candidato;
        }
    }

    if (BestObjetivo)
    {
        FTimerHandle UnlockTimer;
        FTimerDelegate UnlockDel;
        UnlockDel.BindWeakLambda(BestObjetivo, [Objetivo = TWeakObjectPtr<AActor>(BestObjetivo)]()
        {
            if (Objetivo.IsValid())
            {
                IAlsasuaLockpickTarget::Execute_AlForzado(Objetivo.Get());
            }
        });
        W->GetTimerManager().SetTimer(UnlockTimer, UnlockDel, BaseUnlockTime, false);
    }

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
