#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "VegetationType.h"
#include "BiomeDefinition.generated.h"

USTRUCT(BlueprintType)
struct GF_VEGETACION_API FBiomeVegetationEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
	TObjectPtr<UVegetationType> Vegetation = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome", meta = (ClampMin = "0.0"))
	float Weight = 1.0f;
};

UCLASS(BlueprintType)
class GF_VEGETACION_API UBiomeDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Biome")
	FName BiomeName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Biome|Debug")
	FLinearColor DebugColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome|Climate", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinTemperature = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome|Climate", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxTemperature = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome|Climate", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinMoisture = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome|Climate", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxMoisture = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome|Climate", meta = (ClampMin = "0.0"))
	float MinAltitude = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome|Climate", meta = (ClampMin = "0.0"))
	float MaxAltitude = 100000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome|Climate", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float MinSlope = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome|Climate", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float MaxSlope = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome|Vegetation")
	TArray<FBiomeVegetationEntry> VegetationEntries;

	bool Matches(float Temperature, float Moisture, float AltitudeCm, float SlopeDegrees) const
	{
		return Temperature >= MinTemperature && Temperature <= MaxTemperature
			&& Moisture >= MinMoisture && Moisture <= MaxMoisture
			&& AltitudeCm >= MinAltitude && AltitudeCm <= MaxAltitude
			&& SlopeDegrees >= MinSlope && SlopeDegrees <= MaxSlope;
	}
};
