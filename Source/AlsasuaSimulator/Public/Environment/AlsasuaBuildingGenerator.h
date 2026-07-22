#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "AlsasuaBuildingGenerator.generated.h"

UCLASS()
class ALSASUASIMULATOR_API AAlsasuaBuildingGenerator : public AActor {
    GENERATED_BODY()
public:
    AAlsasuaBuildingGenerator();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Alsasua|Urban")
    UInstancedStaticMeshComponent* BuildingInstances;

    // Crea edificios a partir de datos JSON o coordenadas OSM
    UFUNCTION(BlueprintCallable, Category = "Alsasua|Urban")
    void SpawnBuildingsFromData(FString JsonPath);
};
