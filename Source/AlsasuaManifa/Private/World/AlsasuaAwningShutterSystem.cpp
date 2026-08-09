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
    const FString FacPath = FPaths::ProjectContentDir() + TEXT("Datos/building_facades.json");
    TArray<FString> FacLines;
    TMap<int32, int32> VentanasConPersiana;
    TMap<int32, bool> TieneToldo;

    if (FFileHelper::LoadFileToStringArray(FacLines, *FacPath))
    {
        FString FacJs;
        for (const FString& L : FacLines) FacJs += L;

        TArray<TSharedPtr<FJsonValue>> FacArr;
        TSharedRef<TJsonReader<>> FacRd = TJsonReaderFactory<>::Create(FacJs);
        if (FJsonSerializer::Deserialize(FacRd, FacArr))
        {
            for (const auto& FV : FacArr)
            {
                const TSharedPtr<FJsonObject>& FO = FV->AsObject();
                if (!FO) continue;
                const int32 BId = FO->GetIntegerField(TEXT("building_id"));

                int32 PersianaCount = 0;
                bool bToldo = false;
                const TArray<TSharedPtr<FJsonValue>>* VentArr;
                if (FO->TryGetArrayField(TEXT("ventanas"), VentArr))
                {
                    for (const auto& WV : *VentArr)
                    {
                        const TSharedPtr<FJsonObject>& WO = WV->AsObject();
                        if (!WO) continue;
                        if (WO->GetBoolField(TEXT("con_persiana"))) PersianaCount++;
                        if (WO->GetBoolField(TEXT("con_balcon"))) bToldo = true;
                    }
                }

                const TArray<TSharedPtr<FJsonValue>>* TieArr;
                if (FO->TryGetArrayField(TEXT("tiendas_planta_baja"), TieArr))
                {
                    if (TieArr->Num() > 0) bToldo = true;
                }

                VentanasConPersiana.Add(BId, PersianaCount);
                TieneToldo.Add(BId, bToldo);
            }
        }
    }

    const FString BldPath = FPaths::ProjectContentDir() + TEXT("Datos/buildings_final.json");
    FString JsonStr;
    if (!FFileHelper::LoadFileToString(JsonStr, *BldPath)) return 0;

    TArray<TSharedPtr<FJsonValue>> BuildingsArr;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    if (!FJsonSerializer::Deserialize(Reader, BuildingsArr)) return 0;

    UWorld* World = GetWorld();
    if (!World) return 0;

    Toldos.Empty();
    Persianas.Empty();

    int32 Placed = 0;

    for (const auto& BldVal : BuildingsArr)
    {
        const TSharedPtr<FJsonObject>& Bld = BldVal->AsObject();
        if (!Bld) continue;

        const int32 Id = Bld->GetIntegerField(TEXT("id"));
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
        FVector Center = UAlsasuaGeoData::RelLocalToUE5(FVector(CX, 0.0f, CZ));

        bool bHasAwning = false;
        if (bool* Found = TieneToldo.Find(Id)) bHasAwning = *Found;

        if (bHasAwning && FMath::FRand() < 0.7f)
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
            Toldo.ColorToldo = TEXT("crema");
            Toldo.Barrio = TEXT("");
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
                    TEXT("/Engine/BasicShapes/Plane.Plane"));
                if (PlaneMesh)
                    ToldoActor->GetStaticMeshComponent()->SetStaticMesh(PlaneMesh);
            }
            Toldos.Add(Toldo);
            Placed++;
        }

        int32 NumPersianasReales = 0;
        if (int32* Found = VentanasConPersiana.Find(Id))
            NumPersianasReales = *Found;

        if (NumPersianasReales == 0)
            NumPersianasReales = FMath::Max(0, FMath::CeilToInt(Height / 3.0f) - 1);

        for (int32 p = 0; p < NumPersianasReales; p++)
        {
            float Rot = FMath::RandRange(0.0f, 360.0f);
            FVector PersianaPos = Center;
            PersianaPos.Z += (p + 1) * 300.0f + 150.0f;

            FShutterEntry Persiana;
            Persiana.BuildingId = Id;
            Persiana.Posicion = PersianaPos;
            Persiana.Rotacion = Rot;
            Persiana.Color = TEXT("blanco");
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
                    TEXT("/Engine/BasicShapes/Plane.Plane"));
                if (PlaneMesh)
                    PersianaActor->GetStaticMeshComponent()->SetStaticMesh(PlaneMesh);
            }
            Persianas.Add(Persiana);
            Placed++;
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Awnings: %d toldos + %d persianas (facade-driven)"), Toldos.Num(), Persianas.Num());
    return Placed;
}
