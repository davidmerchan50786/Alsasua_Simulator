#include "World/AlsasuaCollisionSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"

void UAlsasuaCollisionSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

int32 UAlsasuaCollisionSystem::GenerarColisionesEdificios()
{
    UWorld* World = GetWorld();
    if (!World) return 0;

    TArray<AActor*> Buildings;
    UGameplayStatics::GetAllActorsOfClass(World, AStaticMeshActor::StaticClass(), Buildings);

    int32 CollisionsAdded = 0;
    for (AActor* Actor : Buildings)
    {
        if (!Actor) continue;
        const FString Label = Actor->GetActorLabel();
        if (!Label.Contains(TEXT("Edificio")) && !Label.Contains(TEXT("Building")))
            continue;

        UStaticMeshComponent* Mesh = Actor->FindComponentByClass<UStaticMeshComponent>();
        if (!Mesh) continue;

        if (!Mesh->IsSimulatingPhysics() && Mesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
        {
            Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            Mesh->SetCollisionObjectType(ECC_WorldStatic);
            Mesh->SetCollisionResponseToAllChannels(ECR_Block);
            CollisionsAdded++;
        }
    }

    UE_LOG(LogTemp, Log, TEXT("CollisionSystem: %d colisiones de edificios generadas"), CollisionsAdded);
    return CollisionsAdded;
}

int32 UAlsasuaCollisionSystem::GenerarColisionesCalles()
{
    UWorld* World = GetWorld();
    if (!World) return 0;

    TArray<AActor*> Roads;
    UGameplayStatics::GetAllActorsOfClass(World, AStaticMeshActor::StaticClass(), Roads);

    int32 CollisionsAdded = 0;
    for (AActor* Actor : Roads)
    {
        if (!Actor) continue;
        const FString Label = Actor->GetActorLabel();
        if (!Label.Contains(TEXT("Road_")) && !Label.Contains(TEXT("Calle_")) && !Label.Contains(TEXT("Rio_")))
            continue;

        UStaticMeshComponent* Mesh = Actor->FindComponentByClass<UStaticMeshComponent>();
        if (!Mesh) continue;

        if (!Mesh->IsSimulatingPhysics() && Mesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
        {
            Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            Mesh->SetCollisionObjectType(ECC_WorldStatic);
            Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
            CollisionsAdded++;
        }
    }

    UE_LOG(LogTemp, Log, TEXT("CollisionSystem: %d colisiones de calles generadas"), CollisionsAdded);
    return CollisionsAdded;
}
