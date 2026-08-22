#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaTerrainMaterialBlender.generated.h"

/**
 * Sistema de blending de materiales de terreno multi-capa.
 * Mezcla hierba, tierra, roca, nieve, y asfalto según altitude/pendiente.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GF_WORLD_API UAlsasuaTerrainMaterialBlender : public UActorComponent
{
	GENERATED_BODY()

public:
	UAlsasuaTerrainMaterialBlender();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// --- Layer Thresholds ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Layers")
	float GrassMaxSlope = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Layers")
	float DirtMaxSlope = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Layers")
	float RockMinSlope = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Layers")
	float SnowMinAltitude = 54000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Layers")
	float SnowMaxAltitude = 58000.f;

	// --- Blend Smoothing ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Blend")
	float BlendSmoothness = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Blend")
	float AltitudeBlendRange = 2000.f;

	// --- Seasonal Modifiers ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Season")
	float SpringSnowLine = 56000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Season")
	float SummerSnowLine = 58000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Season")
	float AutumnSnowLine = 54000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Season")
	float WinterSnowLine = 52000.f;

	// --- Wetness Modifiers ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Wetness")
	float WetGrassDarkening = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Wetness")
	float WetDirtDarkening = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Wetness")
	float WetRockShininess = 0.8f;

private:
	void UpdateTerrainBlending(float DeltaTime);
};
