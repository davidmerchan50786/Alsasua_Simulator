// PoliciaActor.cpp
#include "PoliciaActor.h"
#include "PoliciaController.h"

APoliciaActor::APoliciaActor()
{
	bEsPolicia = true;
	VidaMaxima = 120;
	Vida = 120;

	AIControllerClass = AAlsasuaPoliciaController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}
