// AlsasuaSaveGame.h (capa GAMEPLAY)
// Datos persistidos de una partida. Puerto de SistemaGuardado. Lo escribe/lee
// UGuardadoSubsystem agrupando el estado de los demás subsistemas + jugador.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "AlsasuaLegacySaveGame.generated.h"

UCLASS()
class ALSASUAGAMEPLAY_API UAlsasuaLegacySaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY() bool bValido = false;

	// Economía / reputación / búsqueda.
	UPROPERTY() int32 Dinero = 0;
	UPROPERTY() int32 Puntuacion = 0;
	UPROPERTY() float Apoyo = 50.f;
	UPROPERTY() int32 NivelBusqueda = 0;

	// Mundo.
	UPROPERTY() float Hora = 12.f;

	// Misión activa (se reanuda al cargar).
	UPROPERTY() FName MisionActual;

	// Jugador.
	UPROPERTY() FVector PlayerPos = FVector::ZeroVector;
	UPROPERTY() FRotator PlayerRot = FRotator::ZeroRotator;
	UPROPERTY() int32 PlayerVida = 100;

	// Reaparición (último piso franco).
	UPROPERTY() bool bRespawn = false;
	UPROPERTY() FVector RespawnPos = FVector::ZeroVector;

	// Metadatos.
	UPROPERTY() FDateTime Fecha;
};
