// MenuPrincipalGameMode.cpp
#include "MenuPrincipalGameMode.h"
#include "MenuPrincipalController.h"
#include "GameFramework/SpectatorPawn.h"

AMenuPrincipalGameMode::AMenuPrincipalGameMode()
{
	PlayerControllerClass = AMenuPrincipalController::StaticClass();
	DefaultPawnClass = ASpectatorPawn::StaticClass();   // sin personaje en el menú
}
