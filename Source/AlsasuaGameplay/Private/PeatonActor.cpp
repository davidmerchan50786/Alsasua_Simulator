// PeatonActor.cpp
#include "PeatonActor.h"
#include "PeatonController.h"

APeatonActor::APeatonActor()
{
	bEsPolicia = false;
	Vida = 100;
	VidaMaxima = 100;

	AIControllerClass = AAlsasuaPeatonController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}
