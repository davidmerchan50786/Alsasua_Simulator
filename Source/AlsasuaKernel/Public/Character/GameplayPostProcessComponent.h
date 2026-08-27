#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Camera/CameraShakeBase.h"
#include "GameplayPostProcessComponent.generated.h"

class UPostProcessComponent;
class UMaterialInstanceDynamic;
class UCurveFloat;

/**
 * Componente de post-procesado dinámico que responde al gameplay.
 * Se añade al jugador y controla:
 *   - Viñeta de bajo vida (rojo pulsante)
 *   - Flash de daño (blanco rápido)
 *   - Líneas de velocidad
 *   - Polvo de multitud (screen dirt)
 *   - Screen shake sutil
 *   - Blur de concentración (ADS)
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALSASUAKERNEL_API UGameplayPostProcessComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGameplayPostProcessComponent();

    // ── API pública ────────────────────────────────────────────────────────

    /** Llamar al recibir daño para flash de daño. */
    UFUNCTION(BlueprintCallable, Category = "Alsasua|PostProcess")
    void TriggerDamageFlash(float Intensity = 1.0f);

    /** Activar/desactivar blur de ADS (apuntado). */
    UFUNCTION(BlueprintCallable, Category = "Alsasua|PostProcess")
    void SetADSBloom(bool bActive);

    /** Activar efecto de polvo de multitud. */
    UFUNCTION(BlueprintCallable, Category = "Alsasua|PostProcess")
    void TriggerCrowdDust(float Intensity = 1.0f);

    /** Activar líneas de velocidad. */
    UFUNCTION(BlueprintCallable, Category = "Alsasua|PostProcess")
    void SetSpeedLines(bool bActive, float Intensity = 1.0f);

    /** Activar visión de drogas (ya existía como "Modo Droga"). */
    UFUNCTION(BlueprintCallable, Category = "Alsasua|PostProcess")
    void SetDrugVision(bool bActive, float Intensity = 1.0f);

    /** Explosión cercana: slow-mo + screen shake + flash. */
    UFUNCTION(BlueprintCallable, Category = "Alsasua|PostProcess")
    void TriggerExplosionReaction(float Distance);

    /** Tortura: pulso rojo periódico que escala con el método. */
    UFUNCTION(BlueprintCallable, Category = "Alsasua|PostProcess")
    void TriggerTorturePulse(float Intensity);

    /** Electrodo: flash blanco rápido. */
    UFUNCTION(BlueprintCallable, Category = "Alsasua|PostProcess")
    void TriggerElectrodeFlash();

    /** Ahogo: visión borrosa progresiva. */
    UFUNCTION(BlueprintCallable, Category = "Alsasua|PostProcess")
    void SetDrowningVision(bool bActive, float Intensity);

    /** Golpe: flash rojo + screen shake corto. */
    UFUNCTION(BlueprintCallable, Category = "Alsasua|PostProcess")
    void TriggerBeatingImpact();

    /** Sueño: visión borrosa + doble imagen. */
    UFUNCTION(BlueprintCallable, Category = "Alsasua|PostProcess")
    void SetSleepDeprivationVision(bool bActive, float Intensity);

    /** Nivel de paranoia global: desaturación + aberración + viñeta oscura. */
    UFUNCTION(BlueprintCallable, Category = "Alsasua|PostProcess")
    void SetParanoiaLevel(float Paranoia01);

    // ── Configuración ──────────────────────────────────────────────────────

    /** Intensidad máxima de la viñeta de bajo vida. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PostProcess|Health", meta = (ClampMin = "0", ClampMax = "1"))
    float MaxHealthVignetteIntensity = 0.7f;

    /** Umbral de vida para empezar a mostrar viñeta (0-1, fracción de vida max). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PostProcess|Health", meta = (ClampMin = "0", ClampMax = "1"))
    float HealthVignetteThreshold = 0.4f;

    /** Duración del flash de daño (segundos). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PostProcess|Damage", meta = (ClampMin = "0.05"))
    float DamageFlashDuration = 0.15f;

    /** Camera shake class for explosions (set in BP or C++ constructor). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PostProcess|Shake")
    TSubclassOf<UCameraShakeBase> ExplosionShakeClass;

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    UPROPERTY()
    TObjectPtr<UPostProcessComponent> PostProcessComponent;

    // ── Health Vignette ────────────────────────────────────────────────────
    void UpdateHealthVignette(float DeltaTime);

    // ── Damage Flash ───────────────────────────────────────────────────────
    void UpdateDamageFlash(float DeltaTime);
    float DamageFlashTimer = 0.f;
    float DamageFlashIntensity = 0.f;

    // ── Speed Lines ────────────────────────────────────────────────────────
    void UpdateSpeedLines(float DeltaTime);
    bool bSpeedLinesActive = false;
    float SpeedLinesTargetIntensity = 0.f;
    float SpeedLinesCurrentIntensity = 0.f;

    // ── Crowd Dust ─────────────────────────────────────────────────────────
    void UpdateCrowdDust(float DeltaTime);
    float CrowdDustTimer = 0.f;
    float CrowdDustIntensity = 0.f;

    // ── ADS Bloom ──────────────────────────────────────────────────────────
    bool bADSBloomActive = false;

    // ── Drug Vision ────────────────────────────────────────────────────────
    bool bDrugVisionActive = false;
    float DrugVisionIntensity = 0.f;
    float DrugVisionTargetIntensity = 0.f;

    // ── Paranoia Vision ────────────────────────────────────────────────────
    void UpdateParanoiaVision(float DeltaTime);
    float ParanoiaLevel01 = 0.f;   // 0-1 normalized
    float ParanoiaDesaturation = 0.f;
    float ParanoiaChromatic = 0.f;
    float ParanoiaVignetteAmount = 0.f;
    float ParanoiaTearTimer = 0.f;

    // ── Weight actual (blend total) ────────────────────────────────────────
    float CurrentWeight = 0.f;
    float TargetWeight = 0.f;

    /** Obtener la vida actual como fracción 0-1. */
    float GetHealthFraction() const;
};
