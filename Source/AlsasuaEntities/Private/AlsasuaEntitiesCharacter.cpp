#include "AlsasuaEntitiesCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"

AAlsasuaEntitiesCharacter::AAlsasuaEntitiesCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
    CameraComponent->SetupAttachment(GetCapsuleComponent());
}

void AAlsasuaEntitiesCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
}
