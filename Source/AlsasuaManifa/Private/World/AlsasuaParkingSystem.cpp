#include "World/AlsasuaParkingSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"

void UAlsasuaParkingSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

FVector UAlsasuaParkingSystem::ObtenerPuntoEnCalle(const FString& Barrio)
{
    const TMap<FString, FVector> Centros = {
        {TEXT("Herriko"), UAlsasuaGeoData::UnityaUnreal(FVector(1891.5f, 0.0f, 8568.5f))},
        {TEXT("Zelai"), UAlsasuaGeoData::UnityaUnreal(FVector(1893.0f, 0.0f, 8573.5f))},
        {TEXT("Intxostia"), UAlsasuaGeoData::UnityaUnreal(FVector(1890.0f, 0.0f, 8577.0f))},
        {TEXT("SanPedro"), UAlsasuaGeoData::UnityaUnreal(FVector(1895.2f, 0.0f, 8565.5f))},
        {TEXT("Errota"), UAlsasuaGeoData::UnityaUnreal(FVector(1897.0f, 0.0f, 8570.5f))},
        {TEXT("Harrobieta"), UAlsasuaGeoData::UnityaUnreal(FVector(1889.5f, 0.0f, 8569.0f))},
        {TEXT("Ferroviario"), UAlsasuaGeoData::UnityaUnreal(FVector(1892.5f, 0.0f, 8571.5f))},
    };

    if (const FVector* Centro = Centros.Find(Barrio))
        return *Centro + FVector(FMath::RandRange(-800, 800), FMath::RandRange(-800, 800), 0);

    return UAlsasuaGeoData::UnityaUnreal(FVector(1891.5f, 0.0f, 8572.0f));
}

int32 UAlsasuaParkingSystem::GenerarPlazasAparcamiento()
{
    UWorld* World = GetWorld();
    if (!World) return 0;

    Plazas.Empty();

    const TArray<FString> Barrios = {
        TEXT("Herriko"), TEXT("Zelai"), TEXT("Intxostia"), TEXT("SanPedro"),
        TEXT("Errota"), TEXT("Harrobieta"), TEXT("Ferroviario")
    };

    int32 Placed = 0;

    for (int32 i = 0; i < MaxPlazasCalle; i++)
    {
        FString Barrio = Barrios[FMath::RandRange(0, Barrios.Num() - 1)];
        FVector Pos = ObtenerPuntoEnCalle(Barrio);

        FParkingSpot Spot;
        Spot.Posicion = Pos;
        Spot.Rotacion = FMath::RandRange(0.0f, 360.0f);
        Spot.Tipo = TEXT("calle");
        Spot.bOcupado = (FMath::RandRange(0, 3) == 0);
        Spot.Barrio = Barrio;

        AStaticMeshActor* SpotActor = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), Pos, FRotator(0, Spot.Rotacion, 0));
        if (SpotActor)
        {
            SpotActor->SetMobility(EComponentMobility::Static);
            SpotActor->SetActorScale3D(FVector(4.5f, 2.5f, 0.05f));

            UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr,
                TEXT("/Game/EngineBasicShapes/Plane"));
            if (PlaneMesh)
                SpotActor->GetStaticMeshComponent()->SetStaticMesh(PlaneMesh);

            UMaterialInterface* AsphaltMat = LoadObject<UMaterialInterface>(nullptr,
                TEXT("/Game/Materiales/M_Asphalt_Marcas"));
            if (AsphaltMat)
                SpotActor->GetStaticMeshComponent()->SetMaterial(0, AsphaltMat);

#if WITH_EDITOR
            SpotActor->SetActorLabel(*FString::Printf(TEXT("Parking_%s_%d"), *Barrio.Left(8), i));
#endif
        }

        if (Spot.bOcupado)
        {
            FVector CarPos = Pos;
            CarPos.Z += 40.0f;

            AStaticMeshActor* Car = World->SpawnActor<AStaticMeshActor>(
                AStaticMeshActor::StaticClass(), CarPos, FRotator(0, Spot.Rotacion, 0));
            if (Car)
            {
                Car->SetMobility(EComponentMobility::Movable);
                Car->SetActorScale3D(FVector(4.2f, 1.8f, 1.3f));

                UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr,
                    TEXT("/Game/EngineBasicShapes/Cube"));
                if (CubeMesh)
                    Car->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);

                const TArray<FString> ColoresCoche = {
                    TEXT("/Game/Materiales/M_Vehiculo_Blanco"),
                    TEXT("/Game/Materiales/M_Vehiculo_Gris"),
                    TEXT("/Game/Materiales/M_Vehiculo_Negro"),
                    TEXT("/Game/Materiales/M_Vehiculo_Rojo"),
                    TEXT("/Game/Materiales/M_Vehiculo_Azul")
                };
                UMaterialInterface* CarMat = LoadObject<UMaterialInterface>(nullptr,
                    *ColoresCoche[FMath::RandRange(0, ColoresCoche.Num() - 1)]);
                if (CarMat)
                    Car->GetStaticMeshComponent()->SetMaterial(0, CarMat);

#if WITH_EDITOR
                Car->SetActorLabel(*FString::Printf(TEXT("CocheAparcado_%s_%d"), *Barrio.Left(8), i));
#endif
            }
        }

        Plazas.Add(Spot);
        Placed++;
    }

    for (int32 i = 0; i < MaxGarajes; i++)
    {
        FString Barrio = Barrios[FMath::RandRange(0, Barrios.Num() - 1)];
        FVector Pos = ObtenerPuntoEnCalle(Barrio);

        FParkingSpot Garaje;
        Garaje.Posicion = Pos;
        Garaje.Rotacion = FMath::RandRange(0.0f, 360.0f);
        Garaje.Tipo = TEXT("garaje");
        Garaje.bOcupado = true;
        Garaje.Barrio = Barrio;

        AStaticMeshActor* GarajeActor = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), Pos, FRotator(0, Garaje.Rotacion, 0));
        if (GarajeActor)
        {
            GarajeActor->SetMobility(EComponentMobility::Static);
            GarajeActor->SetActorScale3D(FVector(5.0f, 6.0f, 3.0f));

            UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr,
                TEXT("/Game/EngineBasicShapes/Cube"));
            if (CubeMesh)
                GarajeActor->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);

            UMaterialInterface* GarajeMat = LoadObject<UMaterialInterface>(nullptr,
                TEXT("/Game/Materiales/M_Hormigon_Garaje"));
            if (GarajeMat)
                GarajeActor->GetStaticMeshComponent()->SetMaterial(0, GarajeMat);

#if WITH_EDITOR
            GarajeActor->SetActorLabel(*FString::Printf(TEXT("Garaje_%s_%d"), *Barrio.Left(8), i));
#endif
        }

        Plazas.Add(Garaje);
        Placed++;
    }

    UE_LOG(LogTemp, Log, TEXT("Parking: %d plazas (%d calle + %d garajes) generadas"),
        Placed, MaxPlazasCalle, MaxGarajes);
    return Placed;
}
