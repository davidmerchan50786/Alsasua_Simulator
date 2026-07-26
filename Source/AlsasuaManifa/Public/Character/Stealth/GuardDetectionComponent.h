#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GuardDetectionComponent.generated.h"

/**
 * Estados de alerta del guardia.
 *   Idle → Suspicious → Alert → Combat
 *   Cualquier estado puede deescalar gradualmente.
 */
UENUM(BlueprintType)
enum class EGuardAlertState : uint8
{
    Idle,        // Patrullando normal, no sospecha nada.
    Suspicious,  // Vio algo raro, investigando.
    Alert,       // Sabe que hay un intruso, búsqueda activa.
    Combat       // Ha visto al intruso, atacando.
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnGuardAlertStateChanged, AActor*, Guard, EGuardAlertState, NewState, EGuardAlertState, OldState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGuardDetectedPlayer, AActor*, Guard, FVector, DetectionLocation);

/**
 * Componente de detección para guardias y policía.
 * Cone de visión + detección por sonido + máquina de estados de alerta.
 *
 * Agregar a cualquier ACharacter que actúe como guardia.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALSASUAMANIFA_API UGuardDetectionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGuardDetectionComponent();

    // ── API pública ────────────────────────────────────────────────────────

    /**
     * Notificar al guardia de un sonido en una posición.
     * @param NoiseLocation  Origen del sonido.
     * @param Loudness       Intensidad del sonido (0-1).
     */
    UFUNCTION(BlueprintCallable, Category = "Alsasua|Guard")
    void ReportNoise(FVector NoiseLocation, float Loudness = 1.0f);

    /**
     * Forzar un estado de alerta (ej: refuerzos por radio).
     */
    UFUNCTION(BlueprintCallable, Category = "Alsasua|Guard")
    void ForceAlertState(EGuardAlertState NewState);

    /**
     * Resetear al estado Idle (ej: tras perder al sospechoso).
     */
    UFUNCTION(BlueprintCallable, Category = "Alsasua|Guard")
    void ResetToIdle();

    /** Estado actual. */
    UPROPERTY(BlueprintReadOnly, Category = "Alsasua|Guard")
    EGuardAlertState CurrentState = EGuardAlertState::Idle;

    /** Última posición conocida del sospechoso. */
    UPROPERTY(BlueprintReadOnly, Category = "Alsasua|Guard")
    FVector LastKnownPlayerLocation = FVector::ZeroVector;

    /** ¿Está activamente persiguiendo al jugador? */
    UPROPERTY(BlueprintReadOnly, Category = "Alsasua|Guard")
    bool bIsChasing = false;

    // ── Delegados ──────────────────────────────────────────────────────────
    UPROPERTY(BlueprintAssignable, Category = "Alsasua|Guard")
    FOnGuardAlertStateChanged OnAlertStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "Alsasua|Guard")
    FOnGuardDetectedPlayer OnDetectedPlayer;

    // ── Configuración ──────────────────────────────────────────────────────

    /** Radio de visión del guardia (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision", meta = (ClampMin = "100"))
    float VisionRange = 2000.f;

    /** Ángulo del cono de visión (grados, total). 120 = 60 a cada lado. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision", meta = (ClampMin = "10", ClampMax = "360"))
    float VisionConeAngle = 120.f;

    /** Radio de detección por sonido (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hearing", meta = (ClampMin = "100"))
    float HearingRange = 3000.f;

    /** Tiempo de investigación antes de volver a Idle (segundos). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alert", meta = (ClampMin = "1"))
    float SuspiciousDuration = 5.f;

    /** Tiempo en estado Alert antes de deescalar a Suspicious (segundos). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alert", meta = (ClampMin = "1"))
    float AlertDuration = 15.f;

    /** Tiempo máximo de persecución en Combat antes de deescalar (segundos). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alert", meta = (ClampMin = "5"))
    float CombatDuration = 30.f;

    /** Altura de los ojos del guardia para traces (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision", meta = (ClampMin = "10"))
    float EyeHeight = 60.f;

    /** Altura del centro del target para traces (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision", meta = (ClampMin = "10"))
    float TargetHeight = 40.f;

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    // ── Detección visual ───────────────────────────────────────────────────
    void TickVisionCheck(float DeltaTime);
    bool CanSeeTarget(const AActor* Target) const;
    bool IsInVisionCone(const AActor* Target) const;
    bool HasLineOfSight(const AActor* Target) const;

    // ── Detección por sonido ───────────────────────────────────────────────
    void TickHearingCheck(float DeltaTime);

    // ── Máquina de estados ──────────────────────────────────────────────────
    void TransitionToState(EGuardAlertState NewState);
    void TickStateTimer(float DeltaTime);

    // ── Utilidades ─────────────────────────────────────────────────────────
    AActor* FindPlayerPawn() const;

    // ── State timers ───────────────────────────────────────────────────────
    float StateTimer = 0.f;
    float HearingCooldown = 0.f;

    /** Referencia cacheada al jugador. */
    UPROPERTY()
    TObjectPtr<AActor> CachedPlayerTarget;

    /** Última posición desde la que se vio al jugador. */
    FVector LastSeenLocation = FVector::ZeroVector;
};
