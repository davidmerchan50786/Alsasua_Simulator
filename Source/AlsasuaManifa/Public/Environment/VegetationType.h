#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/StaticMesh.h"
#include "VegetationType.generated.h"

USTRUCT(BlueprintType)
struct ALSASUAMANIFA_API FVegetationPrefab
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation")
	TObjectPtr<UStaticMesh> Mesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float Probability = 100.0f;
};

USTRUCT(BlueprintType)
struct ALSASUAMANIFA_API FVegetationLayerMask
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation")
	FName LayerName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation")
	int32 LayerIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Threshold = 0.0f;
};

UCLASS(BlueprintType, DefaultToInstanced, EditInlineNew)
class ALSASUAMANIFA_API UVegetationType : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation")
	FString TypeName = TEXT("VegetationItem");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation")
	int32 Seed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation|Prefabs")
	TArray<FVegetationPrefab> Prefabs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation|Spawning", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float GlobalProbability = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation|Spawning")
	float DensityPerM2 = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation|Spawning")
	float MinDistance = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation|Spawning")
	FVector2D HeightRange = FVector2D(0.0f, 1000.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation|Spawning")
	FVector2D SlopeRange = FVector2D(0.0f, 60.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation|Spawning")
	FVector2D ScaleRange = FVector2D(0.8f, 1.2f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation|Spawning")
	bool bRandomRotation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation|Spawning")
	float SinkAmount = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation|Spawning")
	bool bCollisionCheck = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation|Spawning")
	bool bRejectUnderwater = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation|Spawning")
	float WaterHeight = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation|Biome")
	TArray<FString> BiomeAffinity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation|LayerMasks")
	TArray<FVegetationLayerMask> LayerMasks;

	/** Contador de instancias colocadas en la última pasada de spawn (runtime, no serializado). */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Vegetation|Runtime")
	int32 InstanceCount = 0;

	/** Selecciona un prefab de la lista ponderado por su Probability. Devuelve nullptr si no hay prefabs válidos. */
	const FVegetationPrefab* SelectWeightedPrefab(const FRandomStream& RandomStream) const;
};
