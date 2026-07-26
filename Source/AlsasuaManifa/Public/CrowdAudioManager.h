#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CrowdAudioManager.generated.h"

class UAudioComponent;
class USoundBase;
class USoundAttenuation;

/**
 * Gestor de audio de multitud.
 * Sistema de capas de sonido ambiental que responde al estado de la multitud:
 *   - Murmullo base (siempre presente si hay NPC)
 *   - Cánticos de protesta (según tensión)
 *   - Gritos de pánico (cuando hay violencia)
 *   - Silbidos de policía (cuando hay alerta)
 *   - Atenuación por distancia del jugador
 */
UCLASS()
class ALSASUAMANIFA_API UCrowdAudioManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ── API pública ────────────────────────────────────────────────────────

    /** Volumen dinámico según NPCs activos (logarítmico). */
    UFUNCTION(BlueprintPure, Category = "Audio")
    float GetCrowdVolumeMultiplier(int32 ActiveProtesters) const;

    /** Actualizar el estado del audio de multitud. Llamar cada frame o cada 0.5s. */
    UFUNCTION(BlueprintCallable, Category = "Audio")
    void UpdateCrowdAudio(int32 AgentCount, float CrowdTension, float DistanceToPlayer);

    /** Disparar un sonido de evento (grito, impacto, explosión). */
    UFUNCTION(BlueprintCallable, Category = "Audio")
    void PlayCrowdEvent(FVector Location, float Intensity = 1.0f);

    /** Activar/desactivar sirenas de policía. */
    UFUNCTION(BlueprintCallable, Category = "Audio")
    void SetSirensActive(bool bActive);

    // ── Configuración ──────────────────────────────────────────────────────

    /** Sonido base de murmullo de multitud. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Layers")
    TObjectPtr<USoundBase> AmbientMurmurSound;

    /** Sonido de cánticos de protesta. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Layers")
    TObjectPtr<USoundBase> ChantSound;

    /** Sonido de gritos de pánico. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Layers")
    TObjectPtr<USoundBase> PanicScreamSound;

    /** Sonido de sirenas de policía. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Layers")
    TObjectPtr<USoundBase> SirenSound;

    /** Sonido de disparos lejanos. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Events")
    TObjectPtr<USoundBase> DistantGunshotSound;

    /** Sonido de impactos/manifestación. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Events")
    TObjectPtr<USoundBase> ImpactProtestSound;

    /** Atenuación personalizada para sonidos de multitud. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TObjectPtr<USoundAttenuation> CrowdAttenuation;

    /** Umbral de tensión para activar cánticos (0-1). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Thresholds", meta = (ClampMin = "0", ClampMax = "1"))
    float ChantTensionThreshold = 0.3f;

    /** Umbral de tensión para gritos de pánico (0-1). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Thresholds", meta = (ClampMin = "0", ClampMax = "1"))
    float PanicTensionThreshold = 0.7f;

    /** Distancia máxima de audibilidad (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio", meta = (ClampMin = "500"))
    float MaxAudibleDistance = 10000.f;

private:
    // ── Audio components (capas) ───────────────────────────────────────────
    UPROPERTY()
    TObjectPtr<UAudioComponent> MurmurLayer;

    UPROPERTY()
    TObjectPtr<UAudioComponent> ChantLayer;

    UPROPERTY()
    TObjectPtr<UAudioComponent> PanicLayer;

    UPROPERTY()
    TObjectPtr<UAudioComponent> SirenLayer;

    /** Volumen target para fade suave. */
    float MurmurTargetVol = 0.f;
    float ChantTargetVol = 0.f;
    float PanicTargetVol = 0.f;
    float SirenTargetVol = 0.f;

    /** Estado actual. */
    bool bSirensActive = false;
    int32 CurrentAgentCount = 0;
    float CurrentTension = 0.f;
};
