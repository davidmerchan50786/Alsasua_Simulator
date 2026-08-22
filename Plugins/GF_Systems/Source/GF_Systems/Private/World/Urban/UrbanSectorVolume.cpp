#include "World/Urban/UrbanSectorVolume.h"

AUrbanSectorVolume::AUrbanSectorVolume()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AUrbanSectorVolume::NotifyActorBeginOverlap(AActor* OtherActor)
{
    Super::NotifyActorBeginOverlap(OtherActor);
    if (!OtherActor) return;

    for (const auto& Weak : ActorsInSector)
    {
        if (Weak.Get() == OtherActor) return;
    }

    ActorsInSector.Add(OtherActor);
    OnActorEnteredSector.Broadcast(this, OtherActor);
}

void AUrbanSectorVolume::NotifyActorEndOverlap(AActor* OtherActor)
{
    Super::NotifyActorEndOverlap(OtherActor);
    if (!OtherActor) return;

    for (int32 i = ActorsInSector.Num() - 1; i >= 0; --i)
    {
        if (ActorsInSector[i].Get() == OtherActor)
        {
            ActorsInSector.RemoveAt(i);
            OnActorLeftSector.Broadcast(this, OtherActor);
            return;
        }
    }
}
