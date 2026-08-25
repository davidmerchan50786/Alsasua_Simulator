#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AguaNivel.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

UCLASS()
class ALSASUAWORLD_API AAguaNivel : public AActor
{
	GENERATED_BODY()

public:
	AAguaNivel();

	UPROPERTY(EditAnywhere, Category="Agua|Geometry")
	float AnchoCm = 800000.f;

	UPROPERTY(EditAnywhere, Category="Agua|Geometry")
	float LargoCm = 800000.f;

	UPROPERTY(EditAnywhere, Category="Agua|Geometry")
	float AlturaNivelCm = -1393.9f;

	UPROPERTY(EditAnywhere, Category="Agua|Visual")
	FLinearColor WaterColor = FLinearColor(0.01f, 0.05f, 0.15f, 0.85f);

	/** Water color shifts darker and greener during rain. */
	UPROPERTY(EditAnywhere, Category="Agua|Visual")
	FLinearColor RainWaterColor = FLinearColor(0.008f, 0.035f, 0.1f, 0.9f);

	UPROPERTY(EditAnywhere, Category="Agua|Visual")
	float OpacityBase = 0.85f;

	UPROPERTY(EditAnywhere, Category="Agua|Visual")
	float RainOpacityBoost = 0.08f;

	UPROPERTY(EditAnywhere, Category="Agua|Visual")
	float FresnelPower = 4.0f;

	UPROPERTY(EditAnywhere, Category="Agua|Visual")
	float WaveSpeed = 0.5f;

	// Gerstner wave parameters
	UPROPERTY(EditAnywhere, Category="Agua|Waves")
	float WaveAmplitude = 15.0f;

	UPROPERTY(EditAnywhere, Category="Agua|Waves")
	float WaveFrequency = 0.02f;

	UPROPERTY(EditAnywhere, Category="Agua|Waves")
	float WaveSteepness = 0.5f;

	UPROPERTY(EditAnywhere, Category="Agua|Waves")
	int32 WaveDirections = 3;

	/** Extra wave amplitude added by wind. WindSpeed 0-30 kmh. */
	UPROPERTY(EditAnywhere, Category="Agua|Waves")
	float WindAmplitudeScale = 3.0f;

	/** Wave frequency shift with wind (higher = choppier). */
	UPROPERTY(EditAnywhere, Category="Agua|Waves")
	float WindFrequencyScale = 0.003f;

	// Shoreline parameters
	UPROPERTY(EditAnywhere, Category="Agua|Shoreline")
	float ShorelineDepth = 200.0f;

	UPROPERTY(EditAnywhere, Category="Agua|Shoreline")
	FLinearColor ShorelineFoamColor = FLinearColor(0.8f, 0.85f, 0.9f, 0.6f);

	/** Extra foam intensity during storms. */
	UPROPERTY(EditAnywhere, Category="Agua|Shoreline")
	float StormFoamBoost = 0.4f;

	// Rain ripple parameters
	UPROPERTY(EditAnywhere, Category="Agua|Rain")
	float RainRippleIntensity = 0.3f;

	UPROPERTY(EditAnywhere, Category="Agua|Rain")
	float RainRippleSpeed = 2.0f;

	UPROPERTY(EditAnywhere, Category="Agua|Rain")
	float RainRippleScale = 15.0f;

	// Specular / reflection
	/** Specular power: lower = wider highlight. Rain makes it shinier. */
	UPROPERTY(EditAnywhere, Category="Agua|Specular")
	float SpecularPower = 64.0f;

	UPROPERTY(EditAnywhere, Category="Agua|Specular")
	float RainSpecularBoost = 2.0f;

	// Depth-based opacity
	/** Max opacity at deep water. */
	UPROPERTY(EditAnywhere, Category="Agua|Depth")
	float DeepOpacity = 0.95f;

	/** Min opacity at shallow shoreline. */
	UPROPERTY(EditAnywhere, Category="Agua|Depth")
	float ShallowOpacity = 0.3f;

	/** Depth fade distance in cm. */
	UPROPERTY(EditAnywhere, Category="Agua|Depth")
	float DepthFadeDistance = 300.0f;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY() UStaticMeshComponent* WaterMesh;
	UPROPERTY() UMaterialInstanceDynamic* WaterMatInst;

	void CrearMaterialDinamico();
	void ActualizarOndas(float Time);
	void ActualizarClima();
};
