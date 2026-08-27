// PeatonController.h (capa GAMEPLAY)
// IA mínima de peatón: deambula por la navmesh a puntos alcanzables aleatorios
// con esperas entre tramos. Puerto del wander civil de SistemaTrafico/NPCBase.
// At high paranoia: patrols like a guard, investigates, reports crimes.
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "PeatonController.generated.h"

UCLASS()
class ALSASUAGAMEPLAY_API AAlsasuaPeatonController : public AAIController
{
	GENERATED_BODY()

public:
	AAlsasuaPeatonController();

	UPROPERTY(EditAnywhere, Category="Peaton") float RadioPaseo = 4000.f;  // 40 m por tramo
	UPROPERTY(EditAnywhere, Category="Peaton") float EsperaMin = 1.f;
	UPROPERTY(EditAnywhere, Category="Peaton") float EsperaMax = 4.f;

	/** Paranoia threshold where civilian starts acting like a guard. */
	UPROPERTY(EditAnywhere, Category="Peaton|Paranoia") float GuardBehaviorThreshold = 60.f;
	/** Additional patrol radius when paranoid (guards cover more ground). */
	UPROPERTY(EditAnywhere, Category="Peaton|Paranoia") float ParanoidPatrolRadius = 6000.f;
	/** Chance per second to report nearby suspicious activity when paranoid. */
	UPROPERTY(EditAnywhere, Category="Peaton|Paranoia") float CrimeReportChance = 0.02f;

	virtual void Tick(float DeltaTime) override;
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

private:
	float Espera = 0.f;
	void NuevoDestino();
	void TickParanoidBehavior(float DeltaTime);
	float GetParanoiaLevel() const;
};
