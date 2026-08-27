#include "NPCGuardCharacter.h"
#include "GAS/AlsasuaAbilitySystemComponent.h"
#include "AlsasuaAttributeSet.h"
#include "Character/Stealth/GuardDetectionComponent.h"
#include "AlsasuaTypes.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "AIController.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Gameplay/Detention/DetentionMinigameComponent.h"

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
	{
		if (Det->CurrentState == EGuardAlertState::Combat)
		{
			Attack();
			TickSquadTactics(DeltaTime);
		}
	}
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

	// Call backup on first attack.
	if (!bHasTarget)
	{
		bHasTarget = true;
		CurrentTarget = Player->GetActorLocation();
		CallForBackup(CurrentTarget);
	}

	const float Dist = FVector::Dist(GetActorLocation(), Player->GetActorLocation());

	// At wanted 5+, guard arrests instead of killing (melee range only).
	if (Dist < 300.f)
	{
		if (UAlsasuaAbilitySystemComponent* PlayerASC = Player->FindComponentByClass<UAlsasuaAbilitySystemComponent>())
		{
			if (const UAlsasuaAttributeSet* PlayerAttr = PlayerASC->GetSet<UAlsasuaAttributeSet>())
			{
				if (PlayerAttr->GetWantedLevel() >= 5.f)
				{
					if (UDetentionMinigameComponent* Det = Player->FindComponentByClass<UDetentionMinigameComponent>())
					{
						if (Det->CurrentState == EDetentionState::Idle)
						{
							const EInterrogationMethod Methods[] = {
								EInterrogationMethod::Electrodes,
								EInterrogationMethod::WaterBoarding,
								EInterrogationMethod::Beating,
								EInterrogationMethod::SleepDeprivation,
								EInterrogationMethod::Threats
							};
							const EInterrogationMethod Chosen = Methods[FMath::RandRange(0, 4)];
							Det->StartMinigame(30.f, 1.2f);
							Det->ApplyTortureMethod(Chosen);
							return;
						}
					}
				}
			}
		}
	}

	if (IDamageable* Dmg = Cast<IDamageable>(Player))
	{
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

// ═══════════════════════════════════════════════════════════════════════════
//  Squad tactics — flanking, backup calls, suppression fire
// ═══════════════════════════════════════════════════════════════════════════

void ANPCGuardCharacter::TickSquadTactics(float DeltaTime)
{
	if (!bHasTarget) return;

	// Suppression fire mode when close to other guards.
	SuppressionTimer -= DeltaTime;
	if (SuppressionTimer <= 0.f)
	{
		// Check if another guard is nearby (squad present).
		bool bSquadPresent = false;
		TArray<AActor*> NearbyGuards;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANPCGuardCharacter::StaticClass(), NearbyGuards);
		for (AActor* Other : NearbyGuards)
		{
			if (Other == this) continue;
			if (FVector::Dist(GetActorLocation(), Other->GetActorLocation()) < SuppressionRadius)
			{
				bSquadPresent = true;
				break;
			}
		}

		if (bSquadPresent)
		{
			EnterSuppressionMode();
			SuppressionTimer = SuppressionFireInterval;
		}
	}

	// Flanking: 50% chance to approach from an angle instead of straight-on.
	if (!bIsFlanking && FMath::FRand() < 0.005f) // rare random decision
	{
		FlankTarget(CurrentTarget);
	}
}

void ANPCGuardCharacter::CallForBackup(FVector Location)
{
	TArray<AActor*> NearbyGuards;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANPCGuardCharacter::StaticClass(), NearbyGuards);

	for (AActor* Other : NearbyGuards)
	{
		if (Other == this) continue;
		const float Dist = FVector::Dist(GetActorLocation(), Other->GetActorLocation());
		if (Dist > BackupCallRadius) continue;

		if (ANPCGuardCharacter* Guard = Cast<ANPCGuardCharacter>(Other))
		{
			Guard->IncreaseAggression(30.f);
			if (UGuardDetectionComponent* Det = Guard->FindComponentByClass<UGuardDetectionComponent>())
			{
				Det->ForceAlertState(EGuardAlertState::Combat);
				Guard->Chase(Location);
			}
		}
	}
}

void ANPCGuardCharacter::FlankTarget(FVector TargetLocation)
{
	bIsFlanking = true;

	// Calculate a position offset from direct approach angle.
	const FVector ToTarget = TargetLocation - GetActorLocation();
	const FVector Right = FVector::CrossProduct(ToTarget.GetSafeNormal(), FVector::UpVector);
	const bool bLeftSide = FMath::FRand() > 0.5f;
	const FVector FlankOffset = Right * (bLeftSide ? FlankingAngle : -FlankingAngle) * 10.f;
	const FVector FlankPos = TargetLocation + FlankOffset;

	if (AAIController* AIC = Cast<AAIController>(GetController()))
		AIC->MoveToLocation(FlankPos);
}

void ANPCGuardCharacter::EnterSuppressionMode()
{
	// Fire at the target with less accuracy but higher volume.
	AActor* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player) return;

	const float Dist = FVector::Dist(GetActorLocation(), Player->GetActorLocation());
	if (Dist > 1500.f) return; // only suppress at medium range

	// Deal reduced suppression damage (spread pattern).
	if (IDamageable* Dmg = Cast<IDamageable>(Player))
	{
		const int32 SuppressionDmg = FMath::RandRange(3, 7); // less than focused fire
		const FVector MuzzleOffset = GetActorForwardVector() * 50.f;
		Dmg->RecibirDano(SuppressionDmg, GetActorLocation() + MuzzleOffset, ETipoDano::Bala);
	}
}
