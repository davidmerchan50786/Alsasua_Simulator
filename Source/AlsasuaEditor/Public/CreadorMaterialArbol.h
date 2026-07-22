// CreadorMaterialArbol.h (capa EDITOR, sólo editor)
// Genera /Game/Materiales/M_Arbol: material opaco con un parámetro vectorial
// "Color". UCargadorArboles crea una instancia dinámica por especie y le fija
// el tono de copa.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CreadorMaterialArbol.generated.h"

UCLASS()
class UCreadorMaterialArbol : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Vegetacion")
	static bool CrearMaterialArbol();
};
