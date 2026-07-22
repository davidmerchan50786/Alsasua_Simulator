// AlsasuaPlayerController.h (capa GAMEPLAY)
// Pega el gameplay al pawn (Entities): le añade el UArmasComponent y cablea el
// input de combate. Hacerlo aquí (no en el personaje) respeta las capas
// (Entities no puede depender de Gameplay).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AlsasuaLegacyPlayerController.generated.h"

class UArmasComponent;

UCLASS()
class ALSASUAGAMEPLAY_API AAlsasuaLegacyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, Category="Respawn") float RetardoRespawn = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Raton", meta = (ClampMin = "0.05"))
	float MouseSensitivityX = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Raton", meta = (ClampMin = "0.05"))
	float MouseSensitivityY = 1.0f;

	void SalirVehiculo(class AVehiculoJugable* V);

	UFUNCTION(BlueprintCallable, Category = "Raton")
	void SetMouseSensitivity(float X, float Y);
	UFUNCTION(BlueprintCallable, Category = "Juego")
	void TogglePause();

private:
	UPROPERTY() UArmasComponent* Armas = nullptr;
	bool bEsperandoRespawn = false;
	float TimerRespawn = 0.f;
	bool bCongeladoArranque = false;
	bool bJuegoEnPausa = false;
	void ComprobarMuerte(float DeltaTime);
	void GestionarCongelado();

	void OnDisparar();
	void EquiparPunos();
	void EquiparPistola();
	void EquiparEscopeta();
	void EquiparFusil();

	// Consumo de sustancias (teclas 5-8)
	void TomarPorro();
	void TomarSpeed();
	void TomarChute();
	void TomarTripi();

	void AlternarDisfraz();   // tecla H
	void OnInteractuar();     // tecla E (avanza diálogo / interactúa)
	void OnConvocarManifestacion();   // tecla M
	// Vehículos
	void EntrarVehiculoCercano();
	void EntrarEn(class AVehiculoJugable* V);
	UPROPERTY() APawn* PersonajePawn = nullptr;   // el personaje a pie mientras conduces

	void OnGuardar();         // F5 guardado rápido
	void OnCargar();          // F9 carga rápida
	void OnMenu();            // Escape (abre/cierra pausa)
	void OnMenuArriba();
	void OnMenuAbajo();
	void OnMenuActivar();     // Enter
};
