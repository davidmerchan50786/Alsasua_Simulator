// MenuPrincipalHUD.h (capa UI)
// HUD del menú principal por Canvas: título + opciones leídas del
// AMenuPrincipalController. Se asigna por DefaultEngine.ini al GameMode del menú.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MenuPrincipalHUD.generated.h"

UCLASS()
class ALSASUAUI_API AMenuPrincipalHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
};
