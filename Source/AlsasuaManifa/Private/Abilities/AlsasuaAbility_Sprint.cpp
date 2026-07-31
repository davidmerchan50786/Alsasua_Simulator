#include "Abilities/AlsasuaAbility_Sprint.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/AlsasuaAbilitySystemComponent.h"
#include "AlsasuaAttributeSet.h"

UAlsasuaAbility_Sprint::UAlsasuaAbility_Sprint()
{
	AbilityInputID = 1;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UAlsasuaAbility_Sprint::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (!Character)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Character->GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;

	if (UWorld* World = Character->GetWorld())
	{
		FTimerDelegate StaminaDrain;
		StaminaDrain.BindLambda([this, WeakChar = TWeakObjectPtr<ACharacter>(Character)]()
		{
			if (!WeakChar.IsValid()) { CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true); return; }
			UAlsasuaAbilitySystemComponent* ASC = Cast<UAlsasuaAbilitySystemComponent>(WeakChar->FindComponentByClass<UAlsasuaAbilitySystemComponent>());
			if (!ASC) { CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true); return; }
			const UAlsasuaAttributeSet* AS = ASC->GetSet<UAlsasuaAttributeSet>();
			if (!AS) { CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true); return; }

			ASC->ApplyModToAttribute(AS->GetStaminaAttribute(), EGameplayModOp::Additive, -StaminaDrainPerTick);

			if (AS->GetStamina() <= 0.f)
			{
				CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
			}
		});
		World->GetTimerManager().SetTimer(StaminaTimerHandle, StaminaDrain, DrainInterval, true);
	}
}

void UAlsasuaAbility_Sprint::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StaminaTimerHandle);
	}

	ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
