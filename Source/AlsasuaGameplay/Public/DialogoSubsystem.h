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
	UFUNCTION(BlueprintCallable, Category="Dialogo") bool EnCurso() const { return Actual != nullptr; }

	// Texto/opciones del nodo actual.
	UFUNCTION(BlueprintCallable, Category="Dialogo") FString HablanteActual() const { return Actual ? Actual->Hablante : FString(); }
	UFUNCTION(BlueprintCallable, Category="Dialogo") FString TextoActual() const { return Actual ? Actual->Texto : FString(); }
	UFUNCTION(BlueprintCallable, Category="Dialogo") TArray<FString> OpcionesActuales() const;

	// Avanza: elige una opción (índice) o continúa la línea automática (Elegir(-1)).
	UFUNCTION(BlueprintCallable, Category="Dialogo") void Elegir(int32 Indice);
	UFUNCTION(BlueprintCallable, Category="Dialogo") void Terminar();

private:
	UPROPERTY() UConversacionDialogo* Conversacion = nullptr;
	const FNodoDialogo* Actual = nullptr;
	void IrA(FName Id);
};
