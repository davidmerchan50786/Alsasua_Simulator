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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVisibilityUpdated, const TArray<int32>&, VisibleIndices);

UCLASS()
class ALSASUAMANIFA_API UAlsasuaAsyncProcessor : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    void ScheduleVisibilityCheck(FVector ObserverLoc, const TArray<FVector>& Targets);

    UPROPERTY(BlueprintAssignable, Category = "AAA|Async")
    FOnVisibilityUpdated OnVisibilityUpdated;

    UPROPERTY(BlueprintReadOnly, Category = "AAA|Async")
    TArray<int32> LatestVisibleIndices;

protected:
    void OnVisibilityComplete(TArray<int32> Results);
};
