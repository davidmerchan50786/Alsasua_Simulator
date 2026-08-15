// CreadorMallaLandmark.h (sólo editor)
// Mallas de los edificios singulares de landmarks_real.json.
//
// Son 19 landmarks de 15 tipos distintos (iglesias, frontones, ayuntamiento,
// ikastola, estación, mercado...) y todos se colocaban como el mismo cubo gris
// de 4x4x5 m: el campo "tipo" del JSON se leía y no se usaba para nada.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CreadorMallaLandmark.generated.h"

UCLASS()
class ALSASUAEDITOR_API UCreadorMallaLandmark : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Genera todos los arquetipos en /Game/Landmarks. Devuelve cuántos creó. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Landmarks")
	static int32 GenerarTodos();

	/** Nave con torre campanario y tejado a dos aguas. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Landmarks")
	static bool GenerarIglesia();

	/** Frontón de pelota: frontis, pared izquierda y cancha. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Landmarks")
	static bool GenerarFronton();

	/** Casa municipal con arkupe de tres ojos, como la de la Plaza de los Fueros. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Landmarks")
	static bool GenerarAyuntamiento();

	/** Bloque escolar de tres plantas en U. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Landmarks")
	static bool GenerarEscuela();

	/** Nave de cubierta curva: polideportivo y mercado. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Landmarks")
	static bool GenerarNave();

	/** Estación con edificio de viajeros y marquesina de andén. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Landmarks")
	static bool GenerarEstacion();

	/** Bloque civil de dos plantas: juzgado, centro de salud, biblioteca... */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Landmarks")
	static bool GenerarBloqueCivico();
};
