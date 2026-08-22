#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FuelStationActor.generated.h"

UCLASS()
class GF_VEHICULOS_API AFuelStationActor : public AActor {
    GENERATED_BODY()
public:
    AFuelStationActor();

    UPROPERTY(VisibleAnywhere) class UBoxComponent* RepairZone;
    UPROPERTY(EditAnywhere, Category="AAA|Station") float RepairSpeed = 20.f;

    UFUNCTION()
    void OnRepairOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};