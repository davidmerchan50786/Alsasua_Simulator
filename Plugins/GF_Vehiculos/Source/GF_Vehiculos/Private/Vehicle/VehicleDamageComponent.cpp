#include "Vehicle/VehicleDamageComponent.h"
#include "Vehicle/BaseVehicle.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Kismet/GameplayStatics.h"

UVehicleDamageComponent::UVehicleDamageComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UVehicleDamageComponent::ApplyVehicleDamage(float Amount)
{
    Health = FMath::Max(0.f, Health - Amount);
    TickDamageStages(0.f);
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
        float Multiplier = (float)IntactTires / 4.0f;

        // Engine stall override at critical health.
        if (bEngineStalled)
            Multiplier = FMath::Min(Multiplier, StallSpeedMultiplier);

        Car->MovementComponent->MaxSpeed = Car->MaxSpeed * Multiplier;

        if (IntactTires == 0 || bEngineStalled)
            Car->ToggleEngine(false);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Damage stages — progressive visual/mechanical degradation
// ─────────────────────────────────────────────────────────────────────────────
void UVehicleDamageComponent::TickDamageStages(float DeltaTime)
{
    const float HealthPct = Health / 100.f;

    // Sparks at 75% — first warning.
    if (HealthPct <= SparksHealthPercent && !bSparksActive)
    {
        bSparksActive = true;
        // TODO: attach Niagara spark system to undercarriage when VFX assets created.
    }

    // Smoke at 50% — you're in trouble.
    if (HealthPct <= SmokeHealthPercent && !bSmokeActive)
    {
        bSmokeActive = true;
        // TODO: attach Niagara smoke system when VFX assets created.
    }

    // Fire at 25% — critical damage, ongoing fire damage.
    if (HealthPct <= FireHealthPercent && !bOnFire)
    {
        bOnFire = true;
        FireDamageTimer = 0.f;
        FuelLeakTimer = 0.f;
        // TODO: attach Niagara fire system when VFX assets created.
    }

    // Engine stall at 10% — barely functional.
    if (HealthPct <= StallHealthPercent && !bEngineStalled)
    {
        bEngineStalled = true;
        UpdateVehiclePerformance();
    }

    // Ongoing fire damage.
    if (bOnFire && DeltaTime > 0.f)
    {
        FireDamageTimer += DeltaTime;
        if (FireDamageTimer >= 1.f)
        {
            FireDamageTimer -= 1.f;
            ApplyVehicleDamage(FireDamagePerSec);
        }

        // Fuel leak warning — vehicle about to explode.
        if (FireExplosionTimer > 0.f)
        {
            FuelLeakTimer += DeltaTime;
            if (FuelLeakTimer >= FireExplosionTimer)
            {
                OnFuelLeakWarning.Broadcast();
                FuelLeakTimer = 0.f; // Reset for repeated warnings.
            }
        }
    }
}
