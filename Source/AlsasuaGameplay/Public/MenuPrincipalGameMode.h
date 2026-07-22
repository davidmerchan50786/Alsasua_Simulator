// MenuPrincipalGameMode.h (capa GAMEPLAY)
// GameMode del nivel de menú principal: sin pawn, controlador de menú. El HUD se
// asigna por DefaultEngine.ini (la UI no la puede referenciar Gameplay).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MenuPrincipalGameMode.generated.h"

UCLASS()
class ALSASUAGAMEPLAY_API AMenuPrincipalGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMenuPrincipalGameMode();
};
