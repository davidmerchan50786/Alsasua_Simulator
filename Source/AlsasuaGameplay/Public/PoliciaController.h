// PoliciaController.h (capa GAMEPLAY)
// IA de la Policía Foral: patrulla / persigue / ataca con VISIÓN que usa los
// factores de sigilo (día-noche, disfraz). Puerto de PoliciaForalIA (núcleo).
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "PoliciaController.generated.h"

UCLASS()
class ALSASUAGAMEPLAY_API AAlsasuaPoliciaController : public AAIController
{
	GENERATED_BODY()

public:
	AAlsasuaPoliciaController();
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, Category="IA") float RadioVision = 4500.f;   // cm (~45 m)
	UPROPERTY(EditAnywhere, Category="IA") float AnguloVision = 90.f;
	UPROPERTY(EditAnywhere, Category="IA") float RadioAtaque = 1500.f;
	UPROPERTY(EditAnywhere, Category="IA") float Cadencia = 1.0f;
	UPROPERTY(EditAnywhere, Category="IA") int32 Dano = 10;

	UPROPERTY(EditAnywhere, Category="IA") float TiempoBusqueda = 15.f;   // s buscando tras perder de vista

	/** Combat cover: after taking damage, how often to strafe to a new firing position. */
	UPROPERTY(EditAnywhere, Category="IA|Cover") float StrafeInterval = 2.5f;
	/** Health fraction below which the officer starts taking cover. */
	UPROPERTY(EditAnywhere, Category="IA|Cover") float CoverHealthPct = 0.8f;

private:
	enum class EEstado : uint8 { Patrulla, Persigue, Ataca, Busca, Dispersion };
	EEstado Estado = EEstado::Patrulla;
	float TimerAtaque = 0.f;
	float TimerBusqueda = 0.f;
	FVector UltimaPosVista = FVector::ZeroVector;
	float TimerDispersion = 0.f;
	float TimerStrafe = 0.f;
	int32 LastHealth = -1;

	bool VeJugador(APawn*& OutJugador) const;
	void Disparar(APawn* Jugador);
	bool DetectarManifestacion(FVector& OutCentro) const;
	void MaybeTakeCover(APawn* Jugador, float DeltaTime);
};
