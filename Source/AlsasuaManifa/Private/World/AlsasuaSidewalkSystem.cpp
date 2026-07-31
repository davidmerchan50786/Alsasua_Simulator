#include "World/AlsasuaSidewalkSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"

void UAlsasuaSidewalkSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

int32 UAlsasuaSidewalkSystem::GenerarAceras()
{
    const FString JsonPath = FPaths::ProjectContentDir() + TEXT("Datos/roads_unity.json");
    FString JsonStr;
    if (!FFileHelper::LoadFileToString(JsonStr, *JsonPath)) return 0;

    TSharedPtr<FJsonValue> RootVal;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    if (!FJsonSerializer::Deserialize(Reader, RootVal) || !RootVal.IsValid()) return 0;

    const TArray<TSharedPtr<FJsonValue>>* RoadsArr;
    if (!RootVal->TryGetArray(RoadsArr)) return 0;

    UWorld* World = GetWorld();
    if (!World) return 0;

    int32 Placed = 0;
    Acera.Empty();

    for (const auto& RoadVal : *RoadsArr)
    {
        const TSharedPtr<FJsonObject>& Road = RoadVal->AsObject();
        if (!Road) continue;

        const FString Type = Road->HasField(TEXT("type")) ? Road->GetStringField(TEXT("type")) : TEXT("");
        if (Type == TEXT("footway") || Type == TEXT("path") || Type == TEXT("track")) continue;

        const FString Calle = Road->HasField(TEXT("name")) ? Road->GetStringField(TEXT("name")) : TEXT("");
        const FString Barrio = Road->HasField(TEXT("barrio")) ? Road->GetStringField(TEXT("barrio")) : TEXT("");
        const float RoadWidth = Road->HasField(TEXT("width")) ? Road->GetNumberField(TEXT("width")) : 6.0f;

        const TArray<TSharedPtr<FJsonValue>>* PointsArr;
        if (!Road->TryGetArrayField(TEXT("points"), PointsArr) || !PointsArr || PointsArr->Num() < 2) continue;

        for (int32 i = 0; i < PointsArr->Num() - 1; i++)
        {
            const TSharedPtr<FJsonObject>& P0 = (*PointsArr)[i]->AsObject();
            const TSharedPtr<FJsonObject>& P1 = (*PointsArr)[i + 1]->AsObject();
            if (!P0 || !P1) continue;

            FVector Loc0 = UAlsasuaGeoData::RelLocalToUE5(FVector(
                P0->GetNumberField(TEXT("x")), 0.0f, P0->GetNumberField(TEXT("z"))));
            FVector Loc1 = UAlsasuaGeoData::RelLocalToUE5(FVector(
                P1->GetNumberField(TEXT("x")), 0.0f, P1->GetNumberField(TEXT("z"))));

            FVector Centro = (Loc0 + Loc1) * 0.5f;
            FVector Direccion = (Loc1 - Loc0).GetSafeNormal();
            FVector Normal = FVector(-Direccion.Y, Direccion.X, 0);
            float Largo = FVector::Distance(Loc0, Loc1);

            float OffsetX = RoadWidth * 100.0f * 0.5f + AnchoAceras * 0.5f;

            for (int32 Side = -1; Side <= 1; Side += 2)
            {
                FVector SidewalkCenter = Centro + Normal * OffsetX * Side;
                SidewalkCenter.Z += AlturaBordillo;

                float Angle = FMath::RadiansToDegrees(FMath::Atan2(Direccion.Y, Direccion.X));

                AStaticMeshActor* SidewalkActor = World->SpawnActor<AStaticMeshActor>(
                    AStaticMeshActor::StaticClass(), SidewalkCenter, FRotator(0, Angle, 0));
                if (!SidewalkActor) continue;

                SidewalkActor->SetMobility(EComponentMobility::Static);
                SidewalkActor->SetActorScale3D(FVector(Largo / 100.0f, AnchoAceras / 100.0f, 0.15f));

                UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr,
                    TEXT("/Game/EngineBasicShapes/Plane"));
                if (PlaneMesh)
                    SidewalkActor->GetStaticMeshComponent()->SetStaticMesh(PlaneMesh);

                UMaterialInterface* SidewalkMat = nullptr;
                if (Barrio == TEXT("Herriko") || Barrio == TEXT("Harrobieta"))
                    SidewalkMat = LoadObject<UMaterialInterface>(nullptr,
                        TEXT("/Game/Materiales/M_Acera_Piedra"));
                else
                    SidewalkMat = LoadObject<UMaterialInterface>(nullptr,
                        TEXT("/Game/Materiales/M_Acera_Hormigon"));

                if (!SidewalkMat)
                    SidewalkMat = LoadObject<UMaterialInterface>(nullptr,
                        TEXT("/Game/Materiales/M_Pavimento"));

                if (!SidewalkMat)
                    SidewalkMat = LoadObject<UMaterialInterface>(nullptr,
                        TEXT("/Game/Materiales/M_Terreno_Acera.M_Terreno_Acera"));

                if (SidewalkMat)
                    SidewalkActor->GetStaticMeshComponent()->SetMaterial(0, SidewalkMat);

#if WITH_EDITOR
                SidewalkActor->SetActorLabel(*FString::Printf(TEXT("Acera_%s_%d"), *Calle.Left(10), Placed));
#endif

                FSidewalkSegment Seg;
                Seg.Inicio = Loc0;
                Seg.Fin = Loc1;
                Seg.Ancho = AnchoAceras;
                Seg.Calle = Calle;
                Seg.Barrio = Barrio;
                Seg.Altura = AlturaBordillo;
                Acera.Add(Seg);

                Placed++;
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Sidewalks: %d segmentos de acera generados"), Placed);
    return Placed;
}
