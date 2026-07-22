#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "AI/AlsasuaCrowdSentiment.h"
#include "AlsasuaAudioManager.generated.h"

UCLASS()
class ALSASUAMANIFA_API UAlsasuaAudioManager : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual bool IsAllowedToTick() const { return true; }
	virtual TStatId GetStatId() const { return TStatId(); }

	// Parámetros de control para Audio Gameplay (Metasounds/FMOD)
	UFUNCTION(BlueprintCallable, Category = "AAA|Audio")
	float GetProtestIntensity() const { return CurrentIntensity; }

	UFUNCTION(BlueprintCallable, Category = "AAA|Audio")
	float GetChaosFactor() const { return CurrentChaos; }

private:
	float CurrentIntensity = 0.0f;
	float CurrentChaos = 0.0f;

	// Filtro de paso bajo (LFP) para evitar cambios bruscos en el audio
	void SmoothAudioParameters(float DeltaTime);
};
