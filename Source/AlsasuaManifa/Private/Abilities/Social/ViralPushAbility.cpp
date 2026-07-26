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

    UWorld* W = GetWorld();
    if (USocialMediaSubsystem* SMS = W ? W->GetSubsystem<USocialMediaSubsystem>() : nullptr) {
        SMS->ViralMultiplier = FMath::Min(SMS->ViralMultiplier + 0.5f, 5.0f);
        UE_LOG(LogTemp, Warning, TEXT("¡Habilidad ViralPush activada! Multiplicador de red: %.1f"), SMS->ViralMultiplier);
    }
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}