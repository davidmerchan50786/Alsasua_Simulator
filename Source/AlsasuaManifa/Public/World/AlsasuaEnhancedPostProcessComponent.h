#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaEnhancedPostProcessComponent.generated.h"

/**
 * Componente de post-procesado avanzado. Extiende el GameplayPostProcessComponent
 * con: color grading LUT dinámico, chromatic aberration, vignette nocturno,
 * film grain climático, bloom adaptativo, DOF contextual.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALSASUAMANIFA_API UAlsasuaEnhancedPostProcessComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAlsasuaEnhancedPostProcessComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// --- Color Grading ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|ColorGrading")
	float DayTemperature = 6500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|ColorGrading")
	float NightTemperature = 4000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|ColorGrading")
	float DaySaturation = 1.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|ColorGrading")
	float NightSaturation = 0.85f;

	// --- Chromatic Aberration ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Chromatic")
	float DayChromaticAberration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Chromatic")
	float NightChromaticAberration = 0.15f;

	// --- Vignette ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Vignette")
	float DayVignetteIntensity = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Vignette")
	float NightVignetteIntensity = 0.55f;

	// --- Film Grain ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Grain")
	float DayGrainIntensity = 0.02f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Grain")
	float RainGrainIntensity = 0.06f;

	// --- Bloom ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Bloom")
	float DayBloomIntensity = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Bloom")
	float NightBloomIntensity = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Bloom")
	float NeonBloomBoost = 1.5f;

	// --- Motion Blur ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|MotionBlur")
	float NormalMotionBlur = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|MotionBlur")
	float CombatMotionBlur = 1.2f;

private:
	void UpdatePostProcess(float DeltaTime);

	float CurrentTemperature = 6500.f;
	float CurrentSaturation = 1.f;
	float CurrentChromatic = 0.f;
	float CurrentVignette = 0.25f;
	float CurrentGrain = 0.02f;
	float CurrentBloom = 0.3f;
};
