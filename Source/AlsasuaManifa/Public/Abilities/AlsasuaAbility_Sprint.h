#pragma once
#include "CoreMinimal.h"
#include "Abilities/AlsasuaGameplayAbility.h"
#include "AlsasuaAbility_Sprint.generated.h"

UCLASS()
class ALSASUAMANIFA_API UAlsasuaAbility_Sprint : public UAlsasuaGameplayAbility
{
	GENERATED_BODY()
public:
	UAlsasuaAbility_Sprint();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
};
