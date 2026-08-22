#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaWaterReflectionManager.generated.h"

class UPlanarReflectionComponent;
class UStaticMeshComponent;

/**
 * Gestiona reflejos planos en el río Arakil. Crea planar reflections
 * y actualiza materiales de agua con parámetros de reflejo dinámicos.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GF_WORLD_API UAlsasuaWaterReflectionManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UAlsasuaWaterReflectionManager();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// --- Reflection Quality ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Reflection")
	float ReflectionQuality = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Reflection")
	int32 ReflectionResolutionWidth = 1024;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Reflection")
	int32 ReflectionResolutionHeight = 1024;

	// --- Water Properties ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Props")
	float WaterSpeed = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Props")
	float WaveAmplitude = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Props")
	float WaveFrequency = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Props")
	FLinearColor ShallowColor = FLinearColor(0.1f, 0.3f, 0.4f, 0.8f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Props")
	FLinearColor DeepColor = FLinearColor(0.01f, 0.05f, 0.12f, 0.95f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Props")
	float FoamIntensity = 0.3f;

	// --- Rain Response ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Rain")
	float RainRippleIntensity = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Rain")
	float RainNormalStrength = 0.8f;

private:
	void SetupReflections();
	void UpdateWaterParameters(float DeltaTime);

	float TimeAccum = 0.f;
};
