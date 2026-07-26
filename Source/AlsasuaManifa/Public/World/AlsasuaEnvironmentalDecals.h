#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaEnvironmentalDecals.generated.h"

class UDecalComponent;

/**
 * Sistema de decals ambientales: grafitis, carteles, basura en suelo.
 * Añade storytelling visual al mundo urbano.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALSASUAMANIFA_API UAlsasuaEnvironmentalDecals : public UActorComponent
{
	GENERATED_BODY()

public:
	UAlsasuaEnvironmentalDecals();

	virtual void BeginPlay() override;

	// --- Graffiti ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnvDecal|Graffiti")
	bool bEnableGraffiti = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnvDecal|Graffiti")
	float GraffitiProbability = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnvDecal|Graffiti")
	float GraffitiSize = 150.f;

	// --- Posters ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnvDecal|Posters")
	bool bEnablePosters = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnvDecal|Posters")
	float PosterProbability = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnvDecal|Posters")
	float PosterSize = 60.f;

	// --- Ground Trash ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnvDecal|Trash")
	bool bEnableTrash = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnvDecal|Trash")
	float TrashProbability = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnvDecal|Trash")
	float TrashSize = 30.f;

	// --- Cracks ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnvDecal|Cracks")
	bool bEnableWallCracks = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnvDecal|Cracks")
	float WallCrackProbability = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnvDecal|Cracks")
	float WallCrackSize = 200.f;

	// --- Moss ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnvDecal|Moss")
	bool bEnableMoss = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnvDecal|Moss")
	float MossProbability = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnvDecal|Moss")
	float MossSize = 300.f;

private:
	void SpawnEnvironmentalDecals();

	UPROPERTY()
	TArray<TObjectPtr<UDecalComponent>> SpawnedDecals;
};
