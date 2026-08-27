#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VehicleDamageComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnVehicleDestroyed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTirePopped, int32, TireIndex);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GF_VEHICULOS_API UVehicleDamageComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UVehicleDamageComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Status")
    float Health = 100.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AAA|Status")
    int32 IntactTires = 4;

    UPROPERTY(BlueprintAssignable)
    FOnVehicleDestroyed OnVehicleDestroyed;

    UPROPERTY(BlueprintAssignable)
    FOnTirePopped OnTirePopped;

    // ── Damage stages ──────────────────────────────────────────────────────
    /** Sparks from undercarriage at this % health. */
    UPROPERTY(EditAnywhere, Category="AAA|DamageStages")
    float SparksHealthPercent = 0.75f;

    /** Smoke VFX at this % health. */
    UPROPERTY(EditAnywhere, Category="AAA|DamageStages")
    float SmokeHealthPercent = 0.50f;

    /** Fire VFX at this % health. */
    UPROPERTY(EditAnywhere, Category="AAA|DamageStages")
    float FireHealthPercent = 0.25f;

    /** Engine stall (severe speed reduction) at this % health. */
    UPROPERTY(EditAnywhere, Category="AAA|DamageStages")
    float StallHealthPercent = 0.10f;

    /** Max speed multiplier when engine is stalling. */
    UPROPERTY(EditAnywhere, Category="AAA|DamageStages")
    float StallSpeedMultiplier = 0.15f;

    /** Damage per second from fire after fire threshold. */
    UPROPERTY(EditAnywhere, Category="AAA|DamageStages")
    float FireDamagePerSec = 2.f;

    UFUNCTION(BlueprintCallable, Category="AAA|Damage")
    void ApplyVehicleDamage(float Amount);

    UFUNCTION(BlueprintCallable, Category="AAA|Damage")
    void PopTire();

    UFUNCTION(BlueprintPure, Category="AAA|DamageStages")
    bool IsOnFire() const { return bOnFire; }

    UFUNCTION(BlueprintPure, Category="AAA|DamageStages")
    bool IsEngineStalled() const { return bEngineStalled; }

    /** Time before fire damage causes explosion (0 = no explosion). */
    UPROPERTY(EditAnywhere, Category="AAA|DamageStages")
    float FireExplosionTimer = 8.f;

    /** Broadcast when fire timer reaches zero (vehicle about to explode). */
    UPROPERTY(BlueprintAssignable)
    FOnVehicleDestroyed OnFuelLeakWarning;

private:
    void UpdateVehiclePerformance();
    void TickDamageStages(float DeltaTime);

    bool bSparksActive = false;
    bool bSmokeActive = false;
    bool bOnFire = false;
    bool bEngineStalled = false;

    float FireDamageTimer = 0.f;
    float FuelLeakTimer = 0.f;
};
