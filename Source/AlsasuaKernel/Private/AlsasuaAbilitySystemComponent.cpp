#include "GAS/AlsasuaAbilitySystemComponent.h"

void UAlsasuaAbilitySystemComponent::ApplyEffectToSelf(TSubclassOf<UGameplayEffect> EffectClass, float Level)
{
	if (EffectClass)
	{
		FGameplayEffectContextHandle Context = MakeEffectContext();
		Context.AddSourceObject(GetOwner());
		FGameplayEffectSpecHandle SpecHandle = MakeOutgoingSpec(EffectClass, Level, Context);
		if (SpecHandle.IsValid())
		{
			ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}
}
