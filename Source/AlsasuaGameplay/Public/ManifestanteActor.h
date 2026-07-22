// ManifestanteActor.h (capa GAMEPLAY)
// Manifestante: civil (Entities) con IA de concentración/marcha/dispersión.
// El subsistema de manifestación actualiza PuntoObjetivo y bDispersar; la IA
// solo los lee. Puerto de la multitud de SistemaManifestacion.
#pragma once

#include "CoreMinimal.h"
#include "AlsasuaNPC.h"
#include "ManifestanteActor.generated.h"

UCLASS()
class ALSASUAGAMEPLAY_API AManifestanteActor : public AAlsasuaNPC
{
	GENERATED_BODY()

public:
	AManifestanteActor();

	// Centro al que se agolpa (concentración o cabeza de marcha).
	UPROPERTY(BlueprintReadWrite, Category="Manifestacion") FVector PuntoObjetivo = FVector::ZeroVector;
	// Radio de movimiento alrededor del punto.
	UPROPERTY(EditAnywhere, Category="Manifestacion") float RadioMilling = 500.f;
	// Si true, huye del punto (carga policial / disolución).
	UPROPERTY(BlueprintReadWrite, Category="Manifestacion") bool bDispersar = false;
};
