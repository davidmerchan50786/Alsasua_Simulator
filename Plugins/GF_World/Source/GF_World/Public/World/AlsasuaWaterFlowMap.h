#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaWaterFlowMap.generated.h"

/**
 * Genera flow map procedural para el río Arakil.
 * Anima la dirección del agua basándose en la geometría del cauce.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GF_WORLD_API UAlsasuaWaterFlowMap : public UActorComponent
{
	GENERATED_BODY()

public:
	UAlsasuaWaterFlowMap();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// --- Flow Parameters ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow|Speed")
	float BaseFlowSpeed = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow|Speed")
	float FastFlowSpeed = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow|Speed")
	float SlowFlowSpeed = 0.2f;

	// --- Flow Direction ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow|Direction")
	FVector FlowDirectionBase = FVector(1.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow|Direction")
	float DirectionVariation = 0.3f;

	// --- Ripple ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow|Ripple")
	float RippleFrequency = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow|Ripple")
	float RippleAmplitude = 0.1f;

	// --- Foam ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow|Foam")
	float FoamSpeedThreshold = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow|Foam")
	float FoamIntensity = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow|Foam")
	float FoamSpawnRate = 10.f;

	// --- Depth ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow|Depth")
	float ShallowDepth = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow|Depth")
	float DeepDepth = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow|Depth")
	float DepthFadeSpeed = 0.3f;

private:
	void UpdateFlowMap(float DeltaTime);

	float TimeAccum = 0.f;
};
