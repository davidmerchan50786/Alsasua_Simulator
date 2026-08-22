#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpikeStripActor.generated.h"

UCLASS()
class GF_VEHICULOS_API ASpikeStripActor : public AActor
{
    GENERATED_BODY()

public:
    ASpikeStripActor();

    UPROPERTY(VisibleAnywhere)
    class UBoxComponent* TriggerBox;

    UPROPERTY(VisibleAnywhere)
    class UStaticMeshComponent* Mesh;

protected:
    UFUNCTION()
    void OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
