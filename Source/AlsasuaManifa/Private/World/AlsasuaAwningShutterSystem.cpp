#include "World/AlsasuaAwningShutterSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"

void UAlsasuaAwningShutterSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

int32 UAlsasuaAwningShutterSystem::ColocarToldosYPersianas()
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

    Toldos.Empty();
    Persianas.Empty();

    const TArray<FString> ColoresToldo = {
        TEXT("rojo"), TEXT("verde"), TEXT("azul"), TEXT("crema"),
        TEXT("marron"), TEXT("blanco"), TEXT("naranja")
    };

    const TArray<FString> ColoresPersiana = {
        TEXT("blanco"), TEXT("verde_oscuro"), TEXT("marron"),
        TEXT("gris"), TEXT("azul_oscuro"), TEXT("beige")
    };

    int32 Placed = 0;

    for (const auto& BldVal : *BuildingsArr)
    {
        const TSharedPtr<FJsonObject>& Bld = BldVal->AsObject();
        if (!Bld) continue;

        const int32 Id = Bld->HasField(TEXT("id")) ? Bld->GetIntegerField(TEXT("id")) : -1;
        const FString Barrio = Bld->HasField(TEXT("barrio")) ? Bld->GetStringField(TEXT("barrio")) : TEXT("Herriko");
        const float Height = Bld->HasField(TEXT("height")) ? Bld->GetNumberField(TEXT("height")) : 10.0f;

        const TArray<TSharedPtr<FJsonValue>>* VertsArr;
        if (!Bld->TryGetArrayField(TEXT("vertices"), VertsArr) || !VertsArr || VertsArr->Num() < 3) continue;

        float CX = 0, CZ = 0;
        for (const auto& V : *VertsArr)
        {
            const TSharedPtr<FJsonObject>& Vert = V->AsObject();
            if (!Vert) continue;
            CX += Vert->GetNumberField(TEXT("x"));
            CZ += Vert->GetNumberField(TEXT("z"));
        }
        CX /= VertsArr->Num();
        CZ /= VertsArr->Num();

        FVector Center = UAlsasuaGeoData::UnityaUnreal(FVector(CX + UAlsasuaGeoData::OX, 0.0f, CZ + UAlsasuaGeoData::OZ));

        if (FMath::FRand() < ProbabilidadToldo)
        {
            float Rot = FMath::RandRange(0.0f, 360.0f);
            FVector ToldoPos = Center;
            ToldoPos.Z += 300.0f;

            FAwningEntry Toldo;
            Toldo.BuildingId = Id;
            Toldo.Posicion = ToldoPos;
            Toldo.Rotacion = Rot;
            Toldo.Ancho = FMath::RandRange(150.0f, 400.0f);
            Toldo.Profundidad = FMath::RandRange(80.0f, 150.0f);
            Toldo.ColorToldo = ColoresToldo[FMath::RandRange(0, ColoresToldo.Num() - 1)];
            Toldo.Barrio = Barrio;
            Toldo.bPlegado = (FMath::FRand() < 0.3f);

            AStaticMeshActor* ToldoActor = World->SpawnActor<AStaticMeshActor>(
                AStaticMeshActor::StaticClass(), ToldoPos, FRotator(0, Rot, 0));
            if (ToldoActor)
            {
                ToldoActor->SetMobility(EComponentMobility::Static);
                float SX = Toldo.Ancho / 100.0f;
                float SZ = Toldo.bPlegado ? 0.1f : Toldo.Profundidad / 100.0f;
                ToldoActor->SetActorScale3D(FVector(SX, SZ, 0.05f));

                UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr,
                    TEXT("/Game/EngineBasicShapes/Plane"));
                if (PlaneMesh)
                    ToldoActor->GetStaticMeshComponent()->SetStaticMesh(PlaneMesh);

                UMaterialInterface* ToldoMat = LoadObject<UMaterialInterface>(nullptr,
                    TEXT("/Game/Materiales/M_Toldo"));
                if (ToldoMat)
                    ToldoActor->GetStaticMeshComponent()->SetMaterial(0, ToldoMat);

#if WITH_EDITOR
                ToldoActor->SetActorLabel(*FString::Printf(TEXT("Toldo_%d_%s"), Id,
                    *Toldo.ColorToldo));
#endif
            }

            Toldos.Add(Toldo);
            Placed++;
        }

        int32 NumPersianas = FMath::Max(0, FMath::CeilToInt(Height / 3.0f) - 1);
        for (int32 p = 0; p < NumPersianas; p++)
        {
            if (FMath::FRand() > ProbabilidadPersiana) continue;

            float Rot = FMath::RandRange(0.0f, 360.0f);
            FVector PersianaPos = Center;
            PersianaPos.Z += (p + 1) * 300.0f + 150.0f;

            FShutterEntry Persiana;
            Persiana.BuildingId = Id;
            Persiana.Posicion = PersianaPos;
            Persiana.Rotacion = Rot;
            Persiana.Color = ColoresPersiana[FMath::RandRange(0, ColoresPersiana.Num() - 1)];
            Persiana.bAbierto = (FMath::FRand() < 0.5f);

            AStaticMeshActor* PersianaActor = World->SpawnActor<AStaticMeshActor>(
                AStaticMeshActor::StaticClass(), PersianaPos, FRotator(0, Rot, 0));
            if (PersianaActor)
            {
                PersianaActor->SetMobility(EComponentMobility::Static);
                float SX = 1.0f;
                float SZ = Persiana.bAbierto ? 0.05f : 1.2f;
                PersianaActor->SetActorScale3D(FVector(SX, 0.05f, SZ));

                UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr,
                    TEXT("/Game/EngineBasicShapes/Plane"));
                if (PlaneMesh)
                    PersianaActor->GetStaticMeshComponent()->SetStaticMesh(PlaneMesh);

                UMaterialInterface* PersianaMat = LoadObject<UMaterialInterface>(nullptr,
                    TEXT("/Game/Materiales/M_Persiana"));
                if (PersianaMat)
                    PersianaActor->GetStaticMeshComponent()->SetMaterial(0, PersianaMat);

#if WITH_EDITOR
                PersianaActor->SetActorLabel(*FString::Printf(TEXT("Persiana_%d_%d_%s"), Id, p,
                    *Persiana.Color));
#endif
            }

            Persianas.Add(Persiana);
            Placed++;
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Awnings: %d toldos + %d persianas colocados"), Toldos.Num(), Persianas.Num());
    return Placed;
}
