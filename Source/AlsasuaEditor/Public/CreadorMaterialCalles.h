#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CreadorMaterialCalles.generated.h"

UCLASS()
class UCreadorMaterialCalles : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Materiales")
	static bool CrearMaterialCalles();

	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Materiales")
	static bool CrearMaterialAcera();

	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Materiales")
	static bool CrearMaterialMarcaBlanca();
};
