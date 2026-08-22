#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaGroundCoverSystem.generated.h"

class UProceduralMeshComponent;

/**
 * Sistema de cobertura del suelo: hojas caídas, piedras, musgo.
 * Genera micro-detalle en el terreno.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GF_WORLD_API UAlsasuaGroundCoverSystem : public UActorComponent
{
	GENERATED_BODY()

public:
	UAlsasuaGroundCoverSystem();

	virtual void BeginPlay() override;

	// --- Fallen Leaves ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|Leaves")
	bool bEnableFallenLeaves = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|Leaves")
	float LeafDensity = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|Leaves")
	float LeafSize = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|Leaves")
	int32 MaxLeaves = 200;

	// --- Rocks ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|Rocks")
	bool bEnableRocks = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|Rocks")
	float RockDensity = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|Rocks")
	float RockMinSize = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|Rocks")
	float RockMaxSize = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|Rocks")
	int32 MaxRocks = 100;

	// --- Moss Patches ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|Moss")
	bool bEnableMossPatches = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|Moss")
	float MossDensity = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|Moss")
	float MossPatchSize = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|Moss")
	int32 MaxMossPatches = 80;

	// --- Pine Needles ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|Needles")
	bool bEnablePineNeedles = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|Needles")
	float NeedleDensity = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|Needles")
	int32 MaxNeedles = 150;

	// --- Colors ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|Colors")
	FLinearColor LeafColor1 = FLinearColor(0.6f, 0.3f, 0.05f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|Colors")
	FLinearColor LeafColor2 = FLinearColor(0.5f, 0.25f, 0.1f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|Colors")
	FLinearColor LeafColor3 = FLinearColor(0.7f, 0.15f, 0.05f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|Colors")
	FLinearColor RockColor = FLinearColor(0.4f, 0.38f, 0.35f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|Colors")
	FLinearColor MossColor = FLinearColor(0.15f, 0.4f, 0.1f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground|Colors")
	FLinearColor NeedleColor = FLinearColor(0.35f, 0.25f, 0.1f);

private:
	void SpawnGroundCover();

	UPROPERTY()
	TObjectPtr<UProceduralMeshComponent> GroundMesh;

	int32 TotalSpawned = 0;
};
