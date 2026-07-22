#include "Abilities/Infiltration/AlsasuaAbility_SilentTakedown.h"

UAlsasuaAbility_SilentTakedown::UAlsasuaAbility_SilentTakedown() {
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UAlsasuaAbility_SilentTakedown::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) {
    if(!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // Lógica de ejecución silenciosa:
    // 1. Verificar si el objetivo está de espaldas
    // 2. Aplicar daño letal o incapacitante
    // 3. Reproducir animación de derribo

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
