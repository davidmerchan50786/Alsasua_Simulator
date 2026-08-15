// CreadorMallaMobiliario.h (sólo editor)
// Genera las mallas del mobiliario urbano que street_furniture.json coloca.
//
// Las piezas se componen de cajas: son bancos, papeleras y bolardos, no
// esculpidos. El color va por vértice porque M_Mobiliario y M_Metal lo usan de
// tinte sobre la veta de madera y la chapa.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CreadorMallaMobiliario.generated.h"

UCLASS()
class ALSASUAEDITOR_API UCreadorMallaMobiliario : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Genera las ocho mallas en /Game/Mobiliario. Devuelve cuántas creó. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Mobiliario")
	static int32 GenerarTodas();

	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Mobiliario")
	static bool GenerarBanco();

	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Mobiliario")
	static bool GenerarPapelera();

	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Mobiliario")
	static bool GenerarBolardo();

	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Mobiliario")
	static bool GenerarMaceta();

	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Mobiliario")
	static bool GenerarBocaIncendio();

	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Mobiliario")
	static bool GenerarTapaAlcantarilla();

	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Mobiliario")
	static bool GenerarBuzonCorreos();

	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Mobiliario")
	static bool GenerarParadaBus();
};
