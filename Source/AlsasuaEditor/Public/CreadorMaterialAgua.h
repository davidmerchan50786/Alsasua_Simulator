// CreadorMaterialAgua.h (capa EDITOR, sólo editor)
// Genera el material de agua /Game/Materiales/M_AguaRio que UCargadorVias aplica
// a los ríos. Material translúcido, muy liso y especular (el aspecto de agua lo
// dan los reflejos Lumen). Ejecutar una vez desde un Editor Utility / Blueprint.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CreadorMaterialAgua.generated.h"

UCLASS()
class UCreadorMaterialAgua : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Crea (o sobrescribe) /Game/Materiales/M_AguaRio y lo guarda.
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Agua")
	static bool CrearMaterialAgua();
};
