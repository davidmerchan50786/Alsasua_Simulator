#include "World/AlsasuaCollisionSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

void UAlsasuaCollisionSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

int32 UAlsasuaCollisionSystem::RepasarColisionesDeProps()
{
    UWorld* World = GetWorld();
    if (!World) return 0;

    // Sólo AStaticMeshActor: edificios y calles no son de esta clase y además ya
    // vienen con BlockAll de su propio constructor. Ver la cabecera.
    TArray<AActor*> Props;
    UGameplayStatics::GetAllActorsOfClass(World, AStaticMeshActor::StaticClass(), Props);

    int32 Repasados = 0;
    for (AActor* Actor : Props)
    {
        if (!Actor) continue;

        UStaticMeshComponent* Mesh = Actor->FindComponentByClass<UStaticMeshComponent>();
        if (!Mesh || !Mesh->GetStaticMesh()) continue;

        // Sin malla o ya con colisión, no hay nada que hacer. Y lo que simula
        // física se deja en paz.
        if (Mesh->IsSimulatingPhysics()) continue;
        if (Mesh->GetCollisionEnabled() != ECollisionEnabled::NoCollision) continue;

        Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Mesh->SetCollisionObjectType(ECC_WorldStatic);
        Mesh->SetCollisionResponseToAllChannels(ECR_Block);
        ++Repasados;
    }

    UE_LOG(LogTemp, Log,
        TEXT("CollisionSystem: %d props sin colisión repasados de %d AStaticMeshActor (edificios y calles ya vienen con BlockAll)"),
        Repasados, Props.Num());
    return Repasados;
}
