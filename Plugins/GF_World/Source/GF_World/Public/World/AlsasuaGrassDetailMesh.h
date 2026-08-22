#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaGrassDetailMesh.generated.h"

/**
 * Genera hierba procedural detallada con reacción al viento.
 * Crea meshes de hierba triangulares que responden al MPC Wind.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GF_WORLD_API UAlsasuaGrassDetailMesh : public UActorComponent
{
	GENERATED_BODY()

public:
	UAlsasuaGrassDetailMesh();

	virtual void BeginPlay() override;

	// --- Grass Parameters ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grass|Density")
	float GrassDensity = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grass|Size")
	float GrassBladeWidth = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grass|Size")
	float GrassBladeHeight = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grass|Size")
	float GrassHeightVariation = 0.4f;

	// --- Colors ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grass|Color")
	FLinearColor GrassBaseColor = FLinearColor(0.2f, 0.55f, 0.1f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grass|Color")
	FLinearColor GrassTipColor = FLinearColor(0.35f, 0.7f, 0.2f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grass|Color")
	float ColorVariation = 0.15f;

	// --- Distribution ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grass|Dist")
	float MinSpawnRadius = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grass|Dist")
	float MaxSpawnRadius = 3000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grass|Dist")
	float MaxSlopeAngle = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grass|Dist")
	float MinAltitude = 50000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grass|Dist")
	float MaxAltitude = 56000.f;

private:
	void SpawnGrassPatch();

	int32 SpawnedBlades = 0;
};
