#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AlsasuaActorPoolSubsystem.generated.h"

/**
 * Pool genérico de actores para evitar spawn/destrucción en runtime.
 * Soporta múltiples clases, warm-up pre-allocado, auto-return por timer,
 * y estadísticas de uso.
 *
 * Uso:
 *   Pool->WarmUpPool<ACrowdRagdollActor>(20);
 *   AActor* Actor = Pool->AcquireActor(ACrowdRagdollActor::StaticClass(), Loc, Rot);
 *   Pool->ReleaseActor(Actor, 5.0f); // auto-return tras 5s
 */
UCLASS()
class ALSASUAMANIFA_API UAlsasuaActorPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ── Pre-warm: crea actores inactivos para evitar hitch en runtime ──────
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Pool")
	void WarmUpPool(TSubclassOf<AActor> Class, int32 Size);

	// ── Acquire: obtiene un actor del pool o crea uno si está vacío ────────
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Pool")
	AActor* AcquireActor(TSubclassOf<AActor> Class, FVector Location, FRotator Rotation);

	// ── Release: devuelve un actor al pool, lo desactiva ───────────────────
	//  Si AutoReturnTime > 0, se devuelve automáticamente tras ese tiempo.
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Pool")
	void ReleaseActor(AActor* Actor, float AutoReturnTime = 0.f);

	// ── Flush: devuelve todos los actores inactivos al pool ────────────────
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Pool")
	void ReleaseAll();

	// ── Stats ──────────────────────────────────────────────────────────────
	UFUNCTION(BlueprintPure, Category = "Alsasua|Pool")
	int32 GetInactiveCount(TSubclassOf<AActor> Class) const;

	UFUNCTION(BlueprintPure, Category = "Alsasua|Pool")
	int32 GetTotalCount(TSubclassOf<AActor> Class) const;

	UFUNCTION(BlueprintPure, Category = "Alsasua|Pool")
	int32 GetTotalAllClasses() const;

private:
	// Pool por clase: InactiveActors contiene actores disponibles.
	TMap<UClass*, TArray<AActor*>> Pools;

	// Conteo total de actores creados por clase (activos + inactivos).
	TMap<UClass*, int32> TotalCounts;

	// Timers de auto-return: actor → handle.
	TMap<AActor*, FTimerHandle> AutoReturnTimers;

	void DeactivateActor(AActor* Actor);
	void ActivateActor(AActor* Actor, FVector Location, FRotator Rotation);
	void OnAutoReturnTimerExpired(AActor* Actor);
};
