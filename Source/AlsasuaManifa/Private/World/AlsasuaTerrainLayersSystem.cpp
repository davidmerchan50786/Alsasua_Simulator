#include "World/AlsasuaTerrainLayersSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "GeoDataAlsasua.h"

void UAlsasuaTerrainLayersSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CrearLayers();
}

void UAlsasuaTerrainLayersSystem::CrearLayers()
{
    Layers.Empty();

    Layers.Add(FBarrioTerrainLayer{TEXT("Herriko"), TEXT("/Game/Materiales/Pavimentos/M_Piedra_Herriko"), 1500.0f, -10.0f});
    Layers.Add(FBarrioTerrainLayer{TEXT("Zelai"), TEXT("/Game/Materiales/Pavimentos/M_Asphalt_Zelai"), 2000.0f, 0.0f});
    Layers.Add(FBarrioTerrainLayer{TEXT("Intxostia"), TEXT("/Game/Materiales/Pavimentos/M_Asphalt_Intxostia"), 2500.0f, 0.0f});
    Layers.Add(FBarrioTerrainLayer{TEXT("SanPedro"), TEXT("/Game/Materiales/Pavimentos/M_Asphalt_SanPedro"), 2000.0f, 0.0f});
    Layers.Add(FBarrioTerrainLayer{TEXT("Errota"), TEXT("/Game/Materiales/Pavimentos/M_Gravel_Errota"), 1500.0f, -5.0f});
    Layers.Add(FBarrioTerrainLayer{TEXT("Harrobieta"), TEXT("/Game/Materiales/Pavimentos/M_Piedra_Harrobieta"), 1500.0f, -10.0f});
    Layers.Add(FBarrioTerrainLayer{TEXT("Ferroviario"), TEXT("/Game/Materiales/Pavimentos/M_Asphalt_Ferroviario"), 2000.0f, 0.0f});
    Layers.Add(FBarrioTerrainLayer{TEXT("Monte"), TEXT("/Game/Materiales/Naturaleza/M_Tierra_Monte"), 3000.0f, -15.0f});
}

void UAlsasuaTerrainLayersSystem::AplicarMaterialesPorBarrio()
{
    UWorld* World = GetWorld();
    if (!World) return;

    for (const FBarrioTerrainLayer& Layer : Layers)
    {
        AStaticMeshActor* FloorActor = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
        if (!FloorActor) continue;

        FloorActor->SetMobility(EComponentMobility::Static);

        UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr,
            TEXT("/Game/EngineBasicShapes/Plane"));
        if (PlaneMesh)
            FloorActor->GetStaticMeshComponent()->SetStaticMesh(PlaneMesh);

        float Scale = 1000.0f;
        FloorActor->SetActorScale3D(FVector(Scale, Scale, 1.0f));

        UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, *Layer.MaterialPath);
        if (Mat)
            FloorActor->GetStaticMeshComponent()->SetMaterial(0, Mat);

#if WITH_EDITOR
        FloorActor->SetActorLabel(*FString::Printf(TEXT("Suelo_%s"), *Layer.Barrio));
#endif
    }

    UE_LOG(LogTemp, Log, TEXT("TerrainLayers: %d capas de suelo creadas"), Layers.Num());
}

void UAlsasuaTerrainLayersSystem::GenerarSueloCiudad()
{
    UWorld* World = GetWorld();
    if (!World) return;

    float Scale = 1000.0f;
    FVector CiudadCenter = UAlsasuaGeoData::UnityaUnreal(FVector(1900.0f, 8570.0f, 0.0f));

    AStaticMeshActor* Suelo = World->SpawnActor<AStaticMeshActor>(
        AStaticMeshActor::StaticClass(), CiudadCenter, FRotator::ZeroRotator);
    if (!Suelo) return;

    Suelo->SetMobility(EComponentMobility::Static);
    Suelo->SetActorScale3D(FVector(Scale, Scale, 1.0f));

    UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr,
        TEXT("/Game/EngineBasicShapes/Plane"));
    if (PlaneMesh)
        Suelo->GetStaticMeshComponent()->SetStaticMesh(PlaneMesh);

    UMaterialInterface* SueloMat = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Materiales/M_Suelo_Ciudad"));
    if (SueloMat)
        Suelo->GetStaticMeshComponent()->SetMaterial(0, SueloMat);

#if WITH_EDITOR
    Suelo->SetActorLabel(TEXT("Suelo_Base_Altsasu"));
#endif

    AplicarMaterialesPorBarrio();

    UE_LOG(LogTemp, Log, TEXT("TerrainLayers: Suelo base de la ciudad generado"));
}
