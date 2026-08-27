#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaFoliageWindComponent.generated.h"

/**
 * Adds procedural wind sway to foliage/static meshes.
 * Reads from MPC_Wind_Control and applies phase-offset sway per instance.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALSASUAKERNEL_API UAlsasuaFoliageWindComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAlsasuaFoliageWindComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Wind intensity multiplier for this actor (0 = no sway). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind")
	float WindIntensity = 1.0f;

	/** Sway frequency — larger values = faster oscillation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind")
	float SwayFrequency = 1.5f;

	/** Maximum sway angle in degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind")
	float MaxSwayAngle = 8.0f;

	/** Phase offset to break uniformity between actors. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind")
	float PhaseOffset = 0.0f;

	/** If true, pick random phase from actor location on BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind")
	bool bRandomizePhase = true;

private:
	float CurrentPhase;
	float CurrentSway;
	FVector OriginalRotation;
};
