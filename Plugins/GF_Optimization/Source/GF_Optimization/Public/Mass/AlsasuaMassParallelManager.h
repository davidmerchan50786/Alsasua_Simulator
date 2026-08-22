#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "Mass/AlsasuaMassProxies.h"
#include "AlsasuaMassParallelManager.generated.h"

/** Estructura para el procesamiento de datos en paralelo (Thread Safe) */
struct FMassUpdateBatch
{
    int32 StartIndex;
    int32 EndIndex;
    float DeltaTime;
    FVector PlayerLocation;
};

/**
 * Gestor de proxies de multitud con actualización paralela.
 * Divide el array de proxies en batches y los procesa en paralelo
 * usando FRunnable o AsyncTask.
 */
UCLASS()
class GF_OPTIMIZATION_API UAlsasuaMassParallelManager : public UWorldSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Tick(float DeltaTime) override;
    virtual bool IsAllowedToTick() const override { return true; }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UAlsasuaMassParallelManager, STATGROUP_Tickables); }
    virtual bool IsTickable() const override { return !IsTemplate(); }   // no ticar el CDO (CLAUDE.md §9)

    /** Array de proxies de manifestantes. */
    UPROPERTY()
    TArray<FMassProtesterProxy> Proxies;

    /** Número de proxies objetivo para spawnear. */
    UPROPERTY(EditAnywhere, Category = "Mass")
    int32 TargetProxyCount = 500;

    /** Radio de spawn alrededor del jugador (cm). */
    UPROPERTY(EditAnywhere, Category = "Mass")
    float SpawnRadius = 5000.f;

    /** Velocidad base de los proxies (cm/s). */
    UPROPERTY(EditAnywhere, Category = "Mass")
    float BaseSpeed = 120.f;

    /** Número de batches para procesamiento paralelo. */
    UPROPERTY(EditAnywhere, Category = "Mass|Performance", meta = (ClampMin = "1", ClampMax = "16"))
    int32 NumBatches = 4;

    /** Ejecuta la actualización paralela de proxies. */
    void ExecuteParallelUpdate(float DeltaTime);

    /** Spawnea proxies para llenar TargetProxyCount. */
    void SpawnProxies();

private:
    /** Referencia cacheada al jugador. */
    FVector CachedPlayerLocation = FVector::ZeroVector;

    /** Timer de actualización de cache del jugador. */
    float PlayerCacheTimer = 0.f;

    /** Actualiza la posición cacheada del jugador. */
    void UpdatePlayerCache();
};
