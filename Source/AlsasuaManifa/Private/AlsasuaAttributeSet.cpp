#include "AlsasuaAttributeSet.h"
#include "GameplayEffectExtension.h"

UAlsasuaAttributeSet::UAlsasuaAttributeSet()
	: Health(100.f), MaxHealth(100.f), Stamina(120.f), MaxStamina(120.f), WantedLevel(0.f), PopularSupport(10.f)
{}

void UAlsasuaAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
	if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
	{
		SetStamina(FMath::Clamp(GetStamina(), 0.0f, GetMaxStamina()));
	}
}
