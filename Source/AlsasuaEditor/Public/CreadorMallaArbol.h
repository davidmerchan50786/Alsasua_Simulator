// CreadorMallaArbol.h (sólo editor)
// Mallas de árbol por especie para el censo de trees_unity.json (2783 árboles).
//
// UCargadorArboles ya instancia por especie con HISM y tinta la copa, pero
// buscaba mallas en /Game/Meshes/Arboles que nadie generaba, así que los 2783
// árboles del pueblo se plantaban como cilindros del motor.
//
// Todas se construyen a 10 m de alto, que es AlturaReferenciaMalla del
// cargador: éste escala cada instancia por su "altura" del censo (4 a 25 m).
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CreadorMallaArbol.generated.h"

UCLASS()
class ALSASUAEDITOR_API UCreadorMallaArbol : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Genera las diez especies en /Game/Meshes/Arboles. Devuelve cuántas creó. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Arboles")
	static int32 GenerarTodas();
};
