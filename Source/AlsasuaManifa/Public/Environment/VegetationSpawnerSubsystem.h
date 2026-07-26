#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/DataTable.h"
#include "VegetationType.h"
#include "VegetationSpawnerSubsystem.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class AActor;
class ALandscape;

USTRUCT(BlueprintType)
struct ALSASUAMANIFA_API FCellCollisionData
{
	GENERATED_BODY()

	FBox Bounds;
	TArray<bool> SubCells; // true = occupied
	int32 SubDivisions = 4;
	float CellSize = 64.0f;
};

UCLASS()
class ALSASUAMANIFA_API UVegetationSpawnerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Vegetation Spawner")
	void SetTargetLandscape(ALandscape* InLandscape);

	UFUNCTION(BlueprintCallable, Category = "Vegetation Spawner")
	void SpawnAllVegetation();

	UFUNCTION(BlueprintCallable, Category = "Vegetation Spawner")
	void SpawnVegetationType(UVegetationType* Vegetation);

	UFUNCTION(BlueprintCallable, Category = "Vegetation Spawner")
	void ClearAllVegetation();

	UFUNCTION(BlueprintCallable, Category = "Vegetation Spawner")
	TArray<FTransform> GeneratePoissonDiscPoints(UVegetationType* Vegetation, const FBox& RegionBounds);

	UFUNCTION(BlueprintCallable, Category = "Vegetation Spawner")
	void RebuildCollisionCache();

	UFUNCTION(BlueprintCallable, Category = "Vegetation Spawner")
	TArray<FTransform> GetSpawnTransforms(UVegetationType* Vegetation, const FBox& RegionBounds);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation Spawner")
	TArray<TObjectPtr<UVegetationType>> VegetationTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation Spawner")
	int32 GlobalSeed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation Spawner")
	float CellSize = 64.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation Spawner")
	int32 CellDivisions = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation Spawner")
	bool bHighPrecisionCollision = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation Spawner")
	float WaterHeight = 0.0f;

protected:
	void SpawnInstances(UVegetationType* Vegetation, const TArray<FTransform>& Transforms);

	void SampleLandscapeHeight(const FVector& WorldPos, float& OutHeight, float& OutWorldHeight, float& OutNormalizedHeight);
	float SampleLandscapeSlope(const FVector& WorldPos);
	float SampleLandscapeConvexity(const FVector& WorldPos, float Radius = 3.0f);
	FVector2D GetNormalizedPosition(const FVector& WorldPos);

	bool IsInsideOccupiedCell(const FVector& WorldPos);
	bool TestSplatmap(const FVector2D& NormalizedPos, UVegetationType* Vegetation, float& OutSpawnChance);

	static int32 GetSplatmapID(int32 LayerID);

	TWeakObjectPtr<ALandscape> TargetLandscape;

	TMap<TWeakObjectPtr<ALandscape>, TArray<FCellCollisionData>> LandscapeCells;

	int32 GetPrefabIndex(UVegetationType* Vegetation, float RandomValue);

private:
	static constexpr int32 PoissonMaxAttempts = 10;
	static constexpr int32 PoissonDimensions = 2;
};
