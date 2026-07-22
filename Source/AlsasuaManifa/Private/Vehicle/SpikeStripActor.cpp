#include "Vehicle/SpikeStripActor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Vehicle/VehicleDamageComponent.h"

ASpikeStripActor::ASpikeStripActor()
{
    TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
    RootComponent = TriggerBox;
    TriggerBox->SetBoxExtent(FVector(100.f, 400.f, 20.f));
    TriggerBox->SetCollisionProfileName(TEXT("Trigger"));

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(RootComponent);

    TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ASpikeStripActor::OnOverlap);
}

void ASpikeStripActor::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (OtherActor)
    {
        if (UVehicleDamageComponent* DamageComp = OtherActor->FindComponentByClass<UVehicleDamageComponent>())
        {
            DamageComp->PopTire();
            UE_LOG(LogTemp, Warning, TEXT("¡Neumático pinchado por pinchos de carretera!"));
        }
    }
}
