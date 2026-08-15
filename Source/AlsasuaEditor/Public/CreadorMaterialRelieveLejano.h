// CreadorMaterialRelieveLejano.h (capa EDITOR, sólo editor)
// Genera /Game/Materiales/M_Relieve_Lejano, el material del anillo de relieve que
// rodea al terreno jugable (ATerrenoLejano).
//
// Mezcla las dos cosas que se piden a esa distancia: cerca de la costura con el
// terreno jugable manda la ortofoto PNOA de 60 km, para que no se note el salto;
// según nos alejamos se funde a un color por altitud y pendiente (prado, roca,
// caliza de cumbre), que a 20-30 km lee mejor que una foto de 14,6 m/px y no
// necesita textura nítida.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CreadorMaterialRelieveLejano.generated.h"

UCLASS()
class UCreadorMaterialRelieveLejano : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Terreno")
	static bool CrearMaterialRelieveLejano();
};
