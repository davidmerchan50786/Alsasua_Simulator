// AlsasuaManifestacionManager.h
// ═══════════════════════════════════════════════════════════════════════════
//  Subsistema de mundo para mega-manifestaciones (1000 NPCs).
//  Port del MegaManifestacion de Unity a UE 5.4 C++.
//
//  Arquitectura (Líder-Seguidor):
//   · Líderes: NavMeshAgent en UE5 → movimiento pathfinding real.
//   · Seguidores: Boid flocking simplificado (lerp matricial → O(1) por agente).
//   · Renderizado GPU instanced vía UInstancedStaticMeshComponent.
//   · Formaciones: offsets rígidos + interpolación suave.
//   · IA de protesta: cánticos, movimiento, escalada de tensión.
// ═══════════════════════════════════════════════════════════════════════════

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "AlsasuaManifestacionManager.generated.h"

class UInstancedStaticMeshComponent;
class AAIController;

// ─────────────────────────────────────────────────────────────────────────────
//  Estado de tensión de la manifestación.
// ─────────────────────────────────────────────────────────────────────────────
UENUM(BlueprintType)
enum class EManifestacionTension : uint8
{
	Pacifica    UMETA(DisplayName = "Pacífica"),
	Incidente   UMETA(DisplayName = "Incidente"),
	Revuelta    UMETA(DisplayName = "Revuelta")
};

// ─────────────────────────────────────────────────────────────────────────────
//  Datos de un líder de manifestación.
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT()
struct FManifestacionLeaderData
{
	GENERATED_BODY()

	/** Controller AI que controla al líder. */
	UPROPERTY()
	TObjectPtr<AAIController> LeaderController = nullptr;

	/** Posición actual del líder. */
	FVector Position = FVector::ZeroVector;

	/** Velocidad actual del líder. */
	FVector Velocity = FVector::ZeroVector;

	/** Destino NavMesh actual. */
	FVector NavDestination = FVector::ZeroVector;

	/** Ruta de waypoints que sigue el líder. */
	TArray<FVector> RouteWaypoints;

	/** Índice del waypoint actual en la ruta. */
	int32 CurrentWaypoint = 0;

	/** true si el líder está vivo y activo. */
	bool bIsActive = true;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Datos de un seguidor de manifestación.
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT()
struct FManifestacionFollowerData
{
	GENERATED_BODY()

	/** Posición actual del seguidor. */
	FVector Position = FVector::ZeroVector;

	/** Velocidad actual del seguidor. */
	FVector Velocity = FVector::ZeroVector;

	/** Altura del suelo bajo el seguidor. */
	float GroundHeight = 0.f;

	/** Índice del líder asignado. */
	int32 AssignedLeaderIndex = 0;

	/** Offset lateral respecto al líder (cm). */
	float OffsetX = 0.f;

	/** Offset frontal respecto al líder (cm, siempre negativo = detrás). */
	float OffsetZ = -500.f;

	/** true si el seguidor está vivo y activo. */
	bool bIsActive = true;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Configuración de la manifestación.
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct GF_AI_API FManifestacionConfig
{
	GENERATED_BODY()

	/** Número total de manifestantes (incluye líderes). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Manifestacion", meta = (ClampMin = "10", ClampMax = "2000"))
	int32 TotalManifestantes = 1000;

	/** Número de líderes (con NavMeshAgent real). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Manifestacion", meta = (ClampMin = "1", ClampMax = "100"))
	int32 NumLeaders = 20;

	/** Velocidad de marcha de los líderes (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Manifestacion", meta = (ClampMin = "50.0"))
	float MarchSpeed = 150.f;

	/** Radio de dispersión inicial del spawn (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Manifestacion", meta = (ClampMin = "100.0"))
	float SpawnRadius = 3000.f;

	/** Radio máximo de seguimiento (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Manifestacion|Follower", meta = (ClampMin = "100.0"))
	float FollowLerpSpeed = 1.5f;

	/** Ruta de waypoints de la manifestación. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Manifestacion|Ruta")
	TArray<FVector> RouteWaypoints;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Delegados de eventos de manifestación.
// ─────────────────────────────────────────────────────────────────────────────
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTensionChanged, EManifestacionTension, NewTension);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLeaderDied, int32, LeaderIndex);

/**
 * UAlsasuaManifestacionManager
 *
 * Subsistema de mundo que gestiona mega-manifestaciones con líderes y seguidores.
 * Port directo del MegaManifestacion de Unity a UE 5.4 C++.
 *
 * Los líderes usan NavMeshAgent (AI movement real) mientras que los seguidores
 * son puros boids matemáticos sin pathfinding (lerp + offset rígido).
 */
UCLASS()
class GF_AI_API UAlsasuaManifestacionManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// ── Ciclo de vida ────────────────────────────────────────────────────────
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ── API pública ─────────────────────────────────────────────────────────

	/**
	 * Inicia una mega-manifestación con la configuración dada.
	 */
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Manifestacion")
	void IniciarManifestacion(const FManifestacionConfig& Config);

	/**
	 * Detiene y despawnnea toda la manifestación.
	 */
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Manifestacion")
	void DetenerManifestacion();

	/**
	 * Devuelve la tensión actual de la manifestación.
	 */
	UFUNCTION(BlueprintPure, Category = "Alsasua|Manifestacion")
	EManifestacionTension GetTensionActual() const { return TensionActual; }

	/**
	 * Establece la tensión de la manifestación manualmente.
	 */
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Manifestacion")
	void SetTension(EManifestacionTension NuevaTension);

	/**
	 * Añade un punto a la ruta de la manifestación en runtime.
	 */
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Manifestacion")
	void AddRoutePoint(const FVector& Point);

	/** Delegado de cambio de tensión. */
	UPROPERTY(BlueprintAssignable, Category = "Alsasua|Manifestacion")
	FOnTensionChanged OnTensionChanged;

	/** Delegado de muerte de líder. */
	UPROPERTY(BlueprintAssignable, Category = "Alsasua|Manifestacion")
	FOnLeaderDied OnLeaderDied;

protected:
	// ── Configuración ───────────────────────────────────────────────────────

	/** Componente ISMC para líderes (malla distinta, más elaborada). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Manifestacion|Visual")
	TObjectPtr<UInstancedStaticMeshComponent> LeaderISMC;

	/** Componente ISMC para seguidores (malla más simple, bulk). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Manifestacion|Visual")
	TObjectPtr<UInstancedStaticMeshComponent> FollowerISMC;

	/** Material para líderes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Manifestacion|Visual")
	TObjectPtr<UMaterialInterface> LeaderMaterial;

	/** Material para seguidores. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Manifestacion|Visual")
	TObjectPtr<UMaterialInterface> FollowerMaterial;

	/** Intervalo de actualización de la lógica (segundos). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Manifestacion|Performance")
	float UpdateInterval = 0.033f; // ~30 Hz

private:
	// ── Datos internos ──────────────────────────────────────────────────────

	/** Datos de líderes. */
	TArray<FManifestacionLeaderData> Leaders;

	/** Datos de seguidores. */
	TArray<FManifestacionFollowerData> Followers;

	/** Configuración activa. */
	FManifestacionConfig ActiveConfig;

	/** Tensión actual. */
	EManifestacionTension TensionActual = EManifestacionTension::Pacifica;

	/** Timer de actualización. */
	float UpdateTimer = 0.f;

	/** Handle del timer de actualización del subsistema. */
	FTimerHandle TickTimerHandle;

	/** Centroid de los líderes (para cálculos de dispersión). */
	FVector LeadersCentroid = FVector::ZeroVector;

	/** Dirección promedio de movimiento de líderes. */
	FVector LeadersAvgDirection = FVector::ForwardVector;

	// ── Métodos internos ────────────────────────────────────────────────────

	/** Crea un líder con NavMeshAgent. */
	void SpawnLeader(int32 LeaderIndex, const FVector& SpawnPos);

	/** Asigna una nueva ruta a un líder usando NavMesh. */
	void AssignNewRouteToLeader(int32 LeaderIndex);

	/** Actualiza la lógica de todos los líderes (NavMesh movement). */
	void TickLeaders(float DeltaTime);

	/** Actualiza la lógica de todos los seguidores (lerp + offset). */
	void TickFollowers(float DeltaTime);

	/** Reasigna seguidores huérfanos cuando su líder muere. */
	void ReassignOrphanedFollowers(int32 DeadLeaderIndex);

	/** Sincroniza las instancias ISMC con las posiciones actuales. */
	void SyncInstancedTransforms();

	/** Inicializa los componentes ISMC. */
	void SetupISMCs();

	/** Método de tick registrado en el TimerManager. */
	void InternalTick();
};
