#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AlsasuaBridgeGenerator.generated.h"

UCLASS()
class UAlsasuaBridgeGenerator : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Puentes")
	static bool GenerarPuentesMejorados();
};
