#include "NPCGuardCharacter.h"
#include "GAS/AlsasuaAbilitySystemComponent.h"
#include "Character/Stealth/GuardDetectionComponent.h"

ANPCGuardCharacter::ANPCGuardCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	AbilitySystemComponent = CreateDefaultSubobject<UAlsasuaAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
}

UAbilitySystemComponent* ANPCGuardCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ANPCGuardCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Deescalamiento pasivo.
	if (SuspicionLevel > 0.f)
	{
		SuspicionLevel = FMath::Max(0.f, SuspicionLevel - PassiveDeescalateRate * DeltaTime);
	}
}

void ANPCGuardCharacter::ReduceAggression(float Amount)
{
	SuspicionLevel = FMath::Clamp(SuspicionLevel - Amount, 0.f, MaxAggression);

	// Si la agresión baja lo suficiente, forzar deescalamiento del GuardDetectionComponent.
	if (SuspicionLevel < 30.f)
	{
		if (UGuardDetectionComponent* Detection = FindComponentByClass<UGuardDetectionComponent>())
		{
			if (Detection->CurrentState != EGuardAlertState::Idle)
			{
				Detection->ResetToIdle();
			}
		}
	}
	else if (SuspicionLevel < 60.f)
	{
		if (UGuardDetectionComponent* Detection = FindComponentByClass<UGuardDetectionComponent>())
		{
			if (Detection->CurrentState == EGuardAlertState::Combat)
			{
				Detection->ForceAlertState(EGuardAlertState::Alert);
			}
		}
	}
}

void ANPCGuardCharacter::IncreaseAggression(float Amount)
{
	SuspicionLevel = FMath::Clamp(SuspicionLevel + Amount, 0.f, MaxAggression);

	if (SuspicionLevel > 70.f)
	{
		if (UGuardDetectionComponent* Detection = FindComponentByClass<UGuardDetectionComponent>())
		{
			if (Detection->CurrentState < EGuardAlertState::Combat)
			{
				Detection->ForceAlertState(EGuardAlertState::Combat);
			}
		}
	}
}

void ANPCGuardCharacter::TryDeescalate()
{
	ReduceAggression(20.f);
}
