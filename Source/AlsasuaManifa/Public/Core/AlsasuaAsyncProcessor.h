#pragma once

#include "CoreMinimal.h"
#include "Async/AsyncWork.h"
#include "Subsystems/WorldSubsystem.h"
#include "AlsasuaAsyncProcessor.generated.h"

/** Tarea para procesamiento de visibilidad masiva en otro hilo */
class FVisibilityComputeTask : public FNonAbandonableTask
{
public:
	FVisibilityComputeTask(FVector InObserverLoc, TArray<FVector> InTargetLocations)
		: ObserverLoc(InObserverLoc), TargetLocations(InTargetLocations) {}

	FORCEINLINE TStatId GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(FVisibilityComputeTask, STATGROUP_ThreadPoolAsyncTasks); }

	void DoWork();

	// Resultado: Índices de objetivos que son "visibles" (distancia/ángulo)
	TArray<int32> VisibleIndices;

private:
	FVector ObserverLoc;
	TArray<FVector> TargetLocations;
};

UCLASS()
class ALSASUAMANIFA_API UAlsasuaAsyncProcessor : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// Encola una tarea de cálculo de visibilidad para no bloquear el GameThread
	void ScheduleVisibilityCheck(FVector ObserverLoc, const TArray<FVector>& Targets);

protected:
	// Callback cuando la tarea termina (Thread Safe)
	void OnVisibilityComplete(TArray<int32> Results);
};
