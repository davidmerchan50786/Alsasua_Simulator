// CreadorMaterialEdificio.h (capa EDITOR, sólo editor)
// Genera /Game/Materiales/M_Edificio: material opaco que usa el color de vértice
// como base (arenisca/teja por edificio). Lo aplica UCargadorEdificios.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CreadorMaterialEdificio.generated.h"

UCLASS()
class UCreadorMaterialEdificio : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Edificios")
	static bool CrearMaterialEdificio();
};
