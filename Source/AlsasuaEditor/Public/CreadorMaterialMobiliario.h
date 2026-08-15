#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CreadorMaterialMobiliario.generated.h"

UCLASS()
class UCreadorMaterialMobiliario : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Materiales")
	static bool CrearMaterialMobiliario();

	/** Hierro pintado: farolas, barandillas, semáforos, bolardos. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Materiales")
	static bool CrearMaterialMetal();
};
