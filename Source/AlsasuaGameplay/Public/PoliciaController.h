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

	UPROPERTY(EditAnywhere, Category="IA") float RadioVision = 2200.f;   // cm (~22 m)
	UPROPERTY(EditAnywhere, Category="IA") float AnguloVision = 90.f;
	UPROPERTY(EditAnywhere, Category="IA") float RadioAtaque = 1500.f;
	UPROPERTY(EditAnywhere, Category="IA") float Cadencia = 1.2f;
	UPROPERTY(EditAnywhere, Category="IA") int32 Dano = 8;

	UPROPERTY(EditAnywhere, Category="IA") float TiempoBusqueda = 8.f;   // s buscando tras perder de vista

private:
	enum class EEstado : uint8 { Patrulla, Persigue, Ataca, Busca };
	EEstado Estado = EEstado::Patrulla;
	float TimerAtaque = 0.f;
	float TimerBusqueda = 0.f;
	FVector UltimaPosVista = FVector::ZeroVector;

	bool VeJugador(APawn*& OutJugador) const;
	void Disparar(APawn* Jugador);
};
