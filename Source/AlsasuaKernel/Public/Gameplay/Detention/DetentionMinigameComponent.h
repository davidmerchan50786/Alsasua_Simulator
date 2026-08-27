#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaCore.h"
#include "DetentionMinigameComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDetentionStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDetentionEnded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQTEWindow, float, WindowDuration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQTEResult, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDetentionResult, bool, bEscaped);

/** Tipo de tortura aplicada durante el interrogatorio. */
UENUM(BlueprintType)
enum class EInterrogationMethod : uint8
{
	None,
	Electrodes,     // Choques eléctricos — daño directo + stress spike + flash pantalla
	WaterBoarding,  // Bolsa/bañera — drenaje stamina + ahogo + visión borrosa
	SleepDeprivation, // Privación del sueño — stress lento + visión doble + alucinaciones
	Beating,        // Golpes — daño directo + flash rojo + stun periódico
	Threats         // Amenazas — solo psicológico, sube stress + afecta apoyo
};

UENUM(BlueprintType)
enum class EDetentionState : uint8 { Idle, Arrested, Interrogating, Resisting, Escaped, Surrendered };

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALSASUAKERNEL_API UDetentionMinigameComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UDetentionMinigameComponent();

    // Start/Stop
    UFUNCTION(BlueprintCallable, Category="AAA|Detention")
    void StartMinigame(float InDuration = 30.f, float DifficultyMultiplier = 1.f);

    UFUNCTION(BlueprintCallable, Category="AAA|Detention")
    void StopMinigame(bool bForceFail = false);

    /** Aplica un método de tortura específico. Cambia el comportamiento del tick. */
    UFUNCTION(BlueprintCallable, Category="AAA|Detention")
    void ApplyTortureMethod(EInterrogationMethod Method);

    UFUNCTION(BlueprintCallable, Category="AAA|Detention")
    void RegisterInputPress();

    UFUNCTION(BlueprintCallable, Category="AAA|Detention")
    void UseInventoryItemDuringMinigame(FName ItemID);

    // Delegates
    UPROPERTY(BlueprintAssignable, Category="AAA|Detention") FOnDetentionStarted OnDetentionStarted;
    UPROPERTY(BlueprintAssignable, Category="AAA|Detention") FOnDetentionEnded OnDetentionEnded;
    UPROPERTY(BlueprintAssignable, Category="AAA|Detention") FOnQTEWindow OnQTEWindow;
    UPROPERTY(BlueprintAssignable, Category="AAA|Detention") FOnQTEResult OnQTEResult;
    UPROPERTY(BlueprintAssignable, Category="AAA|Detention") FOnDetentionResult OnDetentionResult;

    UPROPERTY(BlueprintReadOnly, Category="AAA|Detention") EDetentionState CurrentState = EDetentionState::Idle;

    /** Método de tortura activo. None = interrogatorio genérico. */
    UPROPERTY(BlueprintReadOnly, Category="AAA|Detention") EInterrogationMethod ActiveMethod = EInterrogationMethod::None;

    UPROPERTY(EditAnywhere, Category="AAA|Detention") float Duration = 30.f;
    UPROPERTY(EditAnywhere, Category="AAA|Detention") float MashPowerPerPress = 1.f;
    UPROPERTY(EditAnywhere, Category="AAA|Detention") FVector2D QTEIntervalRange = FVector2D(3.f, 7.f);
    UPROPERTY(EditAnywhere, Category="AAA|Detention") float StressIncreaseRate = 1.f;
    UPROPERTY(EditAnywhere, Category="AAA|Detention") float SuccessThreshold = 100.f;

    UPROPERTY(BlueprintReadOnly, Category="AAA|Detention") float CurrentResistance = 0.f;
    UPROPERTY(BlueprintReadOnly, Category="AAA|Detention") float StressLevel = 0.f;

    // Per-method tuning
    UPROPERTY(EditAnywhere, Category="AAA|Detention|Electrodes") float ElectrodeDamagePerSec = 8.f;
    UPROPERTY(EditAnywhere, Category="AAA|Detention|Electrodes") float ElectrodeStressSpike = 15.f;
    UPROPERTY(EditAnywhere, Category="AAA|Detention|Water") float WaterStaminaDrain = 12.f;
    UPROPERTY(EditAnywhere, Category="AAA|Detention|Sleep") float SleepStressPerSec = 2.f;
    UPROPERTY(EditAnywhere, Category="AAA|Detention|Beating") int32 BeatingDamage = 5;
    UPROPERTY(EditAnywhere, Category="AAA|Detention|Beating") float BeatingStunInterval = 4.f;
    UPROPERTY(EditAnywhere, Category="AAA|Detention|Threats") float ThreatsStressPerSec = 3.f;

    // ── Surrender/Escape consequences ──────────────────────────────────────
    /** % of player cash lost on surrender. */
    UPROPERTY(EditAnywhere, Category="AAA|Detention|Consequences")
    float SurrenderCashLossPercent = 0.25f;

    /** Items lost on surrender (random from inventory). */
    UPROPERTY(EditAnywhere, Category="AAA|Detention|Consequences")
    int32 SurrenderItemsLost = 2;

    /** Apoyo lost on surrender. */
    UPROPERTY(EditAnywhere, Category="AAA|Detention|Consequences")
    float SurrenderApoyoLoss = 15.f;

    /** Paranoia gained on surrender. */
    UPROPERTY(EditAnywhere, Category="AAA|Detention|Consequences")
    float SurrenderParanoiaGain = 10.f;

    /** Wanted reset on surrender (sets to this value). */
    UPROPERTY(EditAnywhere, Category="AAA|Detention|Consequences")
    float SurrenderWantedReset = 0.f;

    /** Time guard stays near player after escape (seconds). */
    UPROPERTY(EditAnywhere, Category="AAA|Detention|Consequences")
    float EscapeGuardFollowTime = 25.f;

    /** Apoyo lost on escape (they know you resisted). */
    UPROPERTY(EditAnywhere, Category="AAA|Detention|Consequences")
    float EscapeApoyoLoss = 5.f;

    // ── Method escalation ──────────────────────────────────────────────────
    /** Time before guard escalates to next torture method (seconds). */
    UPROPERTY(EditAnywhere, Category="AAA|Detention|Escalation")
    float MethodEscalationInterval = 12.f;

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

private:
    float Elapsed = 0.f;
    float NextQTETime = 0.f;
    float QTEWindowDuration = 1.5f;
    float Difficulty = 1.f;
    bool bQTEActive = false;

    // Torture-specific timers
    float BeatingTimer = 0.f;
    bool bStunned = false;
    float StunTimer = 0.f;
    float SleepHallucinationTimer = 0.f;
    float ElectrodeFlashTimer = 0.f;
    float WaterDmgTimer = 0.f;

    void StartQTEWindow();
    void ResolveQTE(bool bSuccess);
    void ApplyStress(float Delta);
    void TickElectrodes(float DeltaTime);
    void TickWaterBoarding(float DeltaTime);
    void TickSleepDeprivation(float DeltaTime);
    void TickBeating(float DeltaTime);
    void TickThreats(float DeltaTime);
    void ApplyDamageToPlayer(int32 Amount);
    void FinishMinigame(bool bEscaped);
    void ApplySurrenderConsequences();
    void ApplyEscapeConsequences();
    void EscalateTortureMethod();

    float MethodEscalationTimer = 0.f;
};
