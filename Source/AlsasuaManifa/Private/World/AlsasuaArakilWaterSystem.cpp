#include "World/AlsasuaArakilWaterSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/StaticMeshActor.h"

void UAlsasuaArakilWaterSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CargarTramosRio();
}

void UAlsasuaArakilWaterSystem::Deinitialize()
{
    Tramos.Empty();
    bCargado = false;
    Super::Deinitialize();
}

bool UAlsasuaArakilWaterSystem::CargarTramosRio()
{
    const FString JsonPath = FPaths::ProjectContentDir() + TEXT("Datos/waterways_unity.json");
    FString JsonStr;
    if (!FFileHelper::LoadFileToString(JsonStr, *JsonPath))
    {
        UE_LOG(LogTemp, Error, TEXT("ArakilWater: No se pudo cargar waterways_unity.json"));
        return false;
    }

    TSharedPtr<FJsonValue> RootVal;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    if (!FJsonSerializer::Deserialize(Reader, RootVal) || !RootVal.IsValid()) return false;

    const TArray<TSharedPtr<FJsonValue>>* RiversArr;
    if (!RootVal->TryGetArray(RiversArr)) return false;

    for (const auto& RiverVal : *RiversArr)
    {
        const TSharedPtr<FJsonObject>& River = RiverVal->AsObject();
        if (!River) continue;

        const FString Name = River->GetStringField(TEXT("name"));
        const float Width = River->HasField(TEXT("width")) ? River->GetNumberField(TEXT("width")) : 8.0f;

        const TArray<TSharedPtr<FJsonValue>>* PtsArr;
        if (!River->TryGetArrayField(TEXT("pts"), PtsArr) || !PtsArr || PtsArr->Num() < 6) continue;

        TArray<FVector> Points;
        for (int32 i = 0; i + 2 < PtsArr->Num(); i += 3)
        {
            float PX = (*PtsArr)[i]->AsNumber();
            float PY = (*PtsArr)[i + 1]->AsNumber();
            float PZ = (*PtsArr)[i + 2]->AsNumber();
            // pts es plano [x,y,z,...], así que PY ya es la componente vertical:
            // el orden que espera UnityaUnreal. // ejes ok
            Points.Add(UAlsasuaGeoData::UnityaUnreal(FVector(PX, PY, PZ)));
        }

        for (int32 i = 0; i < Points.Num() - 1; i++)
        {
            const FVector& Loc0 = Points[i];
            const FVector& Loc1 = Points[i + 1];
            FVector Centro = (Loc0 + Loc1) * 0.5f;
            float Largo = FVector::Distance(Loc0, Loc1);

            FWaterSegment Seg;
            Seg.Centro = Centro;
            Seg.Ancho = Width * 100.0f;
            Seg.Largo = Largo;
            Seg.Profundidad = 200.0f;
            Seg.VelocidadFlujo = WaterSpeed;
            Seg.ColorSuperficie = RiverColor;
            Seg.ColorProfundo = FLinearColor(RiverColor.R * 0.3f, RiverColor.G * 0.3f, RiverColor.B * 0.3f, 0.95f);
            Seg.ColorEspuma = FLinearColor(0.8f, 0.85f, 0.9f, FoamIntensity);
            Seg.Turbidez = 0.3f;

            Tramos.Add(Seg);
        }
    }

    bCargado = true;
    UE_LOG(LogTemp, Log, TEXT("ArakilWater: %d tramos del río Arakil cargados"), Tramos.Num());
    return true;
}

int32 UAlsasuaArakilWaterSystem::GenerarMallaAgua()
{
    if (!bCargado && !CargarTramosRio()) return 0;

    UWorld* World = GetWorld();
    if (!World) return 0;

    int32 Placed = 0;
    for (const FWaterSegment& Seg : Tramos)
    {
        AStaticMeshActor* WaterActor = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), Seg.Centro, FRotator::ZeroRotator);
        if (!WaterActor) continue;

        WaterActor->SetMobility(EComponentMobility::Movable);
        WaterActor->SetActorScale3D(FVector(Seg.Largo / 100.0f, Seg.Ancho / 100.0f, 0.05f));

        UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr,
            TEXT("/Engine/BasicShapes/Plane.Plane"));
        if (PlaneMesh)
            WaterActor->GetStaticMeshComponent()->SetStaticMesh(PlaneMesh);

        UMaterialInterface* WaterMat = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/Materiales/M_AguaRio"));
        if (!WaterMat)
            WaterMat = LoadObject<UMaterialInterface>(nullptr,
                TEXT("/Game/Materiales/M_Agua"));
        if (WaterMat) WaterActor->GetStaticMeshComponent()->SetMaterial(0, WaterMat);

#if WITH_EDITOR
        WaterActor->SetActorLabel(*FString::Printf(TEXT("Arakil_Agua_%d"), Placed));
#endif
        Placed++;
    }

    UE_LOG(LogTemp, Log, TEXT("ArakilWater: %d tramos de agua generados"), Placed);
    return Placed;
}
