#include "Vehicle/BaseVehicle.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/FloatingPawnMovement.h"

ABaseVehicle::ABaseVehicle()
{
    PrimaryActorTick.bCanEverTick = true;

    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    RootComponent = CollisionBox;

    VehicleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VehicleMesh"));
    VehicleMesh->SetupAttachment(RootComponent);

    MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
    MovementComponent->MaxSpeed = MaxSpeed;
}

void ABaseVehicle::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    CurrentSpeedKmh = GetVelocity().Size() * 0.036f; // Conversion simple a km/h
}

void ABaseVehicle::Drive(float ForwardValue, float RightValue)
{
    if (!bEngineActive) return;

    AddMovementInput(GetActorForwardVector(), ForwardValue);

    if (GetVelocity().Size() > 100.f)
    {
        float TurnDir = ForwardValue >= 0 ? 1.f : -1.f;
        AddActorLocalRotation(FRotator(0.f, RightValue * TurnSpeed * GetWorld()->GetDeltaSeconds() * TurnDir, 0.f));
    }
}

void ABaseVehicle::ToggleEngine(bool bOn)
{
    bEngineActive = bOn;
}
