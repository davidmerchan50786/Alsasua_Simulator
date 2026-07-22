#pragma once
#include "CoreMinimal.h"
#include "Abilities/AlsasuaGameplayAbility.h"
#include "ViralPushAbility.generated.h"

UCLASS()
class ALSASUAMANIFA_API UViralPushAbility : public UAlsasuaGameplayAbility {
    GENERATED_BODY()
public:
    UViralPushAbility();
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};