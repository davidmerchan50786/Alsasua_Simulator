#include "NPCGuardCharacter.h"
#include "GAS/AlsasuaAbilitySystemComponent.h"

ANPCGuardCharacter::ANPCGuardCharacter()
{
	AbilitySystemComponent = CreateDefaultSubobject<UAlsasuaAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
}

UAbilitySystemComponent* ANPCGuardCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}
