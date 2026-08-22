#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AlsasuaRVTHelper.generated.h"

class UMaterialParameterCollection;

UCLASS()
class GF_WORLD_API AAlsasuaRVTHelper : public AActor
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;
    // Función para "pintar" humedad en el RVT en una posición específica
    UFUNCTION(BlueprintCallable, Category = "AAA|Visuals")
    void PaintWetnessAtLocation(FVector Location, float Intensity, float Radius);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA|Visuals")
    class URuntimeVirtualTexture* TargetWetnessRVT;

private:
    UPROPERTY()
    TObjectPtr<UMaterialParameterCollection> CachedMPC = nullptr;
};
