#include "Abilities/Infiltration/AlsasuaAbility_Lockpick.h"
#include "Systems/Criminal/HideoutActor.h"

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

    // Detectar Zulo cercano y abrirlo
    AActor* Owner = ActorInfo->AvatarActor.Get();
    TArray<AActor*> ClosestHideouts;
    // Lógica de detección mediante Overlap o Trace
    // Llamar a Hideout->OpenZulo() tras el Delay de BaseUnlockTime

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
