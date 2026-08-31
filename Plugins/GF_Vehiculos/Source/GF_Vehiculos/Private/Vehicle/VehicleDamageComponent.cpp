#include "Vehicle/VehicleDamageComponent.h"
#include "Vehicle/BaseVehicle.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

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
UNiagaraComponent* UVehicleDamageComponent::EnsureStageVFX(TObjectPtr<UNiagaraSystem>& Cache,
                                                           UNiagaraComponent*& Holder,
                                                           const TCHAR* Ruta, FVector Offset)
{
    if (Holder) return Holder;
    if (!Cache)
    {
        Cache = LoadObject<UNiagaraSystem>(nullptr, Ruta);
        if (!Cache)
        {
            UE_LOG(LogTemp, Warning, TEXT("VehicleDamage: no está %s (Tools/create_niagara_vfx.py)."), Ruta);
            return nullptr;
        }
    }
    const ABaseVehicle* Car = Cast<ABaseVehicle>(GetOwner());
    if (!Car || !Car->VehicleMesh) return nullptr;

    Holder = UNiagaraFunctionLibrary::SpawnSystemAttached(
        Cache, Car->VehicleMesh, NAME_None, Offset, FRotator::ZeroRotator,
        EAttachLocation::KeepRelativeOffset, true);
    return Holder;
}

void UVehicleDamageComponent::TickDamageStages(float DeltaTime)
{
    const float HealthPct = Health / 100.f;

    // Sparks at 75% — first warning.
    if (HealthPct <= SparksHealthPercent && !bSparksActive)
    {
        bSparksActive = true;
        EnsureStageVFX(SparkSystem, SparksVFX, TEXT("/Game/VFX/NS_SparkCar.NS_SparkCar"),
                       FVector(0, 0, -40.f)); // bajos del vehículo
    }

    // Smoke at 50% — you're in trouble.
    if (HealthPct <= SmokeHealthPercent && !bSmokeActive)
    {
        bSmokeActive = true;
        EnsureStageVFX(SmokeSystem, SmokeVFX, TEXT("/Game/VFX/NS_Humo.NS_Humo"),
                       FVector(0, 0, 60.f)); // capó
    }

    // Fire at 25% — critical damage, ongoing fire damage.
    if (HealthPct <= FireHealthPercent && !bOnFire)
    {
        bOnFire = true;
        EnsureStageVFX(FireSystem, FireVFX, TEXT("/Game/VFX/NS_Fuego.NS_Fuego"),
                       FVector(0, 0, 50.f)); // motor en llamas
        FireDamageTimer = 0.f;
        FuelLeakTimer = 0.f;
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
