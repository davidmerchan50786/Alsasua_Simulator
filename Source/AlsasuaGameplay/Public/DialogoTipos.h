// DialogoTipos.h (capa GAMEPLAY)
// Datos de diálogo ramificado. Puerto de SistemaDialogo (NodoDialogo /
// OpcionDialogo / ConversacionDialogo) de Unity. UConversacionDialogo es un
// DataAsset para poder autorar conversaciones en el editor o en C++.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DialogoTipos.generated.h"

USTRUCT(BlueprintType)
struct FOpcionDialogo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Texto;
	// Nodo destino al elegir esta opción. NAME_None = termina la conversación.
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Destino;
	// Efectos opcionales sobre apoyo popular al elegir (puede ser negativo).
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float DeltaApoyo = 0.f;

	/**
	 * Opción que hay que ganarse: una tirada contra Dificultad.
	 *
	 * Está en los JSON de Content/Dialogs desde el principio
	 * (bRequiresSkillCheck / DifficultyClass) y el cargador la tiraba a la
	 * basura porque aquí no había dónde meterla. Son las cuatro opciones de
	 * [PERSUASIÓN] e [INTIMIDACIÓN] de los tres NPC, o sea justo las que se
	 * suponía que costaban algo: se elegían y salían siempre bien.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bTirada = false;

	/** Dificultad de la tirada (los datos usan 10-16, escala de d20). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Dificultad = 10;

	/**
	 * Nodo al que se va si la tirada falla. NAME_None = no se va a ninguno: se
	 * queda en el nodo y la opción se gasta, que es lo que se puede hacer con
	 * lo que trae el dato, porque el JSON sólo da un destino por opción.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName DestinoFallo;
};

/** Una opción tal y como hay que enseñarla: con si es tirada y qué falta. */
USTRUCT(BlueprintType)
struct FOpcionMostrable
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FString Texto;
	UPROPERTY(BlueprintReadOnly) bool bTirada = false;
	UPROPERTY(BlueprintReadOnly) int32 Dificultad = 0;
	/** Probabilidad de pasarla ahora mismo, 0-1. Para pintarla en el botón. */
	UPROPERTY(BlueprintReadOnly) float Probabilidad = 1.f;
	/** false si ya se intentó y se falló: el botón va deshabilitado. */
	UPROPERTY(BlueprintReadOnly) bool bDisponible = true;
};

USTRUCT(BlueprintType)
struct FNodoDialogo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Hablante;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(MultiLine=true)) FString Texto;
	// Si vacío -> continúa por Auto (o termina). Si hay opciones, el jugador elige.
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FOpcionDialogo> Opciones;
	// Nodo siguiente cuando no hay opciones (línea automática). NAME_None = fin.
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Auto;
};

UCLASS(BlueprintType)
class ALSASUAGAMEPLAY_API UConversacionDialogo : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Inicio;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FNodoDialogo> Nodos;

	const FNodoDialogo* BuscarNodo(FName Id) const
	{
		for (const FNodoDialogo& N : Nodos) if (N.Id == Id) return &N;
		return nullptr;
	}
};
