#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaNavAvoidance.generated.h"

/** Movimiento AAA: Evitación recíproca de obstáculos (Inspirado en RVO/Detour) */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALSASUAMANIFA_API UAlsasuaNavAvoidance : public UActorComponent
{
	GENERATED_BODY()

public:
	UAlsasuaNavAvoidance();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Radio de seguridad personal
	UPROPERTY(EditAnywhere, Category = "AAA|Navigation")
	float AgentRadius = 60.f;

	// Tiempo de anticipación (segundos hacia el futuro para predecir colisión)
	UPROPERTY(EditAnywhere, Category = "AAA|Navigation")
	float PredictionTime = 1.5f;

	// Fuerza de desviación aplicada para evitar el choque
	FVector GetSteeringAdjustment(const FVector& DesiredVelocity);

private:
	// Escanea el SpatialGrid para encontrar agentes en trayectoria de colisión
	void DetectConflictAgencies(TArray<AActor*>& OutConflicts);
};
