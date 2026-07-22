#include "Core/AlsasuaAsyncProcessor.h"
#include "Async/Async.h"

void FVisibilityComputeTask::DoWork()
{
	for (int32 i = 0; i < TargetLocations.Num(); i++)
	{
		float DistSq = FVector::DistSquared(ObserverLoc, TargetLocations[i]);
		// Un cálculo simple pero que en masa (1000 NPCs) costaría en el GameThread
		if (DistSq < FMath::Square(5000.f)) 
		{
			VisibleIndices.Add(i);
		}
	}
}

void UAlsasuaAsyncProcessor::ScheduleVisibilityCheck(FVector ObserverLoc, const TArray<FVector>& Targets)
{
	TWeakObjectPtr<UAlsasuaAsyncProcessor> WeakThis(this);
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [WeakThis, ObserverLoc, Targets]()
	{
		if (!IsValid(WeakThis.Get())) return;
		FVisibilityComputeTask Task(ObserverLoc, Targets);
		Task.DoWork();

		TArray<int32> Results = Task.VisibleIndices;

		AsyncTask(ENamedThreads::GameThread, [WeakThis, Results]()
		{
			if (!IsValid(WeakThis.Get())) return;
			WeakThis->OnVisibilityComplete(Results);
		});
	});
}

void UAlsasuaAsyncProcessor::OnVisibilityComplete(TArray<int32> Results)
{
	// Aquí se notificaría a los sistemas de renderizado o IA
}
