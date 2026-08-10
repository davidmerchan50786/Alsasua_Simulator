#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "TimeOfDayManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHourChanged, int32, CurrentHour);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNightStarted, bool, bIsNight);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPeriodChanged, FName, NewPeriod);

/**
 * Períodos del día que afectan al gameplay.
 *   Morning  (6-10): guardias patrullan, comercios abren
 *   Day      (10-18): máximo tráfico y peatones
 *   Evening  (18-22): guardias refuerzan, calles vacías
 *   Night    (22-6):  toque de queda, menos guardias pero más agresivos
 */
UENUM(BlueprintType)
enum class ETimePeriod : uint8
{
    Morning,
    Day,
    Evening,
    Night
};

/**
 * Gestor de ciclo día/noche con gameplay afectado.
 * Auto-tickea el tiempo, controla iluminación solar,
 * y notifica a otros sistemas de cambios de período.
 */
UCLASS()
class ALSASUAMANIFA_API UTimeOfDayManager : public UWorldSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // Reloj único del mundo: el sol, la niebla, el alumbrado, las ventanas y el
    // post-proceso leen CurrentTime. Sin este Tick se quedaba clavado a las 8:00.
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UTimeOfDayManager, STATGROUP_Tickables); }
    virtual bool IsTickable() const override { return !IsTemplate(); }

    // ── Tiempo ─────────────────────────────────────────────────────────────

    /** Hora actual (0.0 - 24.0). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
    float CurrentTime = 8.0f;

    /** Horas de juego por segundo real (0.01667 ≈ un día cada 24 min). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time", meta = (ClampMin = "0.0"))
    float TimeSpeed = 0.01667f;

    /** ¿El tiempo avanza automáticamente? */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
    bool bAutoAdvance = true;

    // ── API pública ────────────────────────────────────────────────────────

    UFUNCTION(BlueprintPure, Category = "Time")
    bool IsNight() const { return CurrentTime < 6.0f || CurrentTime > 21.0f; }

    UFUNCTION(BlueprintPure, Category = "Time")
    FString GetFormattedTime() const;

    UFUNCTION(BlueprintPure, Category = "Time")
    ETimePeriod GetCurrentPeriod() const;

    UFUNCTION(BlueprintPure, Category = "Time")
    float GetSunAngle() const;

    /** Número de guardias activos según el período (1-5 stars wanted). */
    UFUNCTION(BlueprintPure, Category = "Time|Gameplay")
    int32 GetGuardDensity() const;

    /** ¿Hay toque de queda? (Night, 22:00 - 06:00). */
    UFUNCTION(BlueprintPure, Category = "Time|Gameplay")
    bool IsCurfewActive() const;

    /** Multiplicador de tráfico según el período. */
    UFUNCTION(BlueprintPure, Category = "Time|Gameplay")
    float GetTrafficMultiplier() const;

    /** Multiplicador de peatones según el período. */
    UFUNCTION(BlueprintPure, Category = "Time|Gameplay")
    float GetPedestrianMultiplier() const;

    /** Forzar hora (para misiones o cutscenes). */
    UFUNCTION(BlueprintCallable, Category = "Time")
    void SetTime(float NewTime);

    // ── Delegados ──────────────────────────────────────────────────────────

    UPROPERTY(BlueprintAssignable, Category = "Time")
    FOnHourChanged OnHourChanged;

    UPROPERTY(BlueprintAssignable, Category = "Time")
    FOnNightStarted OnNightStarted;

    UPROPERTY(BlueprintAssignable, Category = "Time")
    FOnPeriodChanged OnPeriodChanged;

    // ── Configuración de gameplay ──────────────────────────────────────────

    /** Guardias base durante el día. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time|Gameplay")
    int32 DayGuardCount = 3;

    /** Guardias adicionales por noche. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time|Gameplay")
    int32 NightGuardBonus = 2;

    /** Guardias extra en toque de queda. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time|Gameplay")
    int32 CurfewGuardBonus = 3;

    /** Tráfico máximo (día punta). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time|Gameplay")
    float MaxTrafficMultiplier = 1.5f;

    /** Tráfico mínimo (madrugada). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time|Gameplay")
    float MinTrafficMultiplier = 0.3f;

private:
    int32 LastHour = -1;
    bool bWasNight = false;
    ETimePeriod LastPeriod = ETimePeriod::Day;

    void UpdateTime(float DeltaTime);
    void BroadcastTransitions();
};
