#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaEnhancedPostProcessComponent.generated.h"

class APostProcessVolume;

/**
 * Componente de post-procesado avanzado. Extiende el GameplayPostProcessComponent
 * con: color grading LUT dinámico, chromatic aberration, vignette nocturno,
 * film grain climático, bloom adaptativo, DOF contextual.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GF_WORLD_API UAlsasuaEnhancedPostProcessComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAlsasuaEnhancedPostProcessComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// --- Color Grading ---
	// El tinte se da directo en lugar de derivarlo de grados Kelvin: la fórmula
	// anterior tenía el signo invertido y con NightTemperature=4000 (luz cálida
	// de sodio) enfriaba la imagen en vez de calentarla.

	/** Día: neutro. La luz del sol ya viene con su color desde la atmósfera. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|ColorGrading")
	FLinearColor DayColorTint = FLinearColor(1.f, 1.f, 1.f);

	/** Noche: leve desplazamiento de Purkinje, cómo ve de verdad el ojo escotópico. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|ColorGrading")
	FLinearColor NightColorTint = FLinearColor(0.94f, 0.97f, 1.06f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|ColorGrading")
	float DaySaturation = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|ColorGrading")
	float NightSaturation = 0.88f;

	/** Compensación de exposición nocturna: sin ella el auto-exposure hace de la noche un día. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Exposure")
	float DayExposureBias = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Exposure")
	float NightExposureBias = -1.2f;

	// --- Chromatic Aberration ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Chromatic")
	float DayChromaticAberration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Chromatic")
	float NightChromaticAberration = 0.06f;

	// --- Vignette ---
	// 0.55 era un tunel negro: un objetivo real cae ~0.2-0.3 en las esquinas.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Vignette")
	float DayVignetteIntensity = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Vignette")
	float NightVignetteIntensity = 0.3f;

	// --- Bloom ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Bloom")
	float DayBloomIntensity = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Bloom")
	float NightBloomIntensity = 0.6f;

	// --- Motion Blur ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|MotionBlur")
	float NormalMotionBlur = 0.3f;

private:
	void UpdatePostProcess(float DeltaTime);

	FLinearColor CurrentTint = FLinearColor::White;
	float CurrentSaturation = 1.f;
	float CurrentChromatic = 0.f;
	float CurrentVignette = 0.15f;
	float CurrentBloom = 0.35f;
	float CurrentExposureBias = 0.f;

	// Cached PP volumes (refresh every 5s instead of GetAllActors every 0.1s)
	float PPVolumeRefreshTimer = 0.f;
	TArray<AActor*> CachedPPVolumes;
};
