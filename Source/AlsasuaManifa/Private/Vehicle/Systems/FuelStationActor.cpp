#include "Vehicle/Systems/FuelStationActor.h"
#include "Components/BoxComponent.h"
#include "Vehicle/VehicleDamageComponent.h"

AFuelStationActor::AFuelStationActor() {
    RepairZone = CreateDefaultSubobject<UBoxComponent>(TEXT("RepairZone"));
    RootComponent = RepairZone;
    RepairZone->SetBoxExtent(FVector(500.f, 500.f, 200.f));
    RepairZone->OnComponentBeginOverlap.AddDynamic(this, &AFuelStationActor::OnRepairOverlap);
}

void AFuelStationActor::OnRepairOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {
    if(UVehicleDamageComponent* DC = OtherActor->FindComponentByClass<UVehicleDamageComponent>()) {
        DC->Health = 100.f;
        DC->IntactTires = 4;
        UE_LOG(LogTemp, Log, TEXT("Vehiculo reparado en gasolinera de Altsasu."));
    }
}