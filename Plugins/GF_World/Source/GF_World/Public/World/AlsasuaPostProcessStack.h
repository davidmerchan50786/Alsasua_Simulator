#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaPostProcessStack.generated.h"

class APostProcessVolume;
class UTexture;

/**
 * Stack de post-procesado avanzado: LUT, vignette, film grain, chromatic aberration.
 * Extiende el sistema existente con más efectos.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GF_WORLD_API UAlsasuaPostProcessStack : public UActorComponent
{
	GENERATED_BODY()

public:
	UAlsasuaPostProcessStack();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// --- Color Grading LUT ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|LUT")
	bool bEnableLUT = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|LUT")
	TObjectPtr<UTexture> ColorGradingLUT;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|LUT")
	float LUTBlendWeight = 0.5f;

	// --- Film Grain ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Grain")
	bool bEnableFilmGrain = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Grain")
	float GrainIntensity = 0.015f;

	// --- Bloom ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Bloom")
	float BloomThreshold = 1.f;

	// --- Exposure ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Exposure")
	float ExposureMinBrightness = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Exposure")
	float ExposureMaxBrightness = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Exposure")
	float ExposureSpeed = 2.f;

	// --- Contextual DOF ---
	/** Enable distance-based depth of field for cinematic depth perception. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|DOF")
	bool bEnableContextualDOF = true;

	/** Focal distance in cm (distance to sharp focus plane). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|DOF")
	float DOFFocalDistance = 3000.f;

	/** Aperture F-stop: lower = shallower DOF. 16 = deep focus, 1.4 = cinematic bokeh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|DOF", meta=(ClampMin="1.0", ClampMax="32.0"))
	float DOFAperture = 5.6f;

	/** Maximum blur amount (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|DOF")
	float DOFMaxBlur = 4.0f;

private:
	void UpdatePostProcess(float DeltaTime);

	float PPVolumeRefreshTimer = 0.f;
	TArray<AActor*> CachedPPVolumes;
};
