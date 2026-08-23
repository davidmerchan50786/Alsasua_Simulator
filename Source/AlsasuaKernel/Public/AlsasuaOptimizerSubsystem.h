#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "AlsasuaOptimizerSubsystem.generated.h"

UCLASS()
class ALSASUAKERNEL_API UAlsasuaOptimizerSubsystem : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return !IsTemplate(); }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UAlsasuaOptimizerSubsystem, STATGROUP_Game); }

	// LOD band distances (meters)
	UPROPERTY(EditAnywhere, Category="LOD|Distances")
	float LOD1Distance = 3000.f;

	UPROPERTY(EditAnywhere, Category="LOD|Distances")
	float LOD2Distance = 6000.f;

	UPROPERTY(EditAnywhere, Category="LOD|Distances")
	float LOD3Distance = 10000.f;

	// Tick cull distance
	UPROPERTY(EditAnywhere, Category="LOD|Distances")
	float AICullDistance = 15000.f;

	void OptimizeCrowd(class AAlsasuaCharacter* Player);

private:
	float OptimizationTickTimer = 0.0f;
	float NPCRefreshTimer = 0.0f;
	float NPCRefreshInterval = 3.0f;

	UPROPERTY() TArray<TObjectPtr<ACharacter>> CachedNPCs;

	void ApplyLOD(ACharacter* NPC, int32 LODLevel);
};
