#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "AlsasuaRoadManager.generated.h"

UCLASS()
class ALSASUASIMULATOR_API AAlsasuaRoadManager : public AActor {
    GENERATED_BODY()
public:
    AAlsasuaRoadManager();
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Alsasua|Roads")
    USplineComponent* RoadSpline;

    // Genera la malla de la carretera dinámicamente (Nanite Ready)
    UFUNCTION(BlueprintCallable, Category = "Alsasua|Roads")
    void GenerateRoadMesh();
};
