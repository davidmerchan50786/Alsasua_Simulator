#include "NPCCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Engine/World.h"

ANPCCharacter::ANPCCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.1f;
}

void ANPCCharacter::BeginPlay()
{
    Super::BeginPlay();

    // Auto-asignar AIController si no tiene uno.
    if (!GetController())
    {
        SpawnDefaultController();
    }
}
