#include "World/AlsasuaStreetArtSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "GeoDataAlsasua.h"

void UAlsasuaStreetArtSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

int32 UAlsasuaStreetArtSystem::ColocarArteCallejero()
{
    UWorld* World = GetWorld();
    if (!World) return 0;

    Arte.Empty();
    int32 Placed = 0;

    const TArray<TPair<FString, FString>> MensajesMurales = {
        {TEXT("Gora Euskal Herria"), TEXT("rojo")},
        {TEXT("Askatasuna"), TEXT("amarillo")},
        {TEXT("Eskubide Sozialak"), TEXT("azul")},
        {TEXT("Altsasu Bizirik"), TEXT("verde")},
        {TEXT("Herri Antzinakoa"), TEXT("naranja")},
        {TEXT("Navarra Aske"), TEXT("rojo")},
        {TEXT("Sakana Gure Ametxea"), TEXT("amarillo")},
        {TEXT("Euskara Mundura"), TEXT("azul")},
    };

    const TArray<TPair<FString, FString>> Grafitis = {
        {TEXT("KA"), TEXT("blanco")},
        {TEXT("ALTSASU"), TEXT("rojo")},
        {TEXT("NAZK"), TEXT("amarillo")},
        {TEXT("ETA"), TEXT("negro")},
        {TEXT("BARRIKA"), TEXT("azul")},
        {TEXT("SARE"), TEXT("verde")},
        {TEXT("HERRIAK"), TEXT("naranja")},
        {TEXT("ASKI"), TEXT("blanco")},
        {TEXT("ERNAI"), TEXT("rojo")},
        {TEXT("GURE"), TEXT("amarillo")},
        {TEXT("HERRIA"), TEXT("azul")},
        {TEXT("EUSKAL"), TEXT("verde")},
        {TEXT("NAFARROA"), TEXT("naranja")},
        {TEXT("SAKANA"), TEXT("blanco")},
        {TEXT("MUNDUA"), TEXT("rojo")},
    };

    for (int32 i = 0; i < MaxMurales; i++)
    {
        const auto& Mural = MensajesMurales[i % MensajesMurales.Num()];
        FString Barrio;
        FVector Pos;

        switch (i % 4)
        {
        case 0:
            Barrio = TEXT("Herriko");
            Pos = UAlsasuaGeoData::UnityaUnreal(FVector(1891.0f + FMath::RandRange(-2.0f, 2.0f),
                8568.0f + FMath::RandRange(-1.0f, 1.0f), 0));
            break;
        case 1:
            Barrio = TEXT("Harrobieta");
            Pos = UAlsasuaGeoData::UnityaUnreal(FVector(1889.0f + FMath::RandRange(-1.5f, 1.5f),
                8569.5f + FMath::RandRange(-1.0f, 1.0f), 0));
            break;
        case 2:
            Barrio = TEXT("Zelai");
            Pos = UAlsasuaGeoData::UnityaUnreal(FVector(1893.5f + FMath::RandRange(-1.5f, 1.5f),
                8573.0f + FMath::RandRange(-1.0f, 1.0f), 0));
            break;
        case 3:
            Barrio = TEXT("Intxostia");
            Pos = UAlsasuaGeoData::UnityaUnreal(FVector(1890.5f + FMath::RandRange(-2.0f, 2.0f),
                8576.5f + FMath::RandRange(-1.5f, 1.5f), 0));
            break;
        default:
            Barrio = TEXT("Herriko");
            Pos = UAlsasuaGeoData::UnityaUnreal(FVector(1891.5f, 8568.5f, 0));
            break;
        }

        Pos.Z += FMath::RandRange(150.0f, 300.0f);
        float Rot = FMath::RandRange(0.0f, 360.0f);

        FStreetArt Art;
        Art.Tipo = TEXT("mural");
        Art.Mensaje = Mural.Key;
        Art.Posicion = Pos;
        Art.Rotacion = Rot;
        Art.Ancho = FMath::RandRange(200.0f, 500.0f);
        Art.Altura = FMath::RandRange(150.0f, 300.0f);
        Art.Barrio = Barrio;
        Art.Color = Mural.Value;

        AStaticMeshActor* MuralActor = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), Pos, FRotator(0, Rot, 0));
        if (MuralActor)
        {
            MuralActor->SetMobility(EComponentMobility::Static);
            float SX = Art.Ancho / 100.0f;
            float SZ = Art.Altura / 100.0f;
            MuralActor->SetActorScale3D(FVector(SX, 0.05f, SZ));

            UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr,
                TEXT("/Game/EngineBasicShapes/Plane"));
            if (PlaneMesh)
                MuralActor->GetStaticMeshComponent()->SetStaticMesh(PlaneMesh);

            UMaterialInterface* MuralMat = LoadObject<UMaterialInterface>(nullptr,
                TEXT("/Game/Materiales/M_Mural_Pared"));
            if (MuralMat)
                MuralActor->GetStaticMeshComponent()->SetMaterial(0, MuralMat);

#if WITH_EDITOR
            MuralActor->SetActorLabel(*FString::Printf(TEXT("Mural_%s_%s"), *Barrio.Left(8), *Art.Mensaje.Left(10)));
#endif
        }

        Arte.Add(Art);
        Placed++;
    }

    for (int32 i = 0; i < MaxGrafitis; i++)
    {
        const auto& Grafiti = Grafitis[i % Grafitis.Num()];
        FString Barrio;
        FVector Pos;

        const TArray<FString> Barrios = {
            TEXT("Herriko"), TEXT("Zelai"), TEXT("Intxostia"), TEXT("Harrobieta"),
            TEXT("Errota"), TEXT("Ferroviario"), TEXT("SanPedro"), TEXT("Monte")
        };
        Barrio = Barrios[FMath::RandRange(0, Barrios.Num() - 1)];

        if (Barrio == TEXT("Herriko"))
            Pos = UAlsasuaGeoData::UnityaUnreal(FVector(1891.0f + FMath::RandRange(-2.0f, 2.0f),
                8568.0f + FMath::RandRange(-1.5f, 1.5f), 0));
        else if (Barrio == TEXT("Zelai"))
            Pos = UAlsasuaGeoData::UnityaUnreal(FVector(1893.0f + FMath::RandRange(-2.0f, 2.0f),
                8573.5f + FMath::RandRange(-1.5f, 1.5f), 0));
        else if (Barrio == TEXT("Intxostia"))
            Pos = UAlsasuaGeoData::UnityaUnreal(FVector(1890.0f + FMath::RandRange(-2.5f, 2.5f),
                8577.0f + FMath::RandRange(-1.5f, 1.5f), 0));
        else if (Barrio == TEXT("Harrobieta"))
            Pos = UAlsasuaGeoData::UnityaUnreal(FVector(1889.5f + FMath::RandRange(-1.5f, 1.5f),
                8569.0f + FMath::RandRange(-1.5f, 1.5f), 0));
        else
            Pos = UAlsasuaGeoData::UnityaUnreal(FVector(1891.5f + FMath::RandRange(-3.0f, 3.0f),
                8572.0f + FMath::RandRange(-3.0f, 3.0f), 0));

        Pos.Z += FMath::RandRange(50.0f, 180.0f);
        float Rot = FMath::RandRange(0.0f, 360.0f);

        FStreetArt Art;
        Art.Tipo = TEXT("grafiti");
        Art.Mensaje = Grafiti.Key;
        Art.Posicion = Pos;
        Art.Rotacion = Rot;
        Art.Ancho = FMath::RandRange(60.0f, 180.0f);
        Art.Altura = FMath::RandRange(40.0f, 120.0f);
        Art.Barrio = Barrio;
        Art.Color = Grafiti.Value;

        AStaticMeshActor* GrafitiActor = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), Pos, FRotator(0, Rot, 0));
        if (GrafitiActor)
        {
            GrafitiActor->SetMobility(EComponentMobility::Static);
            float SX = Art.Ancho / 100.0f;
            float SZ = Art.Altura / 100.0f;
            GrafitiActor->SetActorScale3D(FVector(SX, 0.03f, SZ));

            UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr,
                TEXT("/Game/EngineBasicShapes/Plane"));
            if (PlaneMesh)
                GrafitiActor->GetStaticMeshComponent()->SetStaticMesh(PlaneMesh);

            UMaterialInterface* GrafitiMat = LoadObject<UMaterialInterface>(nullptr,
                TEXT("/Game/Materiales/M_Grafiti"));
            if (GrafitiMat)
                GrafitiActor->GetStaticMeshComponent()->SetMaterial(0, GrafitiMat);

#if WITH_EDITOR
            GrafitiActor->SetActorLabel(*FString::Printf(TEXT("Grafiti_%s_%s"), *Barrio.Left(6), *Grafiti.Key));
#endif
        }

        Arte.Add(Art);
        Placed++;
    }

    UE_LOG(LogTemp, Log, TEXT("StreetArt: %d murales + %d grafis en %d barrios"), MaxMurales, MaxGrafitis, 8);
    return Placed;
}
