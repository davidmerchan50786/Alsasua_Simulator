#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaRoadDecalSystem.generated.h"

class UDecalComponent;

/**
 * Sistema de decals de carretera: marcas, cruces, flechas, topes.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GF_CARRETERAS_API UAlsasuaRoadDecalSystem : public UActorComponent
{
	GENERATED_BODY()

public:
	UAlsasuaRoadDecalSystem();

	virtual void BeginPlay() override;

	// --- Lane Markings ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road|Lane")
	bool bEnableLaneMarkings = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road|Lane")
	float LaneWidth = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road|Lane")
	float LaneMarkingLength = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road|Lane")
	float LaneMarkingGap = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road|Lane")
	float LaneMarkingWidth = 15.f;

	// --- Crosswalks ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road|Crosswalk")
	bool bEnableCrosswalks = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road|Crosswalk")
	float CrosswalkProbability = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road|Crosswalk")
	float CrosswalkWidth = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road|Crosswalk")
	float CrosswalkStripeWidth = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road|Crosswalk")
	float CrosswalkStripeGap = 30.f;

	// --- Speed Bumps ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road|SpeedBump")
	bool bEnableSpeedBumps = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road|SpeedBump")
	float SpeedBumpProbability = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road|SpeedBump")
	float SpeedBumpWidth = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road|SpeedBump")
	float SpeedBumpHeight = 15.f;

	// --- Direction Arrows ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road|Arrows")
	bool bEnableDirectionArrows = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road|Arrows")
	float ArrowProbability = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road|Arrows")
	float ArrowSize = 80.f;

	// --- Stop Lines ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road|StopLine")
	bool bEnableStopLines = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road|StopLine")
	float StopLineProbability = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road|StopLine")
	float StopLineWidth = 300.f;

	// --- Road Edge ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road|Edge")
	bool bEnableRoadEdge = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road|Edge")
	float RoadEdgeWidth = 10.f;

	// --- Colors ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road|Colors")
	FLinearColor LaneMarkingColor = FLinearColor(1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road|Colors")
	FLinearColor CrosswalkColor = FLinearColor(1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road|Colors")
	FLinearColor SpeedBumpColor = FLinearColor(0.9f, 0.8f, 0.1f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road|Colors")
	FLinearColor ArrowColor = FLinearColor(1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road|Colors")
	FLinearColor StopLineColor = FLinearColor(1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road|Colors")
	FLinearColor RoadEdgeColor = FLinearColor(1.f, 1.f, 1.f);

private:
	void SpawnRoadDecals();

	UPROPERTY()
	TArray<TObjectPtr<UDecalComponent>> SpawnedDecals;
};
