// ManifestanteController.h (capa GAMEPLAY)
// IA del manifestante: se agolpa cerca del PuntoObjetivo (milling), o huye de él
// si bDispersar. Puerto del comportamiento de multitud de SistemaManifestacion.
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "ManifestanteController.generated.h"

UCLASS()
class ALSASUAGAMEPLAY_API AManifestanteController : public AAIController
{
	GENERATED_BODY()

public:
	AManifestanteController();

	virtual void Tick(float DeltaTime) override;
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

private:
	float Espera = 0.f;
	void Decidir();
};
