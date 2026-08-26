#include "NPCGuardCharacter.h"
#include "GAS/AlsasuaAbilitySystemComponent.h"
#include "Character/Stealth/GuardDetectionComponent.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "AIController.h"
#include "Engine/World.h"

ANPCGuardCharacter::ANPCGuardCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	AbilitySystemComponent = CreateDefaultSubobject<UAlsasuaAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AAIController::StaticClass();
}

UAbilitySystemComponent* ANPCGuardCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ANPCGuardCharacter::BeginPlay()
{
	Super::BeginPlay();
	SpawnLocation = GetActorLocation();

	if (UGuardDetectionComponent* Det = FindComponentByClass<UGuardDetectionComponent>())
	{
		Det->OnAlertStateChanged.AddDynamic(this, &ANPCGuardCharacter::OnDetectionStateChanged);
	}

	if (!GetController())
		SpawnDefaultController();
}

void ANPCGuardCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (SuspicionLevel > 0.f)
		SuspicionLevel = FMath::Max(0.f, SuspicionLevel - PassiveDeescalateRate * DeltaTime);
}

void ANPCGuardCharacter::OnDetectionStateChanged(AActor* Guard, EGuardAlertState NewState, EGuardAlertState OldState)
{
	switch (NewState)
	{
	case EGuardAlertState::Idle:
		Patrol();
		break;
	case EGuardAlertState::Suspicious:
		if (UGuardDetectionComponent* Det = FindComponentByClass<UGuardDetectionComponent>())
			Investigate(Det->LastKnownPlayerLocation);
		break;
	case EGuardAlertState::Alert:
	case EGuardAlertState::Combat:
		if (UGuardDetectionComponent* Det = FindComponentByClass<UGuardDetectionComponent>())
		{
			Chase(Det->LastKnownPlayerLocation);
			bHasTarget = true;
			CurrentTarget = Det->LastKnownPlayerLocation;
		}
		break;
	}
}

void ANPCGuardCharacter::Patrol()
{
	bHasTarget = false;
	UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!Nav) return;

	FNavLocation Result;
	if (Nav->GetRandomReachablePointInRadius(SpawnLocation, PatrolRadius, Result))
	{
		if (AAIController* AIC = Cast<AAIController>(GetController()))
			AIC->MoveToLocation(Result.Location);
	}
}

void ANPCGuardCharacter::Investigate(FVector Location)
{
	bHasTarget = false;
	if (AAIController* AIC = Cast<AAIController>(GetController()))
		AIC->MoveToLocation(Location);
}

void ANPCGuardCharacter::Chase(FVector Location)
{
	if (AAIController* AIC = Cast<AAIController>(GetController()))
		AIC->MoveToLocation(Location);
}

void ANPCGuardCharacter::Attack()
{
	// Placeholder: the actual attack would deal damage via IDamageable.
	// For now, bIsChasing is enough to drive the animation and UI.
}

void ANPCGuardCharacter::ReduceAggression(float Amount)
{
	SuspicionLevel = FMath::Clamp(SuspicionLevel - Amount, 0.f, MaxAggression);

	if (UGuardDetectionComponent* Detection = FindComponentByClass<UGuardDetectionComponent>())
	{
		if (SuspicionLevel < 30.f && Detection->CurrentState != EGuardAlertState::Idle)
			Detection->ResetToIdle();
		else if (SuspicionLevel < 60.f && Detection->CurrentState == EGuardAlertState::Combat)
			Detection->ForceAlertState(EGuardAlertState::Alert);
	}
}

void ANPCGuardCharacter::IncreaseAggression(float Amount)
{
	SuspicionLevel = FMath::Clamp(SuspicionLevel + Amount, 0.f, MaxAggression);

	if (UGuardDetectionComponent* Detection = FindComponentByClass<UGuardDetectionComponent>())
	{
		if (SuspicionLevel > 70.f && Detection->CurrentState < EGuardAlertState::Combat)
			Detection->ForceAlertState(EGuardAlertState::Combat);
	}
}

void ANPCGuardCharacter::TryDeescalate()
{
	ReduceAggression(20.f);
}
