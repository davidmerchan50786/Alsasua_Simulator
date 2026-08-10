#include "World/AlsasuaOverheadCableSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"

void UAlsasuaOverheadCableSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

int32 UAlsasuaOverheadCableSystem::ColocarCables()
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

    Cables.Empty();
    int32 Placed = 0;

    for (const auto& RoadVal : *RoadsArr)
    {
        if (Placed >= MaxCables) break;

        const TSharedPtr<FJsonObject>& Road = RoadVal->AsObject();
        if (!Road) continue;

        const FString Calle = Road->HasField(TEXT("name")) ? Road->GetStringField(TEXT("name")) : TEXT("");
        if (Calle.IsEmpty()) continue;

        const TArray<TSharedPtr<FJsonValue>>* PointsArr;
        if (!Road->TryGetArrayField(TEXT("points"), PointsArr) || !PointsArr || PointsArr->Num() < 4) continue;

        int32 Step = FMath::Max(1, PointsArr->Num() / 4);
        for (int32 i = 0; i < PointsArr->Num() - Step; i += Step)
        {
            if (Placed >= MaxCables) break;

            const TSharedPtr<FJsonObject>& P0 = (*PointsArr)[i]->AsObject();
            const TSharedPtr<FJsonObject>& P1 = (*PointsArr)[FMath::Min(i + Step, PointsArr->Num() - 1)]->AsObject();
            if (!P0 || !P1) continue;

            FVector Loc0 = UAlsasuaGeoData::RelLocalToUE5(FVector(
                P0->GetNumberField(TEXT("x")), 0.0f, P0->GetNumberField(TEXT("z"))));
            FVector Loc1 = UAlsasuaGeoData::RelLocalToUE5(FVector(
                P1->GetNumberField(TEXT("x")), 0.0f, P1->GetNumberField(TEXT("z"))));

            Loc0.Z += AlturaCables;
            Loc1.Z += AlturaCables;

            FVector Centro = (Loc0 + Loc1) * 0.5f;
            Centro.Z -= 30.0f;
            FVector Direccion = (Loc1 - Loc0).GetSafeNormal();
            float Largo = FVector::Distance(Loc0, Loc1);
            float Angle = FMath::RadiansToDegrees(FMath::Atan2(Direccion.Y, Direccion.X));

            AStaticMeshActor* CableActor = World->SpawnActor<AStaticMeshActor>(
                AStaticMeshActor::StaticClass(), Centro, FRotator(0, Angle, 0));
            if (!CableActor) continue;

            CableActor->SetMobility(EComponentMobility::Static);
            CableActor->SetActorScale3D(FVector(Largo / 100.0f, 0.02f, 0.02f));

            UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr,
                TEXT("/Engine/BasicShapes/Cube.Cube"));
            if (CubeMesh)
                CableActor->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);

            UMaterialInterface* CableMat = LoadObject<UMaterialInterface>(nullptr,
                TEXT("/Game/Materiales/M_Metal_Negro"));
            if (CableMat)
                CableActor->GetStaticMeshComponent()->SetMaterial(0, CableMat);

            for (int32 P = 0; P < 3; P++)
            {
                FVector PostPos = FMath::Lerp(Loc0, Loc1, (float)P / 2.0f);
                PostPos.Z = Loc0.Z;

                AStaticMeshActor* PostActor = World->SpawnActor<AStaticMeshActor>(
                    AStaticMeshActor::StaticClass(), PostPos - FVector(0, 0, AlturaCables * 0.5f),
                    FRotator::ZeroRotator);
                if (PostActor)
                {
                    PostActor->SetMobility(EComponentMobility::Static);
                    PostActor->SetActorScale3D(FVector(0.12f, 0.12f, AlturaCables / 100.0f));

                    UStaticMesh* CylinderMesh = LoadObject<UStaticMesh>(nullptr,
                        TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
                    if (CylinderMesh)
                        PostActor->GetStaticMeshComponent()->SetStaticMesh(CylinderMesh);

                    UMaterialInterface* PostMat = LoadObject<UMaterialInterface>(nullptr,
                        TEXT("/Game/Materiales/M_Hormigon"));
                    if (PostMat)
                        PostActor->GetStaticMeshComponent()->SetMaterial(0, PostMat);

#if WITH_EDITOR
                    PostActor->SetActorLabel(*FString::Printf(TEXT("Poste_%s_%d"), *Calle.Left(8), P));
#endif
                }
            }

            FOverheadCable Cable;
            Cable.Inicio = Loc0;
            Cable.Fin = Loc1;
            Cable.Caida = 30.0f;
            Cable.Tipo = TEXT("electrico");
            Cable.Calle = Calle;
            Cables.Add(Cable);
            Placed++;
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Cables: %d tendidos eléctricos aéreos"), Placed);
    return Placed;
}
