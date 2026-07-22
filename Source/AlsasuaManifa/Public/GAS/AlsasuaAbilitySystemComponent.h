#pragma once
#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AlsasuaAbilitySystemComponent.generated.h"

UCLASS()
class ALSASUAMANIFA_API UAlsasuaAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()
public:
	// Helper para aplicar un efecto de gameplay a sí mismo de forma rápida desde C++
	void ApplyEffectToSelf(TSubclassOf<class UGameplayEffect> EffectClass, float Level = 1.0f);
};
