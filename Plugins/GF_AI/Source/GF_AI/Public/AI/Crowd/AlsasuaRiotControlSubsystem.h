// AlsasuaRiotControlSubsystem.h
// ═══════════════════════════════════════════════════════════════════════════
//  Subsistema de mundo para el control de disturbios y eventos probabilísticos.
//  Port del ControladorDisturbios de Unity a UE 5.4 C++.
//
//  Funcionalidad:
//   · Trigger probabilístico de disturbios cada N segundos.
//   · Spawning de bengalas con efectos de luz y partículas.
//   · Reclutamiento de NPCs durante disturbios.
//   · Máquina de estados para escalada de tensión.
// ═══════════════════════════════════════════════════════════════════════════

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "AlsasuaRiotControlSubsystem.generated.h"

class UPointLightComponent;
class UNiagaraComponent;

// ─────────────────────────────────────────────────────────────────────────────
//  Niveles de escalada del disturbio.
// ─────────────────────────────────────────────────────────────────────────────
UENUM(BlueprintType)
enum class ERiotEscalation : uint8
{
	Patrol       UMETA(DisplayName = "Patrulla"),
	Gathering    UMETA(DisplayName = "Agrupamiento"),
	Riot         UMETA(DisplayName = "Revuelta"),
	FullRiot     UMETA(DisplayName = "Revuelta Total")
};

// ─────────────────────────────────────────────────────────────────────────────
//  Configuración del sistema de disturbios.
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct GF_AI_API FRiotControlConfig
{
	GENERATED_BODY()

	/** Intervalo entre chequeos de trigger (segundos). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disturbios", meta = (ClampMin = "1.0"))
	float CheckInterval = 10.f;

	/** Probabilidad base de que se inicie un disturbio en cada chequeo [0,1]. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disturbios", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BaseRiotProbability = 0.05f;

	/** Radio de reclutamiento de NPCs para el disturbio (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disturbios", meta = (ClampMin = "100.0"))
	float RecruitmentRadius = 5000.f;

	/** Número mínimo de NPCs necesarios para iniciar un disturbio. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disturbios", meta = (ClampMin = "1"))
	int32 MinNPCsForRiot = 3;

	/** Número máximo de NPCs reclutados por disturbio. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disturbios", meta = (ClampMin = "1", ClampMax = "50"))
	int32 MaxRecruitsPerRiot = 6;

	/** Duración de la bengala (segundos). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disturbios|Bengala", meta = (ClampMin = "1.0"))
	float FlareDuration = 60.f;

	/** Radio de la luz de la bengala (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disturbios|Bengala", meta = (ClampMin = "100.0"))
	float FlareLightRange = 6000.f;

	/** Intensidad de la luz de la bengala. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disturbios|Bengala", meta = (ClampMin = "0.0"))
	float FlareLightIntensity = 10.f;

	/** Tiempo de enfriamiento después de un disturbio (segundos). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disturbios", meta = (ClampMin = "0.0"))
	float RiotCooldown = 30.f;

	/** Probabilidad de escalada al siguiente nivel tras un disturbio [0,1]. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disturbios|Escalada", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float EscalationProbability = 0.3f;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Datos de un NPC reclutado para un disturbio.
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT()
struct FRiotRecruitData
{
	GENERATED_BODY()

	/** Posición del NPC reclutado. */
	FVector Position = FVector::ZeroVector;

	/** Velocidad del NPC. */
	FVector Velocity = FVector::ZeroVector;

	/** Epicentro del disturbio al que fue reclutado. */
	FVector RiotEpicenter = FVector::ZeroVector;

	/** true si el NPC sigue activo en el disturbio. */
	bool bIsActive = true;

	/** Timer de vida del NPC en el disturbio. */
	float Lifetime = 0.f;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Datos de un disturbio activo.
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT()
struct FRiotInstance
{
	GENERATED_BODY()

	/** Epicentro del disturbio. */
	FVector Epicenter = FVector::ZeroVector;

	/** Nivel de escalada actual. */
	ERiotEscalation Escalation = ERiotEscalation::Gathering;

	/** NPCs reclutados en este disturbio. */
	TArray<FRiotRecruitData> Recruits;

	/** Tiempo de vida del disturbio (segundos). */
	float ElapsedTime = 0.f;

	/** Duración máxima del disturbio (segundos). */
	float MaxDuration = 60.f;

	/** true si el disturbio tiene una bengala activa. */
	bool bHasFlare = false;

	/** Componente de luz de la bengala (nullptr si no hay bengala). */
	UPROPERTY()
	TObjectPtr<UPointLightComponent> FlareLight = nullptr;

	/** Componente de partículas de la bengala. */
	UPROPERTY()
	TObjectPtr<UNiagaraComponent> FlareParticles = nullptr;

	/** true si el disturbio sigue activo. */
	bool bIsActive = true;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Delegados de eventos de disturbios.
// ─────────────────────────────────────────────────────────────────────────────
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRiotStarted, const FVector&, Epicenter, ERiotEscalation, Escalation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRiotEnded, const FVector&, Epicenter);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEscalationChanged, ERiotEscalation, NewLevel);

/**
 * UAlsasuaRiotControlSubsystem
 *
 * Subsistema de mundo que gestiona disturbios probabilísticos, bengalas,
 * reclutamiento de NPCs y escalada de tensión. Port del ControladorDisturbios
 * de Unity a UE 5.4 C++.
 */
UCLASS()
class GF_AI_API UAlsasuaRiotControlSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// ── Ciclo de vida ────────────────────────────────────────────────────────
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// ── API pública ─────────────────────────────────────────────────────────

	/**
	 * Inicia manualmente un disturbio en la posición dada.
	 */
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Disturbios")
	void ForzarDisturbio(const FVector& Epicenter);

	/**
	 * Detiene todos los disturbios activos.
	 */
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Disturbios")
	void DetenerTodosDisturbios();

	/**
	 * Devuelve el número de disturbios activos.
	 */
	UFUNCTION(BlueprintPure, Category = "Alsasua|Disturbios")
	int32 GetActiveRiotCount() const { return ActiveRiots.Num(); }

	/**
	 * Devuelve el nivel de escalada del disturbio más alto.
	 */
	UFUNCTION(BlueprintPure, Category = "Alsasua|Disturbios")
	ERiotEscalation GetHighestEscalation() const;

	/**
	 * Ajusta la probabilidad de disturbio en tiempo real.
	 */
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Disturbios")
	void SetRiotProbability(float NewProbability);

	/** Delegado de inicio de disturbio. */
	UPROPERTY(BlueprintAssignable, Category = "Alsasua|Disturbios")
	FOnRiotStarted OnRiotStarted;

	/** Delegado de fin de disturbio. */
	UPROPERTY(BlueprintAssignable, Category = "Alsasua|Disturbios")
	FOnRiotEnded OnRiotEnded;

	/** Delegado de cambio de escalada. */
	UPROPERTY(BlueprintAssignable, Category = "Alsasua|Disturbios")
	FOnEscalationChanged OnEscalationChanged;

protected:
	// ── Configuración ───────────────────────────────────────────────────────

	/** Configuración del sistema de disturbios. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disturbios")
	FRiotControlConfig Config;

	/** ISMC para bengalas visuales (cubos rojos como markers). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Disturbios|Visual")
	TObjectPtr<UInstancedStaticMeshComponent> FlareMarkerISMC;

private:
	// ── Datos internos ──────────────────────────────────────────────────────

	/** Disturbios activos. */
	UPROPERTY()
	TArray<FRiotInstance> ActiveRiots;

	/** Timer para el chequeo probabilístico. */
	float CheckTimer = 0.f;

	/** Timer de enfriamiento post-disturbio. */
	float CooldownTimer = 0.f;

	/** Handle del timer de actualización del subsistema. */
	FTimerHandle TickTimerHandle;

	/** Método de tick registrado en el TimerManager. */
	void InternalTick();

	// ── Métodos internos ────────────────────────────────────────────────────

	/** Chequeo probabilístico de inicio de disturbio. */
	void ProbabilisticRiotCheck();

	/** Inicia un disturbio en una posición. */
	void StartRiot(const FVector& Epicenter, ERiotEscalation InitialEscalation);

	/** Finaliza un disturbio. */
	void EndRiot(int32 RiotIndex);

	/** Crea los efectos visuales de una bengala. */
	void SpawnFlareEffects(FRiotInstance& Riot);

	/** Actualiza la lógica de un disturbio activo. */
	void TickRiot(FRiotInstance& Riot, float DeltaTime);

	/** Actualiza los recruits de un disturbio. */
	void TickRecruits(FRiotInstance& Riot, float DeltaTime);

	/** Recluta NPCs cercanos al epicentro del disturbio. */
	void RecruitNearbyNPCs(FRiotInstance& Riot);

	/** Limpia los efectos visuales de una bengala. */
	void CleanupFlareEffects(FRiotInstance& Riot);

	/** Muestra el marcador de bengala en el ISMC. */
	void SyncFlareMarkers();

	/** Ajusta la probabilidad de disturbio según el nivel de tensión global. */
	float GetModifiedProbability() const;
};
