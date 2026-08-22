#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "BiomeDefinition.h"
#include "BiomeSubsystem.generated.h"

class ALandscape;
class UWorld;

UCLASS()
class GF_VEGETACION_API UBiomeSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	UFUNCTION(BlueprintCallable, Category = "Biome")
	void SetLandscape(ALandscape* InLandscape);

	UFUNCTION(BlueprintCallable, Category = "Biome")
	UBiomeDefinition* GetBiomeAtLocation(const FVector& WorldPos) const;

	UFUNCTION(BlueprintPure, Category = "Biome")
	float GetTemperatureAtLocation(const FVector& WorldPos) const;

	UFUNCTION(BlueprintPure, Category = "Biome")
	float GetMoistureAtLocation(const FVector& WorldPos) const;

	UFUNCTION(BlueprintCallable, Category = "Biome|Debug", meta = (ClampMin = "1"))
	TArray<UBiomeDefinition*> SampleBiomeGrid(const FVector& Center, FVector2D Extent, int32 Resolution, int32& OutGridSizeX, int32& OutGridSizeY) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
	TArray<TObjectPtr<UBiomeDefinition>> Biomes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome|Noise", meta = (ClampMin = "0.0000001"))
	float MoistureNoiseScale = 0.00005f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome|Water")
	float WaterHeight = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome|Water", meta = (ClampMin = "0.0"))
	float WaterInfluenceDistance = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome|Water", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WaterMoistureBonus = 0.35f;

protected:
	void DiscoverLandscape() const;
	bool SampleTerrain(const FVector& WorldPos, float& OutHeightCm, float& OutSlopeDegrees) const;

	mutable TWeakObjectPtr<ALandscape> Landscape;
	mutable bool bLandscapeDiscovered = false;
};
