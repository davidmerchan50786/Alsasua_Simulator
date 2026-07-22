// MenuSubsystem.h (capa GAMEPLAY)
// Estado del menú de pausa / opciones. El PlayerController le pasa el input y el
// HUD lo dibuja. Pausa el juego al abrirse. Puerto del menú de SistemaUI.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MenuSubsystem.generated.h"

UENUM()
enum class EPantallaMenu : uint8 { Ninguno, Pausa, Opciones };

UCLASS()
class ALSASUAGAMEPLAY_API UMenuSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	EPantallaMenu Pantalla = EPantallaMenu::Ninguno;
	int32 Seleccion = 0;
	UPROPERTY(EditAnywhere, Category="Menu") FName NivelMenuPrincipal = TEXT("L_MainMenu");

	bool Abierto() const { return Pantalla != EPantallaMenu::Ninguno; }
	TArray<FString> Opciones() const;

	void AlternarPausa(class APlayerController* PC);
	void Cerrar(class APlayerController* PC);
	void Mover(int32 Dir);
	void Activar(class APlayerController* PC);

private:
	int32 CalidadActual = 3;   // 0 Bajo .. 4 Cine
	FString NombreCalidad() const;
	void AplicarCalidad();
};
