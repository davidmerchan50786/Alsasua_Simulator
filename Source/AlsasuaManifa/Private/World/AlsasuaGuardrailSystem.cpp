#include "World/AlsasuaGuardrailSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"

void UAlsasuaGuardrailSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

int32 UAlsasuaGuardrailSystem::ColocarBarandillas()
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

    Barandillas.Empty();
    int32 Placed = 0;

    for (const auto& RoadVal : *RoadsArr)
    {
        const TSharedPtr<FJsonObject>& Road = RoadVal->AsObject();
        if (!Road) continue;

        const FString Type = Road->HasField(TEXT("type")) ? Road->GetStringField(TEXT("type")) : TEXT("");
        const FString Barrio = Road->HasField(TEXT("barrio")) ? Road->GetStringField(TEXT("barrio")) : TEXT("");

        bool bNeedsGuardrail = (Type == TEXT("bridge") || Type == TEXT("residential") ||
            Barrio == TEXT("Errota") || Barrio == TEXT("Monte"));

        if (!bNeedsGuardrail) continue;

        const TArray<TSharedPtr<FJsonValue>>* PointsArr;
        if (!Road->TryGetArrayField(TEXT("points"), PointsArr) || !PointsArr || PointsArr->Num() < 2) continue;

        for (int32 i = 0; i < PointsArr->Num() - 1; i++)
        {
            const TSharedPtr<FJsonObject>& P0 = (*PointsArr)[i]->AsObject();
            const TSharedPtr<FJsonObject>& P1 = (*PointsArr)[i + 1]->AsObject();
            if (!P0 || !P1) continue;

            FVector Loc0 = UAlsasuaGeoData::UnityaUnreal(FVector(
                P0->GetNumberField(TEXT("x")) + UAlsasuaGeoData::OX, 0.0f, P0->GetNumberField(TEXT("z")) + UAlsasuaGeoData::OZ));
            FVector Loc1 = UAlsasuaGeoData::UnityaUnreal(FVector(
                P1->GetNumberField(TEXT("x")) + UAlsasuaGeoData::OX, 0.0f, P1->GetNumberField(TEXT("z")) + UAlsasuaGeoData::OZ));

            FVector Centro = (Loc0 + Loc1) * 0.5f;
            FVector Direccion = (Loc1 - Loc0).GetSafeNormal();
            FVector Normal = FVector(-Direccion.Y, Direccion.X, 0);
            float Largo = FVector::Distance(Loc0, Loc1);
            float Angle = FMath::RadiansToDegrees(FMath::Atan2(Direccion.Y, Direccion.X));

            float Offset = 300.0f;
            FVector GuardrailPos = Centro + Normal * Offset;
            GuardrailPos.Z += AlturaBarandilla;

            FGuardrail GR;
            GR.Inicio = Loc0;
            GR.Fin = Loc1;
            GR.Tipo = Type;
            GR.Barrio = Barrio;

            AStaticMeshActor* GuardrailActor = World->SpawnActor<AStaticMeshActor>(
                AStaticMeshActor::StaticClass(), GuardrailPos, FRotator(0, Angle, 0));
            if (GuardrailActor)
            {
                GuardrailActor->SetMobility(EComponentMobility::Static);
                GuardrailActor->SetActorScale3D(FVector(Largo / 100.0f, 0.15f, AlturaBarandilla / 100.0f));

                UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr,
                    TEXT("/Game/EngineBasicShapes/Cube"));
                if (CubeMesh)
                    GuardrailActor->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);

                UMaterialInterface* GRMat = LoadObject<UMaterialInterface>(nullptr,
                    TEXT("/Game/Materiales/M_Metal_Guardia"));
                if (!GRMat)
                    GRMat = LoadObject<UMaterialInterface>(nullptr,
                        TEXT("/Game/Materiales/M_Metal"));

                if (GRMat)
                    GuardrailActor->GetStaticMeshComponent()->SetMaterial(0, GRMat);

#if WITH_EDITOR
                GuardrailActor->SetActorLabel(*FString::Printf(TEXT("Barandilla_%s_%d"),
                    *Barrio.Left(8), Placed));
#endif
            }

            Barandillas.Add(GR);
            Placed++;
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Guardrails: %d barandillas en puentes y zonas peligrosas"), Placed);
    return Placed;
}
