#include "World/AlsasuaRooftopDetailSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"

void UAlsasuaRooftopDetailSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

int32 UAlsasuaRooftopDetailSystem::ColocarDetallesCubierta()
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

    Items.Empty();
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

        FVector RoofCenter = UAlsasuaGeoData::UnityaUnreal(FVector(CX + UAlsasuaGeoData::OX, 0.0f, CZ + UAlsasuaGeoData::OZ));
        RoofCenter.Z += Height * 100.0f;

        auto CrearItem = [&](const FString& Tipo, const TCHAR* MeshPath,
            const TCHAR* MatPath, float SX, float SY, float SZ, float OffX, float OffZ)
        {
            FVector Pos = RoofCenter + FVector(OffX, OffZ, 0);

            AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(
                AStaticMeshActor::StaticClass(), Pos, FRotator(0, FMath::RandRange(0.0f, 360.0f), 0));
            if (!Actor) return;

            Actor->SetMobility(EComponentMobility::Static);
            Actor->SetActorScale3D(FVector(SX, SY, SZ));

            UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, MeshPath);
            if (Mesh) Actor->GetStaticMeshComponent()->SetStaticMesh(Mesh);

            UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, MatPath);
            if (Mat) Actor->GetStaticMeshComponent()->SetMaterial(0, Mat);

#if WITH_EDITOR
            Actor->SetActorLabel(*FString::Printf(TEXT("Cubierta_%s_%d_%s"),
                *Tipo.Left(8), Id, *Barrio.Left(6)));
#endif

            FRooftopItem Item;
            Item.BuildingId = Id;
            Item.Tipo = Tipo;
            Item.Posicion = Pos;
            Item.Rotacion = Actor->GetActorRotation().Yaw;
            Item.Escala = SX;
            Item.Barrio = Barrio;
            Items.Add(Item);
            Placed++;
        };

        if (FMath::FRand() < ProbAntena)
        {
            float OffX = FMath::RandRange(-200.0f, 200.0f);
            float OffZ = FMath::RandRange(-200.0f, 200.0f);
            CrearItem(TEXT("antena"),
                TEXT("/Game/EngineBasicShapes/Cylinder"),
                TEXT("/Game/Materiales/M_Metal_Gris"),
                0.05f, 0.05f, 3.0f, OffX, OffZ);
        }

        if (FMath::FRand() < ProbChimenea)
        {
            float OffX = FMath::RandRange(-150.0f, 150.0f);
            float OffZ = FMath::RandRange(-150.0f, 150.0f);
            CrearItem(TEXT("chimenea"),
                TEXT("/Game/EngineBasicShapes/Cube"),
                TEXT("/Game/Materiales/M_Ladrillo_Rojo"),
                0.5f, 0.5f, 1.2f, OffX, OffZ);
        }

        if (FMath::FRand() < ProbDeposito)
        {
            float OffX = FMath::RandRange(-100.0f, 100.0f);
            float OffZ = FMath::RandRange(-100.0f, 100.0f);
            CrearItem(TEXT("deposito_agua"),
                TEXT("/Game/EngineBasicShapes/Cylinder"),
                TEXT("/Game/Materiales/M_Metal_Negro"),
                1.0f, 1.0f, 1.5f, OffX, OffZ);
        }

        if (FMath::FRand() < ProbPlacaSolar)
        {
            float OffX = FMath::RandRange(-200.0f, 200.0f);
            float OffZ = FMath::RandRange(-200.0f, 200.0f);
            CrearItem(TEXT("placa_solar"),
                TEXT("/Game/EngineBasicShapes/Plane"),
                TEXT("/Game/Materiales/M_SolarPanel"),
                2.0f, 1.5f, 0.02f, OffX, OffZ);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Rooftop: %d detalles de cubierta (antenas, chimeneas, depósitos, placas solares)"), Placed);
    return Placed;
}
