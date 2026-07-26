#include "World/AlsasuaFountainSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "GeoDataAlsasua.h"
#include "Kismet/KismetMathLibrary.h"

void UAlsasuaFountainSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

int32 UAlsasuaFountainSystem::ColocarFuentes()
{
    const TArray<TPair<FString, FString>> CallesFuentes = {
        {TEXT("Foruen Plaza"), TEXT("Herriko")},
        {TEXT("Kale Nagusia"), TEXT("Herriko")},
        {TEXT("Iruñeko Etorbidea"), TEXT("Zelai")},
        {TEXT("San Pedro bidea"), TEXT("SanPedro")},
        {TEXT("Harrobieta kalea"), TEXT("Harrobieta")},
        {TEXT("Geltokia kalea"), TEXT("Ferroviario")},
        {TEXT("Errota kalea"), TEXT("Errota")},
        {TEXT("Zelai kalea"), TEXT("Zelai")},
    };

    const TArray<FVector> PosicionesBase = {
        UAlsasuaGeoData::UnityaUnreal(FVector(1891.5f, 0.0f, 8568.5f)),
        UAlsasuaGeoData::UnityaUnreal(FVector(1890.8f, 0.0f, 8570.2f)),
        UAlsasuaGeoData::UnityaUnreal(FVector(1893.0f, 0.0f, 8573.5f)),
        UAlsasuaGeoData::UnityaUnreal(FVector(1895.2f, 0.0f, 8565.5f)),
        UAlsasuaGeoData::UnityaUnreal(FVector(1889.5f, 0.0f, 8569.0f)),
        UAlsasuaGeoData::UnityaUnreal(FVector(1892.5f, 0.0f, 8571.5f)),
        UAlsasuaGeoData::UnityaUnreal(FVector(1897.0f, 0.0f, 8570.5f)),
        UAlsasuaGeoData::UnityaUnreal(FVector(1893.5f, 0.0f, 8574.5f)),
    };

    Fuentes.Empty();
    for (int32 i = 0; i < CallesFuentes.Num(); i++)
    {
        FRealFountain Fuente;
        Fuente.Nombre = FString::Printf(TEXT("Iturri_%s"), *CallesFuentes[i].Key.Left(12));
        Fuente.Calle = CallesFuentes[i].Key;
        Fuente.Barrio = CallesFuentes[i].Value;
        Fuente.Posicion = PosicionesBase[i];
        Fuente.Radio = FMath::RandRange(100.0f, 200.0f);
        Fuente.AlturaCazoleta = FMath::RandRange(60.0f, 120.0f);
        Fuente.bFuncional = true;
        Fuentes.Add(Fuente);
    }

    UWorld* World = GetWorld();
    if (!World) return 0;

    int32 Placed = 0;
    for (const FRealFountain& Fuente : Fuentes)
    {
        FVector Pos = Fuente.Posicion;
        Pos.Z += Fuente.AlturaCazoleta;

        AStaticMeshActor* Cazoleta = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), Pos, FRotator::ZeroRotator);
        if (!Cazoleta) continue;

        Cazoleta->SetMobility(EComponentMobility::Static);

        float Scale = Fuente.Radio / 50.0f;
        Cazoleta->SetActorScale3D(FVector(Scale, Scale, 0.3f));

        UStaticMesh* CylinderMesh = LoadObject<UStaticMesh>(nullptr,
            TEXT("/Game/EngineBasicShapes/Cylinder"));
        if (CylinderMesh)
            Cazoleta->GetStaticMeshComponent()->SetStaticMesh(CylinderMesh);

        UMaterialInterface* FuenteMat = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/Materiales/M_Piedra_Fuente"));
        if (!FuenteMat)
            FuenteMat = LoadObject<UMaterialInterface>(nullptr,
                TEXT("/Game/Materiales/M_Piedra"));

        if (FuenteMat)
            Cazoleta->GetStaticMeshComponent()->SetMaterial(0, FuenteMat);

        AStaticMeshActor* Base = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), Fuente.Posicion, FRotator::ZeroRotator);
        if (Base)
        {
            Base->SetMobility(EComponentMobility::Static);
            Base->SetActorScale3D(FVector(Scale * 1.2f, Scale * 1.2f, 0.15f));
            if (CylinderMesh)
                Base->GetStaticMeshComponent()->SetStaticMesh(CylinderMesh);
            if (FuenteMat)
                Base->GetStaticMeshComponent()->SetMaterial(0, FuenteMat);
        }

#if WITH_EDITOR
        Cazoleta->SetActorLabel(*FString::Printf(TEXT("Fuente_%s"), *Fuente.Barrio));
        if (Base) Base->SetActorLabel(*FString::Printf(TEXT("FuenteBase_%s"), *Fuente.Barrio));
#endif

        Placed++;
    }

    UE_LOG(LogTemp, Log, TEXT("Fountains: %d fuentes reales colocadas en calles de Altsasu"), Placed);
    return Placed;
}
