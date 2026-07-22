#include "Abilities/Social/ViralPushAbility.h"
#include "Systems/Social/SocialMediaSubsystem.h"

UViralPushAbility::UViralPushAbility() {}

void UViralPushAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) {
    if (!ActorInfo)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    if (USocialMediaSubsystem* SMS = GetWorld()->GetSubsystem<USocialMediaSubsystem>()) {
        SMS->ViralMultiplier += 0.5f;
        UE_LOG(LogTemp, Warning, TEXT("¡Habilidad ViralPush activada! Multiplicador de red aumentado."));
    }
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}