// MenuPrincipalController.h (capa GAMEPLAY)
// Controlador del nivel de menú principal: navega Nueva partida / Continuar /
// Salir por teclado y abre el nivel de juego. Sin pawn.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MenuPrincipalController.generated.h"

UCLASS()
class ALSASUAGAMEPLAY_API AMenuPrincipalController : public APlayerController
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category="Menu") FName NivelJuego = TEXT("L_Alsasua");

	int32 Seleccion = 0;
	TArray<FString> Opciones() const;

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	void Arriba();
	void Abajo();
	void Activar();
};
