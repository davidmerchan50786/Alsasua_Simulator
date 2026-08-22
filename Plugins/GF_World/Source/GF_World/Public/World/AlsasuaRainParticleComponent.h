#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaRainParticleComponent.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;

/**
 * Sistema de partículas de lluvia/nieve en tiempo real.
 * Se adjunta al pawn del jugador y genera precipitación visual
 * según el estado del weather subsystem.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GF_WORLD_API UAlsasuaRainParticleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAlsasuaRainParticleComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rain|Niagara")
	TObjectPtr<UNiagaraSystem> RainNiagaraAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rain|Niagara")
	TObjectPtr<UNiagaraSystem> SnowNiagaraAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rain|Niagara")
	TObjectPtr<UNiagaraSystem> ThunderNiagaraAsset;

	// --- Parámetros ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rain|Params")
	float LightRainSpawnRate = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rain|Params")
	float HeavyRainSpawnRate = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rain|Params")
	float ThunderFlashDuration = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rain|Params")
	float ThunderInterval = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rain|Params")
	float SnowSpawnRate = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rain|Params")
	float WindResponse = 0.5f;

	// --- Audio ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rain|Audio")
	TObjectPtr<USoundBase> RainSoundLight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rain|Audio")
	TObjectPtr<USoundBase> RainSoundHeavy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rain|Audio")
	TObjectPtr<USoundBase> ThunderSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rain|Audio")
	TObjectPtr<USoundBase> SnowSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rain|Audio")
	float RainVolumeMax = 0.7f;

private:
	void UpdateRainState(float DeltaTime);
	void SpawnThunderFlash();
	void UpdateWindOnParticles();

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> ActiveRainSystem;

	float ThunderTimer = 0.f;
	float ThunderFlashTimer = 0.f;
	float CurrentSpawnRate = 0.f;
	bool bIsSnowing = false;
};
