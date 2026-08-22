// AlsasuaCrowdSubsystem.h
// ═══════════════════════════════════════════════════════════════════════════
//  Subsistema de mundo (UWorldSubsystem) para la simulación de multitud.
//  Port del SistemaMultitud de Unity a UE 5.4 C++.
//
//  Arquitectura:
//   · UInstancedStaticMeshComponent para renderizado GPU instanced (O(1) draw calls).
//   · Boid flocking: separación, alineación, cohesión (Spatial Hash Grid O(N)).
//   · Actualización a 30 Hz (no cada frame) para ahorrar CPU.
//   · Double buffering de datos de agentes para separar lógica de lectura/escritura.
//   · Spawning desde TArray<FVector> de puntos de ruta.
// ═══════════════════════════════════════════════════════════════════════════

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "AI/Crowd/SpatialHashGrid.h"
#include "AlsasuaCrowdSubsystem.generated.h"

class UInstancedStaticMeshComponent;
class ACrowdRagdollActor;
enum class ERagdollQuality : uint8;

// ─────────────────────────────────────────────────────────────────────────────
//  Datos de un agente de multitud (SoA-friendly, port del struct AgentData).
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT()
struct FAcrowdAgentData
{
	GENERATED_BODY()

	/** Posición world actual. */
	FVector Position = FVector::ZeroVector;

	/** Velocidad actual (cm/s). */
	FVector Velocity = FVector::ZeroVector;

	/** Índice del waypoint hacia el que se dirige. */
	int32 CurrentWaypoint = 0;

	/** Altura Y del suelo bajo el agente (para mantenerlo en el terreno). */
	float GroundHeight = 0.f;

	/** Si false, el agente está muerto y no participa en flocking. */
	bool bAlive = true;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Configuración de flocking (editable desde Blueprint o datos).
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct GF_AI_API FCrowdFlockingParams
{
	GENERATED_BODY()

	/** Velocidad base de marcha (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multitud", meta = (ClampMin = "50.0"))
	float MarchSpeed = 130.f;

	/** Radio de separación (cm). Los agentes se apartan de vecinos en este radio. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multitud|Flocking", meta = (ClampMin = "10.0"))
	float SeparationRadius = 65.f;

	/** Radio de cohesión (cm). Los agentes se acercan al centro de masa local. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multitud|Flocking", meta = (ClampMin = "30.0"))
	float CohesionRadius = 220.f;

	/** Radio de alineación (cm). Los agentes igualan velocidad con vecinos. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multitud|Flocking", meta = (ClampMin = "30.0"))
	float AlignmentRadius = 180.f;

	/** Peso de la fuerza de separación. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multitud|Flocking", meta = (ClampMin = "0.0"))
	float SeparationWeight = 2.2f;

	/** Peso de la fuerza de cohesión. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multitud|Flocking", meta = (ClampMin = "0.0"))
	float CohesionWeight = 0.9f;

	/** Peso de la fuerza de alineación. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multitud|Flocking", meta = (ClampMin = "0.0"))
	float AlignmentWeight = 1.0f;

	/** Peso de la fuerza de seguimiento de ruta. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multitud|Flocking", meta = (ClampMin = "0.0"))
	float RouteWeight = 3.0f;

	/** Ancho de la formación (agentes por fila). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multitud|Formacion", meta = (ClampMin = "1", ClampMax = "50"))
	int32 FormationWidth = 13;

	/** Separación lateral entre agentes en la formación (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multitud|Formacion", meta = (ClampMin = "20.0"))
	float FormationSpacingX = 72.f;

	/** Separación frontal entre filas en la formación (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multitud|Formacion", meta = (ClampMin = "20.0"))
	float FormationSpacingZ = 80.f;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Datos de inicialización para spawnear un grupo de agentes.
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct GF_AI_API FCrowdSpawnRequest
{
	GENERATED_BODY()

	/** Puntos de ruta que los agentes seguirán. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multitud|Spawn")
	TArray<FVector> RoutePoints;

	/** Número de agentes a spawnear. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multitud|Spawn", meta = (ClampMin = "1", ClampMax = "1000"))
	int32 NumAgents = 100;

	/** Centro del área de spawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multitud|Spawn")
	FVector SpawnCenter = FVector::ZeroVector;

	/** Radio de dispersión alrededor del centro de spawn (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multitud|Spawn", meta = (ClampMin = "0.0"))
	float SpawnRadius = 500.f;

	/** Parámetros de flocking para estos agentes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multitud|Spawn")
	FCrowdFlockingParams FlockingParams;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Delegado que se emite cuando cambia el número total de agentes.
// ─────────────────────────────────────────────────────────────────────────────
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCrowdAgentCountChanged, int32, TotalAgents);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCrowdAgentKilled, int32, AgentIndex, FVector, DeathLocation);

/**
 * UAlsasuaCrowdSubsystem
 *
 * Subsistema de mundo que gestiona la multitud de manifestantes usando
 * boid flocking y renderizado GPU instanced (UInstancedStaticMeshComponent).
 *
 * Inspirado directamente en el SistemaMultitud de Unity pero portado a la
 * arquitectura de UE 5.4:
 *   - En lugar de Graphics.DrawMeshInstanced, usamos ISMC.
 *   - En lugar de IJobParallelFor, usamos tick a 30 Hz con FSpatialHashGrid.
 *   - El SpatialHashGrid reemplaza la NativeArray flat-grid.
 */
UCLASS()
class GF_AI_API UAlsasuaCrowdSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// ── Ciclo de vida del subsistema ────────────────────────────────────────
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ── API pública ─────────────────────────────────────────────────────────

	/**
	 * Spawna un grupo de agentes de multitud según la solicitud dada.
	 * @param Request  Parámetros de spawn (ruta, número, posición, flocking).
	 * @return         Primer índice del rango de agentes creados (-1 si falla).
	 */
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Multitud")
	int32 SpawnCrowdAgents(const FCrowdSpawnRequest& Request);

	/**
	 * Elimina todos los agentes de multitud.
	 */
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Multitud")
	void DespawnAllAgents();

	/**
	 * Devuelve el número total de agentes activos.
	 */
	UFUNCTION(BlueprintPure, Category = "Alsasua|Multitud")
	int32 GetAgentCount() const { return Agents.Num(); }

	/**
	 * Devuelve la posición de un agente por índice.
	 */
	UFUNCTION(BlueprintPure, Category = "Alsasua|Multitud")
	FVector GetAgentPosition(int32 AgentIndex) const;

	/**
	 * Actualiza los parámetros de flocking de todos los agentes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Multitud")
	void SetGlobalFlockingParams(const FCrowdFlockingParams& InParams);

	/**
	 * Mata un agente de multitud: spawnea un ragdoll pooled en su posición,
	 * lo elimina del flocking, y el ragdoll se devuelve al pool tras un tiempo.
	 * @param AgentIndex  Índice del agente a matar.
	 * @param DeathImpulse Dirección del impulso al ragdoll (se normaliza).
	 */
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Multitud")
	void KillAgent(int32 AgentIndex, FVector DeathImpulse = FVector::ForwardVector);

	/**
	 * Elimina un agente del sistema (sin ragdoll, útil para despawning silencioso).
	 */
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Multitud")
	void DespawnAgent(int32 AgentIndex);

	/**
	 * Obtiene el número de agentes vivos (excluye muertos).
	 */
	UFUNCTION(BlueprintPure, Category = "Alsasua|Multitud")
	int32 GetAliveAgentCount() const;

	/** Delegado de cambio de conteo de agentes. */
	UPROPERTY(BlueprintAssignable, Category = "Alsasua|Multitud")
	FOnCrowdAgentCountChanged OnAgentCountChanged;

	/** Delegado emitido cuando un agente muere. */
	UPROPERTY(BlueprintAssignable, Category = "Alsasua|Multitud")
	FOnCrowdAgentKilled OnAgentKilled;

protected:
	// ── Configuración del subsistema ────────────────────────────────────────

	/** Malla usada para el instanced rendering de agentes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multitud|Visual")
	TObjectPtr<UStaticMesh> AgentMesh;

	/** Material para las instancias (debe soportar instancing). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multitud|Visual")
	TObjectPtr<UMaterialInterface> AgentMaterial;

	/** Componente ISMC principal para el renderizado GPU instanced. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Multitud|Visual")
	TObjectPtr<UInstancedStaticMeshComponent> InstancedMeshComponent;

	/** Parámetros de flocking globales. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multitud|Flocking")
	FCrowdFlockingParams GlobalFlockingParams;

	/** Frecuencia de actualización de la lógica de multitud (Hz). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multitud|Performance", meta = (ClampMin = "1.0", ClampMax = "120.0"))
	float UpdateFrequency = 30.f;

	/** Número máximo de agentes soportados. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multitud|Performance", meta = (ClampMin = "1", ClampMax = "2000"))
	int32 MaxAgents = 700;

	/** Escala visual de los agentes instanced. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multitud|Visual", meta = (ClampMin = "0.01"))
	FVector AgentScale = FVector(0.35f, 0.35f, 0.875f);

	/** Offset vertical del mesh del agente sobre la posición lógica. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multitud|Visual")
	float VisualOffsetZ = 87.5f;

private:
	// ── Datos internos ──────────────────────────────────────────────────────

	/** Array de agentes (double buffered logic). */
	TArray<FAcrowdAgentData> Agents;

	/** Spatial hash grid para búsquedas de vecinos. */
	FSpatialHashGrid SpatialGrid;

	/** Waypoints aplanados (ruta global de la marcha). */
	TArray<FVector> RouteWaypoints;

	/** Colores por instancia para variación visual. */
	TArray<FLinearColor> AgentColors;

	/** Temporizador para la frecuencia de actualización. */
	float UpdateTimer = 0.f;

	/** Intervalo en segundos entre ticks de lógica. */
	float UpdateInterval = 1.f / 30.f;

	/** Índice de raycast para muestreo de suelo por turno. */
	int32 GroundRaycastIndex = 0;

	/** Paleta de colores predefinida para los agentes. */
	static const TArray<FLinearColor> ColorPalette;

	/** Handle del timer de actualización del subsistema. */
	FTimerHandle TickTimerHandle;

	/** Clase de actor ragdoll a instanciar para muertes. */
	UPROPERTY(EditAnywhere, Category = "Multitud|Ragdoll")
	TSubclassOf<AActor> RagdollActorClass;

	/** Número de ragdolls pre-warm en el pool. */
	UPROPERTY(EditAnywhere, Category = "Multitud|Ragdoll", meta = (ClampMin = "0", ClampMax = "100"))
	int32 RagdollPoolSize = 30;

	/** Máximo de ragdolls activos simultáneamente (si se supera, el más antiguo se recicla). */
	UPROPERTY(EditAnywhere, Category = "Multitud|Ragdoll", meta = (ClampMin = "1", ClampMax = "50"))
	int32 MaxActiveRagdolls = 15;

	/** Distancia al jugador para ragdoll Full (física completa) (cm). */
	UPROPERTY(EditAnywhere, Category = "Multitud|Ragdoll|LOD", meta = (ClampMin = "100.0"))
	float RagdollFullDistance = 500.f;

	/** Distancia al jugador para ragdoll Frozen (impulso + freeze) (cm). */
	UPROPERTY(EditAnywhere, Category = "Multitud|Ragdoll|LOD", meta = (ClampMin = "200.0"))
	float RagdollFrozenDistance = 2000.f;

	/** Tiempo antes de limpiar agentes muertos del array (segundos). */
	UPROPERTY(EditAnywhere, Category = "Multitud|Ragdoll", meta = (ClampMin = "0.5"))
	float DeadAgentCleanupInterval = 5.0f;

	/** Timer para limpieza periódica de agentes muertos. */
	FTimerHandle DeadAgentCleanupTimerHandle;

	/** Lista de ragdolls activos actualmente (para gestión de pool y LOD). */
	UPROPERTY()
	TArray<TObjectPtr<ACrowdRagdollActor>> ActiveRagdolls;

	// ── Métodos internos ────────────────────────────────────────────────────

	/** Método de tick registrado en el TimerManager. */
	void InternalTick();

	/** Inicializa el ISMC y crea la malla por defecto si no se asignó. */
	void SetupInstancedRendering();

	/** Crea una malla de cápsula procedural (port de ObtenerMeshCapsula). */
	UStaticMesh* CreateDefaultCapsuleMesh() const;

	/** Inicializa agentes en formación alrededor de un punto. */
	void InitializeAgentsInFormation(int32 StartIndex, int32 Count,
		const FVector& Center, const FVector& ForwardDir, const FCrowdFlockingParams& Params);

	/** Actualiza la lógica de flocking para todos los agentes. */
	void TickFlocking(float DeltaTime);

	/** Actualiza el Spatial Hash Grid con las posiciones actuales. */
	void RebuildSpatialGrid();

	/** Muestrea el suelo bajo un agente (line trace). */
	void SampleGroundHeight(int32 AgentIndex);

	/** Sincroniza las transformaciones del ISMC con los datos de agentes. */
	void SyncInstancedTransforms();

	/** Asigna un color de la paleta a cada agente. */
	void AssignAgentColors();

	/** Limpia agentes muertos del array (compacta índices y actualiza ISMC). */
	void CleanupDeadAgents();

	/** Callback del timer de limpieza de muertos. */
	void OnDeadAgentCleanupTick();
};
