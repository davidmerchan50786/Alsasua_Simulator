// ArranqueMundo.cpp
#include "ArranqueMundo.h"
#include "Engine/Engine.h"

namespace ArranqueMundo
{
	bool  HayDirector   = false;
	bool  BaselineListo = false;
	float Progreso      = 0.f;

	FString GetDebugSummary()
	{
		return FString::Printf(TEXT("HayDirector=%s | BaselineListo=%s | Progreso=%.2f"),
			HayDirector ? TEXT("yes") : TEXT("no"),
			BaselineListo ? TEXT("yes") : TEXT("no"),
			Progreso);
	}

	void SetProgress(float InProgress)
	{
		Progreso = FMath::Clamp(InProgress, 0.f, 1.f);
	}

	void MarkPhaseComplete(const FString& PhaseName, float InProgress)
	{
		SetProgress(InProgress);
		UE_LOG(LogTemp, Log, TEXT("[Arranque] %s -> %s"), *PhaseName, *GetDebugSummary());
	}
}
