// PeatonController.h (capa GAMEPLAY)
// IA mínima de peatón: deambula por la navmesh a puntos alcanzables aleatorios
// con esperas entre tramos. Puerto del wander civil de SistemaTrafico/NPCBase.
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

	virtual void Tick(float DeltaTime) override;
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

private:
	float Espera = 0.f;
	void NuevoDestino();
};
