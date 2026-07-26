#include "World/AlsasuaContainerSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"

void UAlsasuaContainerSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

int32 UAlsasuaContainerSystem::ColocarContenedores()
{
    UWorld* World = GetWorld();
    if (!World) return 0;

    Contenedores.Empty();

    const TArray<FString> Barrios = {
        TEXT("Herriko"), TEXT("Zelai"), TEXT("Intxostia"), TEXT("SanPedro"),
        TEXT("Errota"), TEXT("Harrobieta"), TEXT("Ferroviario"), TEXT("Monte")
    };

    const TArray<TPair<FString, FLinearColor>> TiposContenedor = {
        {TEXT("resto"), FLinearColor(0.3f, 0.3f, 0.3f)},
        {TEXT("papel"), FLinearColor(0.0f, 0.3f, 0.7f)},
        {TEXT("plastico"), FLinearColor(0.0f, 0.5f, 0.0f)},
        {TEXT("vidrio"), FLinearColor(0.1f, 0.6f, 0.1f)},
        {TEXT("organico"), FLinearColor(0.5f, 0.3f, 0.1f)},
    };

    int32 Placed = 0;

    for (int32 i = 0; i < MaxContenedores; i++)
    {
        FString Barrio = Barrios[FMath::RandRange(0, Barrios.Num() - 1)];
        FVector Centro;
        if (Barrio == TEXT("Herriko"))
            Centro = UAlsasuaGeoData::UnityaUnreal(FVector(1891.5f, 8568.5f, 0));
        else if (Barrio == TEXT("Zelai"))
            Centro = UAlsasuaGeoData::UnityaUnreal(FVector(1893.0f, 8573.5f, 0));
        else if (Barrio == TEXT("Intxostia"))
            Centro = UAlsasuaGeoData::UnityaUnreal(FVector(1890.0f, 8577.0f, 0));
        else if (Barrio == TEXT("SanPedro"))
            Centro = UAlsasuaGeoData::UnityaUnreal(FVector(1895.2f, 8565.5f, 0));
        else if (Barrio == TEXT("Errota"))
            Centro = UAlsasuaGeoData::UnityaUnreal(FVector(1897.0f, 8570.5f, 0));
        else if (Barrio == TEXT("Harrobieta"))
            Centro = UAlsasuaGeoData::UnityaUnreal(FVector(1889.5f, 8569.0f, 0));
        else if (Barrio == TEXT("Ferroviario"))
            Centro = UAlsasuaGeoData::UnityaUnreal(FVector(1892.5f, 8571.5f, 0));
        else
            Centro = UAlsasuaGeoData::UnityaUnreal(FVector(1894.0f, 8575.0f, 0));

        FVector Pos = Centro + FVector(
            FMath::RandRange(-1000.0f, 1000.0f),
            FMath::RandRange(-1000.0f, 1000.0f), 0);

        const auto& TipoInfo = TiposContenedor[FMath::RandRange(0, TiposContenedor.Num() - 1)];
        float Rot = FMath::RandRange(0.0f, 360.0f);

        FContainer Cont;
        Cont.Tipo = TipoInfo.Key;
        Cont.Posicion = Pos;
        Cont.Rotacion = Rot;
        Cont.Barrio = Barrio;

        AStaticMeshActor* ContActor = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), Pos, FRotator(0, Rot, 0));
        if (ContActor)
        {
            ContActor->SetMobility(EComponentMobility::Static);
            ContActor->SetActorScale3D(FVector(1.2f, 0.8f, 1.0f));

            UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr,
                TEXT("/Game/EngineBasicShapes/Cube"));
            if (CubeMesh)
                ContActor->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);

            UMaterialInterface* ContMat = LoadObject<UMaterialInterface>(nullptr,
                TEXT("/Game/Materiales/M_Contenedor"));
            if (ContMat)
                ContActor->GetStaticMeshComponent()->SetMaterial(0, ContMat);

#if WITH_EDITOR
            ContActor->SetActorLabel(*FString::Printf(TEXT("Contenedor_%s_%s_%d"),
                *TipoInfo.Key, *Barrio.Left(6), i));
#endif
        }

        Contenedores.Add(Cont);
        Placed++;
    }

    UE_LOG(LogTemp, Log, TEXT("Containers: %d contenedores de residuos en 8 barrios"), Placed);
    return Placed;
}
