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

UCLASS()
class ALSASUAGAMEPLAY_API UDialogoSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable) FOnNodoMostrado OnNodoMostrado;
	UPROPERTY(BlueprintAssignable) FOnDialogoFin   OnDialogoFin;

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
	UFUNCTION(BlueprintCallable, Category="Dialogo") FString HablanteActual() const { return Actual ? Actual->Hablante : FString(); }
	UFUNCTION(BlueprintCallable, Category="Dialogo") FString TextoActual() const { return Actual ? Actual->Texto : FString(); }
	UFUNCTION(BlueprintCallable, Category="Dialogo") TArray<FString> OpcionesActuales() const;

	// Avanza: elige una opción (índice) o continúa la línea automática (Elegir(-1)).
	UFUNCTION(BlueprintCallable, Category="Dialogo") void Elegir(int32 Indice);
	UFUNCTION(BlueprintCallable, Category="Dialogo") void Terminar();

private:
	/** Conversaciones ya leídas de disco, por nombre de NPC. */
	UPROPERTY() TMap<FString, TObjectPtr<UConversacionDialogo>> Cache;

	UPROPERTY() UConversacionDialogo* Conversacion = nullptr;
	const FNodoDialogo* Actual = nullptr;
	void IrA(FName Id);
};
