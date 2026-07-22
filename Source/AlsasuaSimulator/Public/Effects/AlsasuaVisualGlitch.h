#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AlsasuaVisualGlitch.generated.h"

UCLASS()
class ALSASUASIMULATOR_API UAlsasuaVisualGlitch : public UBlueprintFunctionLibrary {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category = "Alsasua|Effects")
    static void UpdateNPCParanoiaEffect(USkeletalMeshComponent* Mesh, float ParanoiaLevel);
};
