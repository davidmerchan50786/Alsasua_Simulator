#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "AlsasuaOptimizerSubsystem.generated.h"

UCLASS()
class ALSASUAMANIFA_API UAlsasuaOptimizerSubsystem : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return true; }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UAlsasuaOptimizerSubsystem, STATGROUP_Game); }

	// Distancia a partir de la cual desactivamos el Tick de la IA
	UPROPERTY(EditAnywhere, Category = "Optimization")
	float AICullDistance = 5000.f;

	// Distancia a partir de la cual forzamos el LOD mas grueso + sin sombras
	UPROPERTY(EditAnywhere, Category = "Optimization")
	float RenderLODDistance = 2000.f;

	// Gestor de LOD masivo para la multitud
	void OptimizeCrowd(class AAlsasuaCharacter* Player);

private:
	float OptimizationTickTimer = 0.0f;

	// Cache de NPCs para evitar GetAllActorsOfClass cada tick.
	UPROPERTY()
	TArray<TObjectPtr<ACharacter>> CachedNPCs;

	// Intervalo de refresco del cache de NPCs (segundos).
	float NPCRefreshInterval = 3.0f;

	float NPCRefreshTimer = 0.0f;
};
