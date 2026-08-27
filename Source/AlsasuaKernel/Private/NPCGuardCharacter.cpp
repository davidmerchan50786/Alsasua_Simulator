#include "NPCGuardCharacter.h"
#include "GAS/AlsasuaAbilitySystemComponent.h"
#include "Character/Stealth/GuardDetectionComponent.h"
#include "AlsasuaTypes.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
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
	if (AttackCooldown > 0.f)
		AttackCooldown -= DeltaTime;

	if (UGuardDetectionComponent* Det = FindComponentByClass<UGuardDetectionComponent>())
		if (Det->CurrentState == EGuardAlertState::Combat)
			Attack();
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
	if (AttackCooldown > 0.f) return;
	AttackCooldown = 1.0f;

	AActor* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player) return;

	if (IDamageable* Dmg = Cast<IDamageable>(Player))
	{
		const float Dist = FVector::Dist(GetActorLocation(), Player->GetActorLocation());
		const int32 Damage = Dist < 300.f ? 18 : 10;
		Dmg->RecibirDano(Damage, GetActorLocation(), ETipoDano::Impacto);
	}
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
