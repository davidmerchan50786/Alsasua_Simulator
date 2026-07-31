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
    CargarCalles();
}

void UAlsasuaParkingSystem::CargarCalles()
{
    SegmentosCalle.Empty();
    const FString JsonPath = FPaths::ProjectContentDir() + TEXT("Datos/roads_unity.json");
    TArray<FString> Lines;
    if (!FFileHelper::LoadFileToStringArray(Lines, *JsonPath)) return;

    FString Js;
    for (const FString& L : Lines) Js += L;

    TArray<TSharedPtr<FJsonValue>> RoadsArr;
    TSharedRef<TJsonReader<>> Rd = TJsonReaderFactory<>::Create(Js);
    if (!FJsonSerializer::Deserialize(Rd, RoadsArr)) return;

    for (const auto& RV : RoadsArr)
    {
        const TSharedPtr<FJsonObject>& Road = RV->AsObject();
        if (!Road) continue;

        const TArray<TSharedPtr<FJsonValue>>* PtsArr;
        if (!Road->TryGetArrayField(TEXT("points"), PtsArr)) continue;
        if (PtsArr->Num() < 2) continue;

        TArray<FVector> Pts;
        for (const auto& PV : *PtsArr)
        {
            const TSharedPtr<FJsonObject>& PO = PV->AsObject();
            if (!PO) continue;
            Pts.Add(UAlsasuaGeoData::RelLocalToUE5(FVector(PO->GetNumberField(TEXT("x")), 0.0f, PO->GetNumberField(TEXT("z")))));
        }

        for (int32 i = 0; i < Pts.Num() - 1; ++i)
            SegmentosCalle.Add(TPair<FVector, FVector>(Pts[i], Pts[i + 1]));
    }

    UE_LOG(LogTemp, Log, TEXT("Parking: %d segmentos de calle cargados"), SegmentosCalle.Num());
}

FVector UAlsasuaParkingSystem::ObtenerPuntoEnCalle(FVector& OutDir)
{
    if (SegmentosCalle.Num() == 0)
    {
        OutDir = FVector::ForwardVector;
        return FVector(FMath::RandRange(189000.0f, 192000.0f), FMath::RandRange(856000.0f, 858000.0f), 0.0f);
    }

    const TPair<FVector, FVector>& Seg = SegmentosCalle[FMath::RandRange(0, SegmentosCalle.Num() - 1)];
    const float T = FMath::FRandRange(0.1f, 0.9f);
    const FVector RoadPos = FMath::Lerp(Seg.Key, Seg.Value, T);
    const FVector Dir = (Seg.Value - Seg.Key).GetSafeNormal();
    const FVector Right = FVector(-Dir.Y, Dir.X, 0.0f);
    const float Side = (FMath::RandRange(0, 1) == 0) ? 1.0f : -1.0f;
    const float Offset = FMath::RandRange(150.0f, 250.0f);
    OutDir = Dir;
    return RoadPos + Right * Side * Offset;
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
        FVector RoadDir;
        FVector Pos = ObtenerPuntoEnCalle(RoadDir);

        FParkingSpot Spot;
        Spot.Posicion = Pos;
        Spot.Rotacion = RoadDir.Rotation().Yaw;
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

            AStaticMeshActor* Car = World->SpawnActor<AStaticMeshActor>(
                AStaticMeshActor::StaticClass(), CarPos, FRotator(0, Spot.Rotacion, 0));
            if (Car)
            {
                Car->SetMobility(EComponentMobility::Movable);
                Car->SetActorScale3D(FVector(4.2f, 1.8f, 1.3f));

                // ponytail: no CitySample vehicle meshes — too large, not copied yet
                // UStaticMesh* CarMesh = LoadObject<UStaticMesh>(nullptr,
                //     TEXT("/Game/CitySample/Vehicles/..."));
                // if (CarMesh)
                //     Car->GetStaticMeshComponent()->SetStaticMesh(CarMesh);

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
        FVector GarajeDir;
        FVector Pos = ObtenerPuntoEnCalle(GarajeDir);

        FParkingSpot Garaje;
        Garaje.Posicion = Pos;
        Garaje.Rotacion = GarajeDir.Rotation().Yaw;
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
