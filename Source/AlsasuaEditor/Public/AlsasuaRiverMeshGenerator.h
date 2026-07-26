#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AlsasuaRiverMeshGenerator.generated.h"

UCLASS()
class UAlsasuaRiverMeshGenerator : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Rio")
	static bool GenerarLechoRio();

	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Rio")
	static bool GenerarBancasRio();
};
