#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaFoliageDensitySystem.generated.h"

/**
 * Sistema de densidad de foliage por bioma.
 * Ajusta densidad de hierba, arbustos, y árboles según el barrio.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALSASUAMANIFA_API UAlsasuaFoliageDensitySystem : public UActorComponent
{
	GENERATED_BODY()

public:
	UAlsasuaFoliageDensitySystem();

	virtual void BeginPlay() override;

	// --- Urban Density ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage|Urban")
	float UrbanGrassDensity = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage|Urban")
	float UrbanBushDensity = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage|Urban")
	float UrbanTreeDensity = 0.3f;

	// --- Residential Density ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage|Residential")
	float ResidentialGrassDensity = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage|Residential")
	float ResidentialBushDensity = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage|Residential")
	float ResidentialTreeDensity = 0.5f;

	// --- Industrial Density ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage|Industrial")
	float IndustrialGrassDensity = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage|Industrial")
	float IndustrialBushDensity = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage|Industrial")
	float IndustrialTreeDensity = 0.15f;

	// --- Nature Density ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage|Nature")
	float NatureGrassDensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage|Nature")
	float NatureBushDensity = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage|Nature")
	float NatureTreeDensity = 1.0f;

	// --- Hillside Density ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage|Hillside")
	float HillsideGrassDensity = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage|Hillside")
	float HillsideBushDensity = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage|Hillside")
	float HillsideTreeDensity = 0.8f;

private:
	void ApplyDensitySettings();
};
