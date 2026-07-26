#include "GuardiaCivil.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Engine/World.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"

AGuardiaCivil::AGuardiaCivil()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.2f;

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

    // Asignar patrol radius y patrol target inicial.
    PatrolRadius = FMath::RandRange(2000.0f, 5000.0f);
}
