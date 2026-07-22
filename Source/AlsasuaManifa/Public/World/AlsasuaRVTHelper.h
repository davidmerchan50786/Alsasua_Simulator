#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AlsasuaRVTHelper.generated.h"

UCLASS()
class ALSASUAMANIFA_API AAlsasuaRVTHelper : public AActor
{
    GENERATED_BODY()

public:
    // Función para "pintar" humedad en el RVT en una posición específica
    UFUNCTION(BlueprintCallable, Category = "AAA|Visuals")
    void PaintWetnessAtLocation(FVector Location, float Intensity, float Radius);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA|Visuals")
    class URuntimeVirtualTexture* TargetWetnessRVT;
};
