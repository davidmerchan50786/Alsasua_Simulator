#include "Utils/ActorPoolComponent.h"

void UActorPoolComponent::BeginPlay() {
    Super::BeginPlay();
    if(!ActorClass) return;

    for(int32 i=0; i<PoolSize; i++) {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AActor* NewActor = GetWorld()->SpawnActor<AActor>(ActorClass, FVector::ZeroVector, FRotator::ZeroRotator, Params);
        if(NewActor) {
            NewActor->SetActorHiddenInGame(true);
            NewActor->SetActorEnableCollision(false);
            InactivePool.Add(NewActor);
        }
    }
}

AActor* UActorPoolComponent::GetFromPool(FVector Location, FRotator Rotation) {
    if(InactivePool.Num() > 0) {
        AActor* Actor = InactivePool.Pop();
        Actor->SetActorLocationAndRotation(Location, Rotation);
        Actor->SetActorHiddenInGame(false);
        Actor->SetActorEnableCollision(true);
        return Actor;
    }
    return nullptr;
}

void UActorPoolComponent::ReturnToPool(AActor* Actor) {
    if(Actor) {
        Actor->SetActorHiddenInGame(true);
        Actor->SetActorEnableCollision(false);
        InactivePool.Add(Actor);
    }
}