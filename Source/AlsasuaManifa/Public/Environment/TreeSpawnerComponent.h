#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/DataTable.h"
#include "Dom/JsonObject.h"
#include "TreeSpawnerComponent.generated.h"

USTRUCT(BlueprintType)
struct ALSASUAMANIFA_API FTreePlacementData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree")
	FVector Position = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree")
	float Height = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree")
	float CrownRadius = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree")
	float RotationDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree")
	FString Species;
};

USTRUCT(BlueprintType)
struct ALSASUAMANIFA_API FSpeciesMeshMap
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree")
	FString SpeciesName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree")
	TObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree")
	FVector2D ScaleRange = FVector2D(0.8f, 1.2f);
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALSASUAMANIFA_API UTreeSpawnerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTreeSpawnerComponent();

	UFUNCTION(BlueprintCallable, Category = "Tree Spawner")
	void LoadFromJson(const FString& JsonFilePath);

	UFUNCTION(BlueprintCallable, Category = "Tree Spawner")
	void LoadFromCSV(const FString& CSVFilePath);

	UFUNCTION(BlueprintCallable, Category = "Tree Spawner")
	void SpawnAllTrees();

	UFUNCTION(BlueprintCallable, Category = "Tree Spawner")
	void SpawnTreeSpecies(const FString& Species);

	UFUNCTION(BlueprintCallable, Category = "Tree Spawner")
	void ClearAllTrees();

	UFUNCTION(BlueprintCallable, Category = "Tree Spawner")
	void ClearTreeSpecies(const FString& Species);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree Spawner")
	TMap<FString, FSpeciesMeshMap> SpeciesMeshMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree Spawner")
	FTransform OriginOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree Spawner")
	float HeightOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree Spawner")
	bool bRandomizeRotation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree Spawner")
	bool bRandomizeScale = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree Spawner")
	bool bApplyWorldOffset = true;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void CreateHISMForSpecies(const FString& Species);
	UHierarchicalInstancedStaticMeshComponent* GetOrCreateHISM(const FString& Species, UStaticMesh* Mesh);
	void PlaceTreeInstances(const FString& Species, const TArray<FTreePlacementData>& Trees);

	UPROPERTY()
	TMap<FString, TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> HISMComponents;

	TMap<FString, TArray<FTreePlacementData>> TreeData;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> HISMHolders;

	bool ParseJsonFile(const FString& FilePath);
	bool ParseCSVFile(const FString& FilePath);
};
