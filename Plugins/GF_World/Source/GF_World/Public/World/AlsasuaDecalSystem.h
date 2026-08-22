#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaDecalSystem.generated.h"

class UDecalComponent;
class UMaterialInterface;

/**
 * Sistema de decals para carreteras: marcas, grietas, charcos, desgaste.
 * Se añade a actores de carretera y genera decals procedurales.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GF_WORLD_API UAlsasuaDecalSystem : public UActorComponent
{
	GENERATED_BODY()

public:
	UAlsasuaDecalSystem();

	virtual void BeginPlay() override;

	// --- Road Markings ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decal|Road")
	TObjectPtr<UMaterialInterface> RoadMarkingMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decal|Road")
	float RoadMarkingWidth = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decal|Road")
	float RoadMarkingLength = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decal|Road")
	FLinearColor RoadMarkingColor = FLinearColor(1.f, 1.f, 1.f);

	// --- Puddles ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decal|Puddle")
	TObjectPtr<UMaterialInterface> PuddleDecalMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decal|Puddle")
	float PuddleMinRadius = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decal|Puddle")
	float PuddleMaxRadius = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decal|Puddle")
	int32 MaxPuddlesPerRoad = 5;

	// --- Cracks ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decal|Crack")
	TObjectPtr<UMaterialInterface> CrackDecalMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decal|Crack")
	int32 MaxCracksPerRoad = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decal|Crack")
	float CrackScale = 1.5f;

	// --- Wear ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decal|Wear")
	TObjectPtr<UMaterialInterface> WearDecalMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decal|Wear")
	float WearProbability = 0.3f;

private:
	void SpawnRoadMarkings();
	void SpawnPuddles();
	void SpawnCracks();
	void SpawnWear();

	UPROPERTY()
	TArray<TObjectPtr<UDecalComponent>> SpawnedDecals;
};
