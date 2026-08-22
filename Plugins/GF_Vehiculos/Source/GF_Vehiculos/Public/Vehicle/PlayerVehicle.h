#pragma once

#include "CoreMinimal.h"
#include "Vehicle/BaseVehicle.h"
#include "PlayerVehicle.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVehicleEntered, APlayerVehicle*, Vehicle);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVehicleExited, APlayerVehicle*, Vehicle);

/**
 * Vehículo conduciible por el jugador. Conducción estilo GTA:
 *   - Gas / Freno / Freno de mano
 *   - Dirección con inercia
 *   - Cámara tercera persona con suavizado
 *   - Sonido de motor dinámico
 *   - Enter / Exit
 */
UCLASS()
class GF_VEHICULOS_API APlayerVehicle : public ABaseVehicle
{
    GENERATED_BODY()

public:
    APlayerVehicle();

    // ── Componentes ────────────────────────────────────────────────────────
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<class USpringArmComponent> SpringArm;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<class UCameraComponent> Camera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<class UVehicleDamageComponent> DamageComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<class UAudioComponent> EngineAudio;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<class UAudioComponent> HornAudio;

    // ── Sonidos ─────────────────────────────────────────────────────────────

    /** Sonido del claxon. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving|Audio")
    TObjectPtr<USoundBase> HornSound;

    // ── API ────────────────────────────────────────────────────────────────

    /** Subir al vehículo (possessa por el controller del pawn llamante). */
    UFUNCTION(BlueprintCallable, Category = "Alsasua|Vehicle")
    void EnterVehicle(APawn* Driver);

    /** Bajar del vehículo (devuelve el pawn al world). */
    UFUNCTION(BlueprintCallable, Category = "Alsasua|Vehicle")
    void ExitVehicle();

    /** ¿Tiene conductor? */
    UFUNCTION(BlueprintPure, Category = "Alsasua|Vehicle")
    bool IsOccupied() const { return DriverPawn != nullptr; }

    /** Delegados de enter/exit. */
    UPROPERTY(BlueprintAssignable, Category = "Alsasua|Vehicle")
    FOnVehicleEntered OnVehicleEntered;

    UPROPERTY(BlueprintAssignable, Category = "Alsasua|Vehicle")
    FOnVehicleExited OnVehicleExited;

    // ── Configuración de conducción ────────────────────────────────────────

    /** Fuerza de frenado (deceleración en cm/s²). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving", meta = (ClampMin = "100"))
    float BrakeForce = 2000.f;

    /** Fuerza del freno de mano (más agresiva, permite derrape). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving", meta = (ClampMin = "100"))
    float HandbrakeForce = 4000.f;

    /** Coeficiente de fricción al derrapar (0 = sin fricción, 1 = adherencia total). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving|Drift", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DriftFriction = 0.3f;

    /** Velocidad a la que el motor empieza a sonar alto (km/h). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving|Audio")
    float EngineHighRPMThreshold = 100.f;

    /** Pitch máximo del motor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving|Audio")
    float EngineMaxPitch = 2.5f;

protected:
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
    // ── Input handlers ─────────────────────────────────────────────────────
    void OnGas(float Value);
    // Aquí había un OnBrake(float) que no definía nadie y que no enlaza
    // ningún BindAxis: el freno lo hace OnGas con valor negativo, como dice
    // el comentario de SetupPlayerInputComponent. Los otros siete On* sí
    // están definidos; éste era el resto del eje separado que nunca se cableó.
    void OnSteer(float Value);
    void OnHandbrakePressed();
    void OnHandbrakeReleased();
    void OnHornPressed();
    void OnHornReleased();
    void OnToggleEngine();

    // ── Lógica interna ─────────────────────────────────────────────────────
    void UpdateDrivingPhysics(float DeltaTime);
    void UpdateCamera(float DeltaTime);
    void UpdateEngineAudio(float DeltaTime);
    void ApplyDriftPhysics(float DeltaTime);

    /** Pawn que conduce este vehículo. */
    UPROPERTY()
    TObjectPtr<APawn> DriverPawn;

    /** Valores de input acumulados. */
    float GasInput = 0.f;
    float BrakeInput = 0.f;
    float SteerInput = 0.f;
    bool bHandbrakeActive = false;

    /** Estado del motor. */
    bool bEngineRunning = true;

    /** Ángulo de dirección actual (suavizado). */
    float CurrentSteerAngle = 0.f;

    /** Velocidad angular de derrape. */
    float DriftAngularVelocity = 0.f;

    /** Handle del timer de input para exit. */
    FTimerHandle ExitTimerHandle;
};
