#include "World/AlsasuaDoorEntranceSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"

void UAlsasuaDoorEntranceSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

int32 UAlsasuaDoorEntranceSystem::ColocarPuertas()
{
    const FString JsonPath = FPaths::ProjectContentDir() + TEXT("Datos/buildings_final.json");
    FString JsonStr;
    if (!FFileHelper::LoadFileToString(JsonStr, *JsonPath)) return 0;

    TSharedPtr<FJsonValue> RootVal;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    if (!FJsonSerializer::Deserialize(Reader, RootVal) || !RootVal.IsValid()) return 0;

    const TArray<TSharedPtr<FJsonValue>>* BuildingsArr;
    if (!RootVal->TryGetArray(BuildingsArr)) return 0;

    UWorld* World = GetWorld();
    if (!World) return 0;

    Puertas.Empty();
    int32 Placed = 0;

    const TArray<FString> ColoresPuerta = {
        TEXT("marron"), TEXT("verde_oscuro"), TEXT("azul_oscuro"),
        TEXT("rojo"), TEXT("gris"), TEXT("negro"), TEXT("blanco")
    };

    for (const auto& BldVal : *BuildingsArr)
    {
        const TSharedPtr<FJsonObject>& Bld = BldVal->AsObject();
        if (!Bld) continue;

        const int32 Id = Bld->HasField(TEXT("id")) ? Bld->GetIntegerField(TEXT("id")) : -1;
        const FString Barrio = Bld->HasField(TEXT("barrio")) ? Bld->GetStringField(TEXT("barrio")) : TEXT("Herriko");

        const TArray<TSharedPtr<FJsonValue>>* VertsArr;
        if (!Bld->TryGetArrayField(TEXT("vertices"), VertsArr) || !VertsArr || VertsArr->Num() < 3) continue;

        float CX = 0, CZ = 0;
        float minX = 1e9f, maxX = -1e9f;
        float minZ = 1e9f, maxZ = -1e9f;
        for (const auto& V : *VertsArr)
        {
            const TSharedPtr<FJsonObject>& Vert = V->AsObject();
            if (!Vert) continue;
            float VX = Vert->GetNumberField(TEXT("x"));
            float VZ = Vert->GetNumberField(TEXT("z"));
            CX += VX;
            CZ += VZ;
            minX = FMath::Min(minX, VX);
            maxX = FMath::Max(maxX, VX);
            minZ = FMath::Min(minZ, VZ);
            maxZ = FMath::Max(maxZ, VZ);
        }
        CX /= VertsArr->Num();
        CZ /= VertsArr->Num();

        float DX = maxX - minX;
        float DZ = maxZ - minZ;
        float DoorRot;
        FVector DoorOffset;

        if (DX > DZ)
        {
            DoorRot = (FMath::FRand() < 0.5f) ? 0.0f : 180.0f;
            float SideZ = (FMath::FRand() < 0.5f) ? minZ : maxZ;
            DoorOffset = FVector(CX, SideZ, 0);
        }
        else
        {
            DoorRot = (FMath::FRand() < 0.5f) ? 90.0f : 270.0f;
            float SideX = (FMath::FRand() < 0.5f) ? minX : maxX;
            DoorOffset = FVector(SideX, CZ, 0);
        }

        FVector DoorPos = UAlsasuaGeoData::UnityaUnreal(FVector(DoorOffset.X + UAlsasuaGeoData::OX, 0.0f, DoorOffset.Z + UAlsasuaGeoData::OZ));
        DoorPos.Z += 110.0f;

        FDoorEntry Puerta;
        Puerta.BuildingId = Id;
        Puerta.Posicion = DoorPos;
        Puerta.Rotacion = DoorRot;
        Puerta.Tipo = (Barrio == TEXT("Herriko") || Barrio == TEXT("Harrobieta")) ?
            TEXT("madera_vieja") : TEXT("moderna");
        Puerta.Color = ColoresPuerta[FMath::RandRange(0, ColoresPuerta.Num() - 1)];
        Puerta.Barrio = Barrio;

        AStaticMeshActor* PuertaActor = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), DoorPos, FRotator(0, DoorRot, 0));
        if (PuertaActor)
        {
            PuertaActor->SetMobility(EComponentMobility::Static);
            PuertaActor->SetActorScale3D(FVector(1.0f, 0.1f, 2.2f));

            UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr,
                TEXT("/Game/EngineBasicShapes/Cube"));
            if (CubeMesh)
                PuertaActor->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);

            UMaterialInterface* PuertaMat = LoadObject<UMaterialInterface>(nullptr,
                TEXT("/Game/Materiales/M_Puerta"));
            if (!PuertaMat)
                PuertaMat = LoadObject<UMaterialInterface>(nullptr,
                    TEXT("/Game/Materiales/M_Madera"));

            if (PuertaMat)
                PuertaActor->GetStaticMeshComponent()->SetMaterial(0, PuertaMat);

#if WITH_EDITOR
            PuertaActor->SetActorLabel(*FString::Printf(TEXT("Puerta_%d_%s"), Id, *Barrio.Left(6)));
#endif
        }

        if (Barrio == TEXT("Herriko") && FMath::FRand() < 0.3f)
        {
            FVector ToldoPos = DoorPos;
            ToldoPos.Z += 130.0f;

            AStaticMeshActor* ToldoActor = World->SpawnActor<AStaticMeshActor>(
                AStaticMeshActor::StaticClass(), ToldoPos, FRotator(0, DoorRot, 0));
            if (ToldoActor)
            {
                ToldoActor->SetMobility(EComponentMobility::Static);
                ToldoActor->SetActorScale3D(FVector(2.0f, 1.0f, 0.05f));

                UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr,
                    TEXT("/Game/EngineBasicShapes/Plane"));
                if (PlaneMesh)
                    ToldoActor->GetStaticMeshComponent()->SetStaticMesh(PlaneMesh);

                UMaterialInterface* ToldoMat = LoadObject<UMaterialInterface>(nullptr,
                    TEXT("/Game/Materiales/M_Toldo"));
                if (ToldoMat)
                    ToldoActor->GetStaticMeshComponent()->SetMaterial(0, ToldoMat);

#if WITH_EDITOR
                ToldoActor->SetActorLabel(*FString::Printf(TEXT("ToldoEntrada_%d"), Id));
#endif
            }
        }

        Puertas.Add(Puerta);
        Placed++;
    }

    UE_LOG(LogTemp, Log, TEXT("Doors: %d puertas y entradas colocadas"), Placed);
    return Placed;
}
