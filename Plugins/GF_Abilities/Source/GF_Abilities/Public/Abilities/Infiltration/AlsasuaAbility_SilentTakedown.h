#pragma once
#include "CoreMinimal.h"
#include "Abilities/AlsasuaGameplayAbility.h"
#include "AlsasuaAbility_SilentTakedown.generated.h"

UCLASS()
class GF_ABILITIES_API UAlsasuaAbility_SilentTakedown : public UAlsasuaGameplayAbility {
    GENERATED_BODY()
public:
    UAlsasuaAbility_SilentTakedown();

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};