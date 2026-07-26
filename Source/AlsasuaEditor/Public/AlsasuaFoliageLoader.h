#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AlsasuaFoliageLoader.generated.h"

UCLASS()
class UAlsasuaFoliageLoader : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Foliage")
	static bool ScanAndRegisterFoliage();

	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Foliage")
	static bool ReplaceProceduralTreesWithFoliage();
};
