// AlsasuaSaveGame.h (capa GAMEPLAY)
// Datos persistidos de una partida. Puerto de SistemaGuardado. Lo escribe/lee
// UGuardadoSubsystem agrupando el estado de los demás subsistemas + jugador.
// Versión unificada que consolida todos los campos de guardado (Legacy + Manifa).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "AlsasuaLegacySaveGame.generated.h"

static constexpr int32 ALSASUA_SAVE_VERSION = 2;

UCLASS()
class ALSASUAGAMEPLAY_API UAlsasuaLegacySaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	// --- Versión (para migración automática) ---
	UPROPERTY() int32 SaveVersion = ALSASUA_SAVE_VERSION;

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

	// Misiones completadas (consolidado desde UAlsasuaSaveGame).
	UPROPERTY() TArray<FString> CompletedMissionIDs;

	// Jugador.
	UPROPERTY() FVector PlayerPos = FVector::ZeroVector;
	UPROPERTY() FRotator PlayerRot = FRotator::ZeroRotator;
	UPROPERTY() int32 PlayerVida = 100;

	// Reaparición (último piso franco).
	UPROPERTY() bool bRespawn = false;
	UPROPERTY() FVector RespawnPos = FVector::ZeroVector;

	// --- Datos de disfraz (consolidado) ---
	/** 1 = encubierto, 0 = a cara descubierta. Es UDisfrazSubsystem::bEncubierto. */
	UPROPERTY() int32 DisfrazType = 0;
	/**
	 * SIN IMPLEMENTAR: no hay sistema de durabilidad de disfraz. Viene del save
	 * de Manifa y se conserva para no romper los guardados que ya lo tienen
	 * escrito; UGuardadoSubsystem le ponía un 1.0f fijo y nadie lo leía. Lo que
	 * sí existe es el enfriamiento de UDisfrazSubsystem tras delatarte, y ése es
	 * de sesión, no de partida. Si algún día hay durabilidad, éste es su hueco.
	 */
	UPROPERTY() float DisfrazDurability = 1.f;

	// Metadatos.
	UPROPERTY() FDateTime Fecha;
	UPROPERTY() FString SaveSlotName = TEXT("AlsasuaDefaultSlot");
	UPROPERTY() int32 UserIndex = 0;

	// --- Migración automática desde formatos anteriores ---
	bool MigrateFromLegacyV1();

	// Nombre del slot (convierte de formato legacy slot numérico a string).
	static FString NombreSlot(int32 Slot)
	{
		return FString::Printf(TEXT("AlsasuaSlot%d"), Slot);
	}
};
