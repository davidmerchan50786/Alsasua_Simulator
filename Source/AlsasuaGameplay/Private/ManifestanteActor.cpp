// ManifestanteActor.cpp
#include "ManifestanteActor.h"
#include "ManifestanteController.h"

AManifestanteActor::AManifestanteActor()
{
	bEsPolicia = false;
	Vida = 100;
	VidaMaxima = 100;
	AIControllerClass = AManifestanteController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}
