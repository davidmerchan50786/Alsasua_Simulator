#pragma once
#include "CoreMinimal.h"
#include "Components/SplineMeshComponent.h"
#include "AlsasuaRoadComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALSASUASIMULATOR_API UAlsasuaRoadComponent : public USplineMeshComponent {
    GENERATED_BODY()
public:
    UAlsasuaRoadComponent();
    
    // Configura el material para que use el Runtime Virtual Texture del terreno
    UFUNCTION(BlueprintCallable, Category = "Alsasua|Environment")
    void SetupRVTMasking();
};
