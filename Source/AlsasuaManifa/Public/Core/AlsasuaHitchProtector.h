#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "AlsasuaHitchProtector.generated.h"

UCLASS()
class ALSASUAMANIFA_API UAlsasuaHitchProtector : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return !IsTemplate(); }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UAlsasuaHitchProtector, STATGROUP_Game); }

	// Devuelve el factor de escala de densidad actual (1.0 = normal, 0.2 = pánico)
	UFUNCTION(BlueprintCallable, Category = "AAA|Optimization")
	float GetGlobalLODScale() const { return CurrentLODScale; }

private:
	float CurrentLODScale = 1.0f;
	float PanicDuration = 0.0f;
	const float HitchThreshold = 0.033f; // 33ms = 30fps baseline
};
