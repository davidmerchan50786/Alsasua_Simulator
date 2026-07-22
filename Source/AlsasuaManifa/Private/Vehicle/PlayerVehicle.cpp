#include "Vehicle/PlayerVehicle.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Vehicle/VehicleDamageComponent.h"

APlayerVehicle::APlayerVehicle()
{
    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(RootComponent);
    SpringArm->TargetArmLength = 600.f;
    SpringArm->bUsePawnControlRotation = true;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm);

    DamageComponent = CreateDefaultSubobject<UVehicleDamageComponent>(TEXT("DamageComponent"));

    EngineAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("EngineAudio"));
    EngineAudio->SetupAttachment(RootComponent);
}

void APlayerVehicle::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    PlayerInputComponent->BindAxis("MoveForward", this, &APlayerVehicle::MoveForward);
    PlayerInputComponent->BindAxis("MoveRight", this, &APlayerVehicle::MoveRight);
}

void APlayerVehicle::MoveForward(float Val) { Drive(Val, 0.f); }
void APlayerVehicle::MoveRight(float Val) { Drive(0.f, Val); }

void APlayerVehicle::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Simulación de pitch de motor basado en velocidad
    if (EngineAudio)
    {
        float NewPitch = FMath::Clamp(1.0f + (CurrentSpeedKmh / 150.f), 1.0f, 2.5f);
        EngineAudio->SetPitchMultiplier(NewPitch);
    }
}
