// ParanoiaVisualSubsystem.h (capa GAMEPLAY)
// Cuando la paranoia global sube, el mundo visual cambia:
//   - NPCs parecen guardias civiles (material swap)
//   - Coches parecen patrol (material swap + sirena + sirena light)
//   - Post-process: desaturación, aberración cromática, viñeta, film grain
//   - Alucinaciones: guardias fantasma que se acercan al jugador
//   - Incertidumbre: algunas alucinaciones son REALES → matarlas tiene consecuencias
//   - Luces parpadean, sombras se estiran
// No puedes distinguir amenaza real de paranoia.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "ParanoiaVisualSubsystem.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class UAudioComponent;
class USoundBase;
class UNiagaraSystem;
class UNiagaraComponent;
class UGameplayPostProcessComponent;

// Datos de una víctima civil muerta (para alucinaciones).
USTRUCT(BlueprintType)
struct FVictimaRecord
{
	GENERATED_BODY()

	UPROPERTY() FVector Ubicacion = FVector::ZeroVector;
	UPROPERTY() float TiempoMuerte = 0.f;
	UPROPERTY() bool bEsPolicia = false;
};

// Estado de una alucinación activa.
USTRUCT()
struct FAlucinacionState
{
	GENERATED_BODY()

	UPROPERTY() TObjectPtr<AActor> Actor = nullptr;
	bool bEsReal = false;            // Si true, es un NPC real disfrazado de fantasma
	float FlickerTimer = 0.f;       // Timer para efecto de parpadeo
	float DamageTimer = 0.f;        // Cooldown de daño al jugador
	float FadeAlpha = 0.f;          // 0-1 para fade in/out
	bool bFadingIn = true;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnParanoiaVisualCambia, float, NivelParanoia);

UCLASS()
class ALSASUAGAMEPLAY_API UParanoiaVisualSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	// ── Umbrales ────────────────────────────────────────────────────────────
	/** Paranoia mínima para que NPCs empiecen a parecer guardias (0-100). */
	UPROPERTY(EditAnywhere, Category="Paranoia|Umbrales", meta=(ClampMin="0",ClampMax="100"))
	float GuardiaThreshold = 30.f;

	/** Paranoia para que TODOS los NPCs parezcan guardias (0-100). */
	UPROPERTY(EditAnywhere, Category="Paranoia|Umbrales", meta=(ClampMin="0",ClampMax="100"))
	float TodosGuardiaThreshold = 60.f;

	/** Paranoia para que coches parezcan patrol (0-100). */
	UPROPERTY(EditAnywhere, Category="Paranoia|Umbrales", meta=(ClampMin="0",ClampMax="100"))
	float PatrolThreshold = 40.f;

	/** Paranoia para spawnear alucinaciones (0-100). */
	UPROPERTY(EditAnywhere, Category="Paranoia|Umbrales", meta=(ClampMin="0",ClampMax="100"))
	float AlucinacionThreshold = 70.f;

	/** Paranoia máxima para efectos extremos (pantalla falla, luces fallan). */
	UPROPERTY(EditAnywhere, Category="Paranoia|Umbrales", meta=(ClampMin="0",ClampMax="100"))
	float ExtremoThreshold = 85.f;

	/** % de alucinaciones que son REALES (no puedes distinguir). */
	UPROPERTY(EditAnywhere, Category="Paranoia|Alucinaciones", meta=(ClampMin="0",ClampMax="100"))
	int32 PorcentajeReal = 15;

	/** Velocidad de movimiento de alucinaciones (cm/s). */
	UPROPERTY(EditAnywhere, Category="Paranoia|Alucinaciones")
	float VelocidadAlucinacion = 350.f;

	/** Daño por segundo cuando un fantasma está cerca del jugador. */
	UPROPERTY(EditAnywhere, Category="Paranoia|Alucinaciones")
	float DanoAlucinacion = 3.f;

	/** Distancia de daño de alucinaciones (cm). */
	UPROPERTY(EditAnywhere, Category="Paranoia|Alucinaciones")
	float DistanciaDanoAlucinacion = 250.f;

	/** Intervalo entre spawns de alucinaciones (segundos, escala con paranoia). */
	UPROPERTY(EditAnywhere, Category="Paranoia|Alucinaciones")
	float IntervaloSpawnMin = 2.f;

	/** Intervalo máximo entre spawns (cuando paranoia baja). */
	UPROPERTY(EditAnywhere, Category="Paranoia|Alucinaciones")
	float IntervaloSpawnMax = 8.f;

	// ── Materiales override ─────────────────────────────────────────────────
	/** Materiales GuardiaCivil para aplicar a NPCs (orden: slot 0, 1, ...). */
	UPROPERTY(EditAnywhere, Category="Paranoia|Materiales")
	TArray<TObjectPtr<UMaterialInterface>> MaterialesGuardia;

	/** Material patrol para coches. */
	UPROPERTY(EditAnywhere, Category="Paranoia|Materiales")
	TObjectPtr<UMaterialInterface> MaterialPatrol;

	/** Material translúcido para fantasmas. */
	UPROPERTY(EditAnywhere, Category="Paranoia|Materiales")
	TObjectPtr<UMaterialInterface> MaterialFantasma;

	// ── Audio ───────────────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, Category="Paranoia|Audio")
	TObjectPtr<USoundBase> SirenaSound;

	UPROPERTY(EditAnywhere, Category="Paranoia|Audio")
	TObjectPtr<USoundBase> ParanoiaHeartbeat;

	UPROPERTY(EditAnywhere, Category="Paranoia|Audio")
	TObjectPtr<USoundBase> WhisperAmbiente;

	UPROPERTY(EditAnywhere, Category="Paranoia|Audio")
	TObjectPtr<USoundBase> DistorsionAudio;

	// ── VFX ─────────────────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, Category="Paranoia|VFX")
	TObjectPtr<UNiagaraSystem> TrailFantasma;

	// ── API ─────────────────────────────────────────────────────────────────
	UFUNCTION(BlueprintCallable, Category="Paranoia")
	void RegistrarVictimaCivil(FVector Ubicacion, bool bEsPolicia);

	UFUNCTION(BlueprintPure, Category="Paranoia")
	float GetNivelParanoia() const { return NivelParanoiaActual; }

	/** Llamar cuando el jugador ataca una alucinación. Retorna true si era real. */
	UFUNCTION(BlueprintCallable, Category="Paranoia")
	bool OnAlucinacionAtacada(AActor* Alucinacion);

	// ── Delegate ────────────────────────────────────────────────────────────
	UPROPERTY(BlueprintAssignable, Category="Paranoia")
	FOnParanoiaVisualCambia OnParanoiaVisualCambia;

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;

	// UGameInstanceSubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UParanoiaVisualSubsystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate(); }

private:
	float NivelParanoiaActual = 0.f;

	// ── Material swap state ─────────────────────────────────────────────────
	struct FOriginalMaterialState
	{
		TArray<TObjectPtr<UMaterialInterface>> Originales;
		bool bSwapped = false;
	};
	TMap<TObjectPtr<USceneComponent>, FOriginalMaterialState> MaterialStates;

	void SwapNPCMaterials(AActor* NPC, bool bGuardiaLook);
	void SwapVehicleMaterials(AActor* Vehicle, bool bPatrolLook);
	void RevertAllMaterials();
	void UpdateVisualOverrides();

	// ── Alucinaciones ───────────────────────────────────────────────────────
	UPROPERTY() TArray<FVictimaRecord> VictimRegistradas;
	UPROPERTY() TArray<FAlucinacionState> AlucinacionesActivas;
	float TimerSpawnAlucinacion = 0.f;
	float MaxAlucinacionesSimultaneas = 6;

	void SpawnAlucinacion(FVector Ubicacion, bool bEsReal);
	void TickAlucinaciones(float DeltaTime);
	void UpdateAlucinacionVisuals(FAlucinacionState& Al, float DeltaTime);

	// ── Post-process ────────────────────────────────────────────────────────
	UGameplayPostProcessComponent* FindPlayerPostProcess() const;
	void UpdateParanoiaPostProcess(float DeltaTime);

	float HeartbeatTimer = 0.f;
	float WhisperTimer = 0.f;
	float DistortionTimer = 0.f;

	// ── Environment effects ─────────────────────────────────────────────────
	void TickEnvironmentEffects(float DeltaTime);
	TArray<TObjectPtr<UPointLightComponent>> LightsParpadeantes;
	float LightFlickerTimer = 0.f;
};
