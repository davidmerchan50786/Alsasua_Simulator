#include "GuardiaCivil.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "AIController.h"
#include "Engine/World.h"

AGuardiaCivil::AGuardiaCivil()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.2f;

    bEsPolicia = true;
    VidaMaxima = 120;
    Vida = 120;

    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    AIControllerClass = AAIController::StaticClass();

    if (UCharacterMovementComponent* CMC = GetCharacterMovement())
    {
        CMC->MaxWalkSpeed = 350.0f;
        CMC->RotationRate = FRotator(0.0, 360.0, 0.0);
        CMC->bUseRVOAvoidance = true;
    }
}

void AGuardiaCivil::BeginPlay()
{
    Super::BeginPlay();
    SpawnLocation = GetActorLocation();
    PatrolRadius = FMath::RandRange(2000.0f, 5000.0f);
    PickNewPatrolTarget();
}

void AGuardiaCivil::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    TickAlertScan(DeltaTime);
    TickPatrol(DeltaTime);
}

void AGuardiaCivil::SetAlertState(EGuardAlertState NewState)
{
    if (AlertState == NewState) return;
    AlertState = NewState;

    if (UCharacterMovementComponent* CMC = GetCharacterMovement())
    {
        switch (NewState)
        {
        case EGuardAlertState::Idle:
            CMC->MaxWalkSpeed = 350.0f;
            break;
        case EGuardAlertState::Suspicious:
            CMC->MaxWalkSpeed = 200.0f;
            break;
        case EGuardAlertState::Alert:
        case EGuardAlertState::Combat:
            CMC->MaxWalkSpeed = 500.0f;
            break;
        }
    }
}

void AGuardiaCivil::PickNewPatrolTarget()
{
    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!NavSys) return;

    FNavLocation Result;
    if (NavSys->GetRandomReachablePointInRadius(SpawnLocation, PatrolRadius, Result))
    {
        CurrentPatrolTarget = Result.Location;
    }
    else
    {
        CurrentPatrolTarget = SpawnLocation + FMath::VRand() * PatrolRadius;
        CurrentPatrolTarget.Z = SpawnLocation.Z;
    }
    bWaiting = false;
}

void AGuardiaCivil::TickPatrol(float DeltaTime)
{
    if (AlertState >= EGuardAlertState::Alert) return;

    if (bWaiting)
    {
        WaitTimer -= DeltaTime;
        if (WaitTimer <= 0.f) PickNewPatrolTarget();
        return;
    }

    if (AAIController* AIC = Cast<AAIController>(GetController()))
    {
        if (FVector::Dist2D(GetActorLocation(), CurrentPatrolTarget) < 150.f)
        {
            bWaiting = true;
            WaitTimer = FMath::RandRange(PatrolWaitMin, PatrolWaitMax);
            return;
        }
        AIC->MoveToLocation(CurrentPatrolTarget);
    }
}

void AGuardiaCivil::TickAlertScan(float DeltaTime)
{
    UWorld* W = GetWorld();
    if (!W) return;

    APawn* Player = W->GetFirstPlayerController() ? W->GetFirstPlayerController()->GetPawn() : nullptr;
    if (!Player) return;

    const float Dist = FVector::Dist(GetActorLocation(), Player->GetActorLocation());

    if (Dist < CombatRadius)
        SetAlertState(EGuardAlertState::Combat);
    else if (Dist < SuspiciousRadius)
        SetAlertState(EGuardAlertState::Suspicious);
    else
        SetAlertState(EGuardAlertState::Idle);
}
