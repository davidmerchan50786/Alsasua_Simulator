// CreadorMaterialTerrenoOrto.h (capa EDITOR, sólo editor)
// Genera /Game/Materiales/M_Terreno_Orto: material de Landscape que proyecta la
// ortofoto PNOA en planta sobre el terreno (mismo mapeo que los tejados) y se
// funde a un color neutro fuera del área cubierta por la ortofoto.
// Asígnalo como "Landscape Material" del Landscape.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CreadorMaterialTerrenoOrto.generated.h"

UCLASS()
class UCreadorMaterialTerrenoOrto : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Terreno")
	static bool CrearMaterialTerrenoOrto();
};
