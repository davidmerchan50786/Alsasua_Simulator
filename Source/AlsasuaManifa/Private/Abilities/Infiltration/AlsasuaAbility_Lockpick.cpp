#include "Abilities/Infiltration/AlsasuaAbility_Lockpick.h"
#include "Systems/Criminal/HideoutActor.h"
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

    // SphereOverlapActors en vez de GetAllActorsOfClass.
    AHideoutActor* BestHideout = nullptr;
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
        AHideoutActor* H = Cast<AHideoutActor>(Overlap.GetActor());
        if (!H || !H->bIsLocked)
        {
            continue;
        }

        const float Dist = FVector::Dist(Owner->GetActorLocation(), H->GetActorLocation());
        if (Dist < BestDist)
        {
            BestDist = Dist;
            BestHideout = H;
        }
    }

    if (BestHideout)
    {
        FTimerHandle UnlockTimer;
        FTimerDelegate UnlockDel;
        UnlockDel.BindLambda([WeakHideout = TWeakObjectPtr<AHideoutActor>(BestHideout)]()
        {
            if (WeakHideout.IsValid())
            {
                WeakHideout->OpenZulo();
            }
        });
        W->GetTimerManager().SetTimer(UnlockTimer, UnlockDel, BaseUnlockTime, false);
    }

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
