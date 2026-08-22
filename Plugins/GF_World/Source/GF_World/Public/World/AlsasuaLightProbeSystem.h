#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaLightProbeSystem.generated.h"

class USphereComponent;

/**
 * Sistema de probes de luz para interiores.
 * Coloca probes que capturan iluminación ambiental para interiors.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GF_WORLD_API UAlsasuaLightProbeSystem : public UActorComponent
{
	GENERATED_BODY()

public:
	UAlsasuaLightProbeSystem();

	virtual void BeginPlay() override;

	// --- Probe Configuration ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightProbe|Config")
	float ProbeSpacing = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightProbe|Config")
	float ProbeHeight = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightProbe|Config")
	float ProbeRadius = 300.f;

	// --- Light Properties ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightProbe|Light")
	float ProbeIntensity = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightProbe|Light")
	FLinearColor ProbeColorWarm = FLinearColor(1.f, 0.9f, 0.7f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightProbe|Light")
	FLinearColor ProbeColorCool = FLinearColor(0.7f, 0.8f, 1.f);

	// --- Bounce ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightProbe|Bounce")
	float BounceIntensity = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightProbe|Bounce")
	FLinearColor BounceColor = FLinearColor(0.8f, 0.85f, 0.9f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightProbe|Bounce")
	float BounceRadius = 500.f;

private:
	void SpawnLightProbes();

	UPROPERTY()
	TArray<TObjectPtr<USphereComponent>> SpawnedProbes;
};
