#include "Vehicle/VehicleDamageComponent.h"
#include "Vehicle/BaseVehicle.h"
#include "GameFramework/FloatingPawnMovement.h"

UVehicleDamageComponent::UVehicleDamageComponent() { PrimaryComponentTick.bCanEverTick = false; }

void UVehicleDamageComponent::ApplyVehicleDamage(float Amount)
{
    Health = FMath::Max(0.f, Health - Amount);
    if (Health <= 0.f) OnVehicleDestroyed.Broadcast();
}

void UVehicleDamageComponent::PopTire()
{
    if (IntactTires > 0)
    {
        IntactTires--;
        OnTirePopped.Broadcast(IntactTires);
        UpdateVehiclePerformance();
    }
}

void UVehicleDamageComponent::UpdateVehiclePerformance()
{
    if (ABaseVehicle* Car = Cast<ABaseVehicle>(GetOwner()))
    {
        // Reducir velocidad maxima por cada rueda pinchada
        float Multiplier = (float)IntactTires / 4.0f;
        Car->MovementComponent->MaxSpeed = Car->MaxSpeed * Multiplier;

        if (IntactTires == 0) Car->ToggleEngine(false);
    }
}
