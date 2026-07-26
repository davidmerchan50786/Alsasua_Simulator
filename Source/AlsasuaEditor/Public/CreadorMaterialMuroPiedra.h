#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CreadorMaterialMuroPiedra.generated.h"

UCLASS()
class UCreadorMaterialMuroPiedra : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Materiales")
	static bool CrearMaterialMuroPiedra();
};
