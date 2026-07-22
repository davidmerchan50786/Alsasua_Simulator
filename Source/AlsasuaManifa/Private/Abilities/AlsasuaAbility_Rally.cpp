#include "Abilities/AlsasuaAbility_Rally.h"
#include "AlsasuaAttributeSet.h"
#include "AbilitySystemComponent.h"

UAlsasuaAbility_Rally::UAlsasuaAbility_Rally()
{
	AbilityInputID = 2; // Map to Action Key
}

void UAlsasuaAbility_Rally::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
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

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (ASC)
	{
		ASC->SetNumericAttributeBase(UAlsasuaAttributeSet::GetPopularSupportAttribute(), ASC->GetNumericAttribute(UAlsasuaAttributeSet::GetPopularSupportAttribute()) + 5.f);
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
