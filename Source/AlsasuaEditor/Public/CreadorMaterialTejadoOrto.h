// CreadorMaterialTejadoOrto.h (capa EDITOR, sólo editor)
// Genera /Game/Materiales/M_Tejado_Orto: proyecta la ortofoto PNOA (25 cm/px)
// en planta sobre los tejados (mapeo de WorldPosition.XY a UV de la ortofoto).
// Importa antes la textura /Game/Textures/T_Ortofoto y asígnala al parámetro
// "Ortofoto" (o se toma por defecto si existe en esa ruta).
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CreadorMaterialTejadoOrto.generated.h"

UCLASS()
class UCreadorMaterialTejadoOrto : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Edificios")
	static bool CrearMaterialTejadoOrto();
};
