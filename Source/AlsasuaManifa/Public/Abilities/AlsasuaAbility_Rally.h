#pragma once
#include "CoreMinimal.h"
#include "Abilities/AlsasuaGameplayAbility.h"
#include "AlsasuaAbility_Rally.generated.h"

UCLASS()
class ALSASUAMANIFA_API UAlsasuaAbility_Rally : public UAlsasuaGameplayAbility
{
	GENERATED_BODY()
public:
	UAlsasuaAbility_Rally();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
