// DialogoSubsystem.h (capa GAMEPLAY)
// Ejecuta conversaciones ramificadas: lleva el nodo actual, expone texto/opciones
// y avanza por elección o por línea automática. Puerto del runtime de SistemaDialogo.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DialogoTipos.h"
#include "DialogoSubsystem.generated.h"

// (Hablante, Texto, ¿tiene opciones?)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnNodoMostrado, const FString&, Hablante, const FString&, Texto, bool, bTieneOpciones);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDialogoFin);
// (¿pasó?, lo sacado en el dado, el modificador por apoyo, la dificultad)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnTiradaResuelta, bool, bExito, int32, Dado, int32, Modificador, int32, Dificultad);

UCLASS()
class ALSASUAGAMEPLAY_API UDialogoSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable) FOnNodoMostrado OnNodoMostrado;
	UPROPERTY(BlueprintAssignable) FOnDialogoFin   OnDialogoFin;
	UPROPERTY(BlueprintAssignable) FOnTiradaResuelta OnTiradaResuelta;

	UFUNCTION(BlueprintCallable, Category="Dialogo") void Iniciar(UConversacionDialogo* Conv);

	/**
	 * Carga una conversación de Content/Dialogs/<NombreNPC>.json.
	 *
	 * Ahí había tres árboles de diálogo escritos a mano —Alcalde, Guardia y
	 * Periodista— que no leía nadie: Iniciar() sólo aceptaba un
	 * UConversacionDialogo montado en C++, y el único que se monta es el tutorial
	 * de MisionesSubsystem. Contenido autorado que el juego no podía cargar.
	 *
	 * El hablante sale del nombre del fichero, que es como está organizado: uno
	 * por NPC. Devuelve null si el fichero no está o no tiene nodos.
	 */
	UFUNCTION(BlueprintCallable, Category="Dialogo")
	UConversacionDialogo* CargarConversacion(const FString& NombreNPC);

	/** Carga y arranca de una vez. false si no hay conversación para ese NPC. */
	UFUNCTION(BlueprintCallable, Category="Dialogo")
	bool IniciarConNPC(const FString& NombreNPC);
	UFUNCTION(BlueprintCallable, Category="Dialogo") bool EnCurso() const { return Actual != nullptr; }

	// Texto/opciones del nodo actual.
	UFUNCTION(BlueprintCallable, Category="Dialogo")
	FString HablanteActual() const { return HablanteOverride.IsEmpty() ? (Actual ? Actual->Hablante : FString()) : HablanteOverride; }
	UFUNCTION(BlueprintCallable, Category="Dialogo") FString TextoActual() const { return Actual ? Actual->Texto : FString(); }
	UFUNCTION(BlueprintCallable, Category="Dialogo") TArray<FString> OpcionesActuales() const;

	/** Muestra el nombre real del NPC en vez de la clave del fichero de diálogo
	 *  (p.ej. "Mikel" en lugar de "Rebelde"). Se resetea al cambiar de conversación. */
	UFUNCTION(BlueprintCallable, Category="Dialogo")
	void SetHablanteActual(const FString& NombreReal) { HablanteOverride = NombreReal; }

	/**
	 * Lo mismo, pero diciendo cuáles son tirada, contra qué y con qué
	 * probabilidad ahora mismo. OpcionesActuales() sigue devolviendo sólo los
	 * textos para no romper a quien ya la use.
	 */
	UFUNCTION(BlueprintCallable, Category="Dialogo")
	TArray<FOpcionMostrable> OpcionesDetalladas() const;

	/**
	 * Modificador que el apoyo popular pone en la tirada, de -5 a +5.
	 *
	 * Apoyo va de 0 a 100 y el punto neutro es 50, así que el modificador es
	 * (Apoyo-50)/10. Con las dificultades del dato (10-16) eso deja entre un 25%
	 * y un 55% con el pueblo indiferente, y mueve la aguja de verdad en los
	 * extremos: es el mismo eje que ya conduce la economía y la IA.
	 */
	UFUNCTION(BlueprintPure, Category="Dialogo") int32 ModificadorApoyo() const;

	// Avanza: elige una opción (índice) o continúa la línea automática (Elegir(-1)).
	UFUNCTION(BlueprintCallable, Category="Dialogo") void Elegir(int32 Indice);
	UFUNCTION(BlueprintCallable, Category="Dialogo") void Terminar();

private:
	/** Conversaciones ya leídas de disco, por nombre de NPC. */
	UPROPERTY() TMap<FString, TObjectPtr<UConversacionDialogo>> Cache;

	UPROPERTY() UConversacionDialogo* Conversacion = nullptr;
	const FNodoDialogo* Actual = nullptr;
	/** Nombre real del interlocutor mostrado en el HUD, si es distinto de la
	 *  clave del fichero de diálogo. Vacío = usar el hablante del nodo. */
	UPROPERTY() FString HablanteOverride;
	void IrA(FName Id);

	/** Tiradas ya falladas en esta conversación, "<nodo>:<índice>". Una tirada
	 *  fallida se queda fallada: sin esto se reintenta pulsando el botón otra
	 *  vez, y la dificultad no significa nada. Se vacía al terminar. */
	TSet<FString> TiradasFalladas;

	static FString ClaveTirada(FName Nodo, int32 Indice);
	bool TiradaDisponible(int32 Indice) const;
};
