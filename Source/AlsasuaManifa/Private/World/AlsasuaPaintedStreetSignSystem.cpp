#include "World/AlsasuaPaintedStreetSignSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"

void UAlsasuaPaintedStreetSignSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

int32 UAlsasuaPaintedStreetSignSystem::ColocarRotulosPintados()
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

    Rotulos.Empty();
    int32 Placed = 0;

    const TArray<FString> ColoresSign = {
        TEXT("azul"), TEXT("blanco"), TEXT("verde"), TEXT("marron")
    };

    for (const auto& RoadVal : *RoadsArr)
    {
        if (Placed >= MaxRotulos) break;

        const TSharedPtr<FJsonObject>& Road = RoadVal->AsObject();
        if (!Road) continue;

        const FString NombreES = Road->HasField(TEXT("name")) ? Road->GetStringField(TEXT("name")) : TEXT("");
        const FString NombreEU = Road->HasField(TEXT("name_eu")) ? Road->GetStringField(TEXT("name_eu")) : TEXT("");
        const FString Barrio = Road->HasField(TEXT("barrio")) ? Road->GetStringField(TEXT("barrio")) : TEXT("");

        if (NombreES.IsEmpty()) continue;

        const TArray<TSharedPtr<FJsonValue>>* PointsArr;
        if (!Road->TryGetArrayField(TEXT("points"), PointsArr) || !PointsArr || PointsArr->Num() < 2) continue;

        const TSharedPtr<FJsonObject>& P0 = (*PointsArr)[0]->AsObject();
        if (!P0) continue;

        FVector Loc = UAlsasuaGeoData::RelLocalASueloUE5(GetWorld(), FVector(
            P0->GetNumberField(TEXT("x")), 0.0f, P0->GetNumberField(TEXT("z"))), 250.0f);

        float Rot = FMath::RandRange(0.0f, 360.0f);

        FPaintedSignEntry Rotulo;
        Rotulo.NombreES = NombreES;
        Rotulo.NombreEU = NombreEU;
        Rotulo.Posicion = Loc;
        Rotulo.Rotacion = Rot;
        Rotulo.Ancho = FMath::Max(NombreES.Len(), NombreEU.Len()) * 25.0f + 50.0f;
        Rotulo.Altura = 60.0f;
        Rotulo.Barrio = Barrio;
        Rotulo.Color = ColoresSign[FMath::RandRange(0, ColoresSign.Num() - 1)];

        AStaticMeshActor* RotuloActor = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), Loc, FRotator(0, Rot, 0));
        if (RotuloActor)
        {
            RotuloActor->SetMobility(EComponentMobility::Static);
            float SX = Rotulo.Ancho / 100.0f;
            float SZ = Rotulo.Altura / 100.0f;
            RotuloActor->SetActorScale3D(FVector(SX, 0.03f, SZ));

            UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr,
                TEXT("/Engine/BasicShapes/Plane.Plane"));
            if (PlaneMesh)
                RotuloActor->GetStaticMeshComponent()->SetStaticMesh(PlaneMesh);

            UMaterialInterface* SignMat = LoadObject<UMaterialInterface>(nullptr,
                TEXT("/Game/Materiales/M_Rotulo_Pared"));
            if (SignMat)
                RotuloActor->GetStaticMeshComponent()->SetMaterial(0, SignMat);

#if WITH_EDITOR
            RotuloActor->SetActorLabel(*FString::Printf(TEXT("RotuloPintado_%s_%s"),
                *NombreES.Left(15), *Barrio.Left(6)));
#endif
        }

        Rotulos.Add(Rotulo);
        Placed++;
    }

    UE_LOG(LogTemp, Log, TEXT("PaintedSigns: %d rótulos de calle pintados en muros (bilingües eu/es)"), Placed);
    return Placed;
}
