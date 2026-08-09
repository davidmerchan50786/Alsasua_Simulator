#include "World/AlsasuaLODManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

TStatId UAlsasuaLODManager::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UAlsasuaLODManager, STATGROUP_Tickables);
}

void UAlsasuaLODManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    UWorld* World = GetWorld();
    if (!World) return;

    TimeSinceCache += DeltaTime;
    if (TimeSinceCache > 5.0f || !bCached)
    {
        CacheActors();
        TimeSinceCache = 0.0f;
    }

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC) return;
    APawn* Pawn = PC->GetPawn();
    if (!Pawn) return;

    const FVector CamLoc = Pawn->GetActorLocation();

    UpdateTreeLODs(CamLoc);
    UpdateBuildingLODs(CamLoc);
}

void UAlsasuaLODManager::CacheActors()
{
    UWorld* World = GetWorld();
    if (!World) return;

    TArray<AActor*> AllStatic;
    UGameplayStatics::GetAllActorsOfClass(World, AStaticMeshActor::StaticClass(), AllStatic);

    CachedTrees.Empty(2783);
    CachedBuildings.Empty(1030);

    for (AActor* Actor : AllStatic)
    {
        if (!Actor) continue;
        const FString Label = Actor->GetName();
        if (Label.Contains(TEXT("Arbol_")) || Label.Contains(TEXT("Arboleda")))
            CachedTrees.Add(Actor);
        else if (Label.Contains(TEXT("Edificio_")) || Label.Contains(TEXT("Building_")))
            CachedBuildings.Add(Actor);
    }

    bCached = true;
}

void UAlsasuaLODManager::UpdateTreeLODs(const FVector& CameraLocation)
{
    int32 VisibleCount = 0;
    for (AActor* Tree : CachedTrees)
    {
        if (!Tree) continue;
        const float Dist = FVector::Distance(CameraLocation, Tree->GetActorLocation());

        UStaticMeshComponent* Mesh = Tree->FindComponentByClass<UStaticMeshComponent>();
        if (!Mesh) continue;

        if (Dist > TreeHideDistance || VisibleCount >= MaxVisibleTrees)
        {
            Tree->SetActorHiddenInGame(true);
            Tree->SetActorEnableCollision(false);
        }
        else if (Dist > TreeLOD1Distance)
        {
            Tree->SetActorHiddenInGame(false);
            Tree->SetActorEnableCollision(false);
            Mesh->SetForcedLodModel(3);
            VisibleCount++;
        }
        else if (Dist > TreeLOD0Distance)
        {
            Tree->SetActorHiddenInGame(false);
            Tree->SetActorEnableCollision(true);
            Mesh->SetForcedLodModel(2);
            VisibleCount++;
        }
        else
        {
            Tree->SetActorHiddenInGame(false);
            Tree->SetActorEnableCollision(true);
            Mesh->SetForcedLodModel(0);
            VisibleCount++;
        }
    }
}

void UAlsasuaLODManager::UpdateBuildingLODs(const FVector& CameraLocation)
{
    int32 VisibleCount = 0;
    for (AActor* Building : CachedBuildings)
    {
        if (!Building) continue;
        const float Dist = FVector::Distance(CameraLocation, Building->GetActorLocation());

        UStaticMeshComponent* Mesh = Building->FindComponentByClass<UStaticMeshComponent>();
        if (!Mesh) continue;

        if (Dist > LOD2Distance || VisibleCount >= MaxVisibleBuildings)
        {
            Building->SetActorHiddenInGame(true);
            Building->SetActorEnableCollision(false);
        }
        else if (Dist > LOD1Distance)
        {
            Building->SetActorHiddenInGame(false);
            Building->SetActorEnableCollision(true);
            Mesh->SetForcedLodModel(2);
            VisibleCount++;
        }
        else if (Dist > LOD0Distance)
        {
            Building->SetActorHiddenInGame(false);
            Building->SetActorEnableCollision(true);
            Mesh->SetForcedLodModel(1);
            VisibleCount++;
        }
        else
        {
            Building->SetActorHiddenInGame(false);
            Building->SetActorEnableCollision(true);
            Mesh->SetForcedLodModel(0);
            VisibleCount++;
        }
    }
}

void UAlsasuaLODManager::ConfigureLODsForQuality(int32 QualityLevel)
{
    switch (QualityLevel)
    {
    case 0: // Low
        LOD0Distance = 3000.0f;
        LOD1Distance = 8000.0f;
        LOD2Distance = 15000.0f;
        TreeLOD0Distance = 2000.0f;
        TreeLOD1Distance = 5000.0f;
        TreeHideDistance = 12000.0f;
        MaxVisibleTrees = 1000;
        MaxVisibleBuildings = 300;
        break;
    case 1: // Medium
        LOD0Distance = 5000.0f;
        LOD1Distance = 15000.0f;
        LOD2Distance = 30000.0f;
        TreeLOD0Distance = 3000.0f;
        TreeLOD1Distance = 8000.0f;
        TreeHideDistance = 20000.0f;
        MaxVisibleTrees = 2000;
        MaxVisibleBuildings = 500;
        break;
    case 2: // High
        LOD0Distance = 8000.0f;
        LOD1Distance = 20000.0f;
        LOD2Distance = 40000.0f;
        TreeLOD0Distance = 5000.0f;
        TreeLOD1Distance = 12000.0f;
        TreeHideDistance = 30000.0f;
        MaxVisibleTrees = 3000;
        MaxVisibleBuildings = 800;
        break;
    case 3: // Ultra
        LOD0Distance = 12000.0f;
        LOD1Distance = 30000.0f;
        LOD2Distance = 50000.0f;
        TreeLOD0Distance = 8000.0f;
        TreeLOD1Distance = 18000.0f;
        TreeHideDistance = 40000.0f;
        MaxVisibleTrees = 5000;
        MaxVisibleBuildings = 1030;
        break;
    }

    UE_LOG(LogTemp, Log, TEXT("LODManager: Calidad %d configurada"), QualityLevel);
}
