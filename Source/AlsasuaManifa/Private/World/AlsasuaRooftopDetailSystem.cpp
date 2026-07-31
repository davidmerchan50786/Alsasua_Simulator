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

    TArray<TSharedPtr<FJsonValue>> BuildingsArr;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    if (!FJsonSerializer::Deserialize(Reader, BuildingsArr) || BuildingsArr.Num() == 0) return 0;

    UWorld* World = GetWorld();
    if (!World) return 0;

    Items.Empty();
    int32 Placed = 0;

    for (const auto& BldVal : BuildingsArr)
    {
        const TSharedPtr<FJsonObject>& Bld = BldVal->AsObject();
        if (!Bld) continue;

        const int32 Id = Bld->GetIntegerField(TEXT("id"));
        const float Height = Bld->HasField(TEXT("height")) ? Bld->GetNumberField(TEXT("height")) : 10.0f;
        const FString Barrio = Bld->HasField(TEXT("barrio")) ? Bld->GetStringField(TEXT("barrio")) : TEXT("");
        const FString RoofTipo = Bld->HasField(TEXT("roof_tipo_real")) ? Bld->GetStringField(TEXT("roof_tipo_real")) : TEXT("desconocido");

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

        FVector RoofCenter = UAlsasuaGeoData::RelLocalToUE5(FVector(CX, 0.0f, CZ));
        RoofCenter.Z += Height * 100.0f;

        const bool bFlatRoof = RoofTipo.Contains(TEXT("cemento"));
        const bool bPitchedRoof = RoofTipo.Contains(TEXT("pizarra")) || RoofTipo.Contains(TEXT("teja"));

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

        if (bPitchedRoof && FMath::FRand() < 0.4f)
        {
            float OffX = FMath::RandRange(-200.0f, 200.0f);
            float OffZ = FMath::RandRange(-200.0f, 200.0f);
            CrearItem(TEXT("antena"),
                TEXT("/Engine/EngineMeshes/Cylinder"),
                TEXT("/Engine/EngineMaterials/DefaultMaterial"),
                0.05f, 0.05f, 3.0f, OffX, OffZ);
        }

        if (bPitchedRoof && FMath::FRand() < 0.6f)
        {
            float OffX = FMath::RandRange(-150.0f, 150.0f);
            float OffZ = FMath::RandRange(-150.0f, 150.0f);
            CrearItem(TEXT("chimenea"),
                TEXT("/Engine/EngineMeshes/Cube"),
                TEXT("/Engine/EngineMaterials/DefaultMaterial"),
                0.5f, 0.5f, 1.2f, OffX, OffZ);
        }

        if (bFlatRoof && FMath::FRand() < 0.3f)
        {
            float OffX = FMath::RandRange(-100.0f, 100.0f);
            float OffZ = FMath::RandRange(-100.0f, 100.0f);
            CrearItem(TEXT("deposito_agua"),
                TEXT("/Engine/EngineMeshes/Cylinder"),
                TEXT("/Engine/EngineMaterials/DefaultMaterial"),
                1.0f, 1.0f, 1.5f, OffX, OffZ);
        }

        if (bFlatRoof && FMath::FRand() < 0.2f)
        {
            float OffX = FMath::RandRange(-200.0f, 200.0f);
            float OffZ = FMath::RandRange(-200.0f, 200.0f);
            CrearItem(TEXT("placa_solar"),
                TEXT("/Engine/EngineMeshes/Plane"),
                TEXT("/Engine/EngineMaterials/DefaultMaterial"),
                2.0f, 1.5f, 0.02f, OffX, OffZ);
        }

        if (FMath::FRand() < 0.15f)
        {
            float OffX = FMath::RandRange(-150.0f, 150.0f);
            float OffZ = FMath::RandRange(-150.0f, 150.0f);
            CrearItem(TEXT("satelital"),
                TEXT("/Engine/EngineMeshes/Cylinder"),
                TEXT("/Engine/EngineMaterials/DefaultMaterial"),
                0.3f, 0.3f, 0.8f, OffX, OffZ);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Rooftop: %d detalles de cubierta (antenas, chimeneas, depósitos, placas solares)"), Placed);
    return Placed;
}
