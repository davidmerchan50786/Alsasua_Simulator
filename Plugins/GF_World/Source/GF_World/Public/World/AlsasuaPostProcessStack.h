#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaPostProcessStack.generated.h"

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

private:
	void UpdatePostProcess(float DeltaTime);
};
