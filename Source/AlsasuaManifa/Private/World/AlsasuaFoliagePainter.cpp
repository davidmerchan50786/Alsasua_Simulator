#include "World/AlsasuaFoliagePainter.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Components/SceneComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"
#include "CargarJsonComun.h"

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
    // No hay grass_02: "diffus" era la textura, no la malla.
    Grass02.AssetPath = Base + TEXT("/multi_stylized_grass/01_d");
    Grass02.EscalaMin = 0.5f;
    Grass02.EscalaMax = 1.5f;
    Grass02.Densidad = 3.0f;
    Grass02.Tipo = TEXT("hierba");
    Tipos.Add(Grass02);

    FFoliageTypeData Grass07;
    Grass07.Nombre = TEXT("Hierba_Corta");
    // La malla repite el nombre de la carpeta.
    Grass07.AssetPath = Base + TEXT("/grass_07/grass_07");
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

    // greenspaces_unity.json es un array de 273 zonas verdes en la raíz, no un
    // objeto con campo "spaces". Salía por aquí sin pintar una sola mata.
    TArray<TSharedPtr<FJsonValue>> Arr;
    if (!JsonDatos::CargarArray(TEXT("Datos/greenspaces_unity.json"), Arr, { TEXT("spaces") }))
    {
        UE_LOG(LogTemp, Error, TEXT("FoliagePainter: sin zonas verdes en greenspaces_unity.json"));
        return 0;
    }

    // --- Polígonos ----------------------------------------------------------
    // Aquí se leían "x", "z" y "radius". Ninguno de los tres existe en ninguna de
    // las 273 zonas: cada una es un polígono en "poly", un array plano
    // [x,z,x,z,...] en local ABSOLUTO. Con el cargador arreglado, aquello habría
    // dado 0 en las dos coordenadas y habría amontonado todo el foliage del
    // pueblo en el origen del mundo, con un aviso de campo ausente por zona.
    TArray<TArray<FVector2D>> Poligonos;
    for (const auto& Val : Arr)
    {
        const TSharedPtr<FJsonObject>& Obj = Val->AsObject();
        if (!Obj) continue;

        const TArray<TSharedPtr<FJsonValue>>* Pts = nullptr;
        if (!Obj->TryGetArrayField(TEXT("poly"), Pts) || !Pts || Pts->Num() < 6) continue;

        TArray<FVector2D> P;
        P.Reserve(Pts->Num() / 2);
        for (int32 i = 0; i + 1 < Pts->Num(); i += 2)
        {
            const FVector M = UAlsasuaGeoData::UnityaUnreal(
                FVector((*Pts)[i]->AsNumber(), 0.0, (*Pts)[i + 1]->AsNumber()));
            P.Add(FVector2D(M.X, M.Y));
        }
        if (P.Num() >= 3) Poligonos.Add(MoveTemp(P));
    }
    if (Poligonos.Num() == 0) return 0;

    // --- Un HISM por tipo ---------------------------------------------------
    AActor* Host = World->SpawnActor<AActor>(AActor::StaticClass(),
        FVector::ZeroVector, FRotator::ZeroRotator);
    if (!Host) return 0;
    Host->SetRootComponent(NewObject<USceneComponent>(Host, TEXT("Raiz")));
    Host->GetRootComponent()->RegisterComponent();
#if WITH_EDITOR
    Host->SetActorLabel(TEXT("Foliage_ZonasVerdes"));
#endif

    struct FCapa { const FFoliageTypeData* Tipo; UHierarchicalInstancedStaticMeshComponent* ISM; };
    TArray<FCapa> Capas;
    float SumaDensidad = 0.f;
    for (const FFoliageTypeData& T : Tipos)
    {
        UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *T.AssetPath);
        if (!Mesh) continue;   // pack no importado: esa capa simplemente no sale

        UHierarchicalInstancedStaticMeshComponent* ISM =
            NewObject<UHierarchicalInstancedStaticMeshComponent>(Host, *(TEXT("ISM_") + T.Nombre));
        ISM->SetStaticMesh(Mesh);
        ISM->SetupAttachment(Host->GetRootComponent());
        ISM->SetMobility(EComponentMobility::Static);
        // Matas y piedras: ni colisión (el jugador las atraviesa) ni sombra
        // dinámica, que a este número de instancias es lo que se come el frame.
        ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        ISM->SetCastShadow(false);
        ISM->RegisterComponent();
        Capas.Add({ &T, ISM });
        SumaDensidad += T.Densidad;
    }
    if (Capas.Num() == 0 || SumaDensidad <= 0.f)
    {
        UE_LOG(LogTemp, Warning, TEXT("FoliagePainter: ninguna malla del pack Naturaleza importada."));
        Host->Destroy();
        return 0;
    }

    // --- Siembra ------------------------------------------------------------
    FRandomStream Rng(20240815);
    int32 TotalPlaced = 0;

    auto Dentro = [](const TArray<FVector2D>& P, const FVector2D& Q)
    {
        bool b = false;
        for (int32 i = 0, j = P.Num() - 1; i < P.Num(); j = i++)
        {
            if (((P[i].Y > Q.Y) != (P[j].Y > Q.Y)) &&
                (Q.X < (P[j].X - P[i].X) * (Q.Y - P[i].Y) / (P[j].Y - P[i].Y) + P[i].X))
            {
                b = !b;
            }
        }
        return b;
    };

    for (const TArray<FVector2D>& P : Poligonos)
    {
        if (TotalPlaced >= MaxInstancias) break;

        FVector2D Min(TNumericLimits<double>::Max()), Max(-TNumericLimits<double>::Max());
        double Area2 = 0.0;
        for (int32 i = 0, j = P.Num() - 1; i < P.Num(); j = i++)
        {
            Min.X = FMath::Min(Min.X, P[i].X); Min.Y = FMath::Min(Min.Y, P[i].Y);
            Max.X = FMath::Max(Max.X, P[i].X); Max.Y = FMath::Max(Max.Y, P[i].Y);
            Area2 += P[j].X * P[i].Y - P[i].X * P[j].Y;   // fórmula del cordón
        }
        const double AreaM2 = FMath::Abs(Area2) * 0.5 / (100.0 * 100.0);
        const int32 Objetivo = FMath::Min(
            (int32)(AreaM2 * DensidadPor100m2 / 100.0),
            MaxInstancias - TotalPlaced);

        // Diez intentos por instancia: en un polígono muy dentado el rechazo
        // sube, pero acotar los intentos evita quedarse dando vueltas.
        int32 Puestas = 0;
        for (int32 intento = 0; intento < Objetivo * 10 && Puestas < Objetivo; ++intento)
        {
            const FVector2D Q(Rng.FRandRange(Min.X, Max.X), Rng.FRandRange(Min.Y, Max.Y));
            if (!Dentro(P, Q)) continue;

            // Reparto por densidad relativa: la hierba sale mucho más que la roca.
            float Tirada = Rng.FRandRange(0.f, SumaDensidad);
            const FCapa* Elegida = &Capas.Last();
            for (const FCapa& C : Capas)
            {
                Tirada -= C.Tipo->Densidad;
                if (Tirada <= 0.f) { Elegida = &C; break; }
            }

            const float Z = UAlsasuaGeoData::AlturaSueloUE5(World, Q.X, Q.Y);
            FTransform T(FRotator(0.f, Rng.FRandRange(0.f, 360.f), 0.f),
                         FVector(Q.X, Q.Y, Z),
                         FVector(Rng.FRandRange(Elegida->Tipo->EscalaMin, Elegida->Tipo->EscalaMax)));
            Elegida->ISM->AddInstance(T, /*bWorldSpace=*/true);
            ++Puestas;
            ++TotalPlaced;
        }
    }

    UE_LOG(LogTemp, Log,
        TEXT("FoliagePainter: %d instancias en %d zonas verdes, %d capas (%d draw calls)."),
        TotalPlaced, Poligonos.Num(), Capas.Num(), Capas.Num());
    return TotalPlaced;
}
