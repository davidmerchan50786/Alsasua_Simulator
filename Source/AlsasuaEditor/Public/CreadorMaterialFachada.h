// CreadorMaterialFachada.h (capa EDITOR, sólo editor)
// Genera /Game/Materiales/M_Fachada: material de edificio con ventanas
// procedurales (rejilla por UV) que se encienden de noche (escalar Night del
// MPC_Clima), gateadas a las caras verticales. Hereda el vertex-color + mojado.
// Ejecuta CrearMaterialEdificio() antes (crea el MPC_Clima).
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CreadorMaterialFachada.generated.h"

UCLASS()
class UCreadorMaterialFachada : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Edificios")
	static bool CrearMaterialFachada();
};
