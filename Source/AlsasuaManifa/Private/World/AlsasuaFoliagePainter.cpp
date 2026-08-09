#include "World/AlsasuaFoliagePainter.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"

void UAlsasuaFoliagePainter::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    InicializarTipos();
}

void UAlsasuaFoliagePainter::Deinitialize()
{
    Tipos.Empty();
    bCargado = false;
    Super::Deinitialize();
}

void UAlsasuaFoliagePainter::InicializarTipos()
{
    Tipos.Empty(20);

    const FString Base = TEXT("/Game/AssetsImportados/Naturaleza");

    FFoliageTypeData Grass02;
    Grass02.Nombre = TEXT("Hierba_Larga");
    Grass02.AssetPath = Base + TEXT("/multi_stylized_grass/01_d")   // no hay grass_02; "diffus" era la textura, no la malla;
    Grass02.EscalaMin = 0.5f;
    Grass02.EscalaMax = 1.5f;
    Grass02.Densidad = 3.0f;
    Grass02.Tipo = TEXT("hierba");
    Tipos.Add(Grass02);

    FFoliageTypeData Grass07;
    Grass07.Nombre = TEXT("Hierba_Corta");
    Grass07.AssetPath = Base + TEXT("/grass_07/grass_07")           // la malla repite el nombre de carpeta;
    Grass07.EscalaMin = 0.3f;
    Grass07.EscalaMax = 1.0f;
    Grass07.Densidad = 4.0f;
    Grass07.Tipo = TEXT("hierba");
    Tipos.Add(Grass07);

    FFoliageTypeData HedgeLong;
    HedgeLong.Nombre = TEXT("Seto_Largo");
    HedgeLong.AssetPath = Base + TEXT("/Hedges/Long/HedgeLong");
    HedgeLong.EscalaMin = 0.8f;
    HedgeLong.EscalaMax = 1.2f;
    HedgeLong.Densidad = 0.5f;
    HedgeLong.Tipo = TEXT("seto");
    Tipos.Add(HedgeLong);

    FFoliageTypeData HedgeSmall;
    HedgeSmall.Nombre = TEXT("Seto_Pequeno");
    HedgeSmall.AssetPath = Base + TEXT("/Hedges/Small/HedgeSmall");
    HedgeSmall.EscalaMin = 0.6f;
    HedgeSmall.EscalaMax = 1.0f;
    HedgeSmall.Densidad = 0.7f;
    HedgeSmall.Tipo = TEXT("seto");
    Tipos.Add(HedgeSmall);

    FFoliageTypeData Rock01;
    Rock01.Nombre = TEXT("Roca_01");
    Rock01.AssetPath = Base + TEXT("/rocks/01/rock_01");
    Rock01.EscalaMin = 0.5f;
    Rock01.EscalaMax = 2.0f;
    Rock01.Densidad = 0.2f;
    Rock01.Tipo = TEXT("roca");
    Tipos.Add(Rock01);

    FFoliageTypeData Rock03;
    Rock03.Nombre = TEXT("Roca_03");
    Rock03.AssetPath = Base + TEXT("/rocks/03/rock_03");
    Rock03.EscalaMin = 0.3f;
    Rock03.EscalaMax = 1.5f;
    Rock03.Densidad = 0.15f;
    Rock03.Tipo = TEXT("roca");
    Tipos.Add(Rock03);

    FFoliageTypeData Weed01;
    Weed01.Nombre = TEXT("Maleza_01");
    Weed01.AssetPath = Base + TEXT("/tiny_weeds_2/01");
    Weed01.EscalaMin = 0.3f;
    Weed01.EscalaMax = 0.8f;
    Weed01.Densidad = 2.0f;
    Weed01.Tipo = TEXT("maleza");
    Tipos.Add(Weed01);

    FFoliageTypeData Ivy;
    Ivy.Nombre = TEXT("Hiedra");
    Ivy.AssetPath = Base + TEXT("/tiny_weeds_3/ivy/ivy_default");
    Ivy.EscalaMin = 0.5f;
    Ivy.EscalaMax = 1.0f;
    Ivy.Densidad = 1.0f;
    Ivy.Tipo = TEXT("hiedra");
    Tipos.Add(Ivy);

    bCargado = true;
}

bool UAlsasuaFoliagePainter::CargarTipos()
{
    if (bCargado) return true;
    InicializarTipos();
    return Tipos.Num() > 0;
}

int32 UAlsasuaFoliagePainter::PintarFoliageEnZonasVerdes()
{
    if (!bCargado && !CargarTipos()) return 0;

    UWorld* World = GetWorld();
    if (!World) return 0;

    const FString GreensPath = FPaths::ProjectContentDir() + TEXT("Datos/greenspaces_unity.json");
    TArray<FString> Lineas;
    if (!FFileHelper::LoadFileToStringArray(Lineas, *GreensPath))
    {
        UE_LOG(LogTemp, Error, TEXT("FoliagePainter: No se pudo cargar greenspaces_unity.json"));
        return 0;
    }

    FString JsonStr;
    for (const FString& L : Lineas) JsonStr += L;

    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) return 0;

    const TArray<TSharedPtr<FJsonValue>>* Arr;
    if (!Root->TryGetArrayField(TEXT("spaces"), Arr) && !Root->TryGetArrayField(TEXT(""), Arr)) return 0;

    int32 TotalPlaced = 0;
    for (const auto& Val : *Arr)
    {
        const TSharedPtr<FJsonObject>& Obj = Val->AsObject();
        if (!Obj) continue;

        const float GX = Obj->GetNumberField(TEXT("x"));
        const float GZ = Obj->GetNumberField(TEXT("z"));
        const float Radius = Obj->HasField(TEXT("radius")) ? Obj->GetNumberField(TEXT("radius")) : 50.0f;

        const FVector Center = UAlsasuaGeoData::UnityaUnreal(FVector(GX, GZ, 0));
        const float RadiusCm = Radius * 100.0f;

        for (const FFoliageTypeData& Tipo : Tipos)
        {
            const int32 Count = FMath::Max(1, (int32)(RadiusCm * Tipo.Densidad / 500.0f));
            for (int32 i = 0; i < Count; i++)
            {
                const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
                const float Dist = FMath::FRandRange(0.0f, RadiusCm);
                FVector Offset(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 0);
                FVector SpawnLoc = Center + Offset;

                float Scale = FMath::FRandRange(Tipo.EscalaMin, Tipo.EscalaMax);

                AStaticMeshActor* FoliageActor = World->SpawnActor<AStaticMeshActor>(
                    AStaticMeshActor::StaticClass(), SpawnLoc,
                    FRotator(0, FMath::FRandRange(0.0f, 360.0f), 0));
                if (FoliageActor)
                {
                    FoliageActor->SetMobility(EComponentMobility::Movable);
                    FoliageActor->SetActorScale3D(FVector(Scale));

                    UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Tipo.AssetPath);
                    if (Mesh) FoliageActor->GetStaticMeshComponent()->SetStaticMesh(Mesh);

#if WITH_EDITOR
                    FoliageActor->SetActorLabel(*FString::Printf(TEXT("Foliage_%s_%d"),
                        *Tipo.Nombre, TotalPlaced));
#endif
                    TotalPlaced++;
                }
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("FoliagePainter: %d piezas de foliage en zonas verdes"), TotalPlaced);
    return TotalPlaced;
}
