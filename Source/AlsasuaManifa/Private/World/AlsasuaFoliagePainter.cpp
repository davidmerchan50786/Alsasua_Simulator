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
    const FString Base = TEXT("/Game/AssetsImportados/Naturaleza");

    // El pack Naturaleza trae cinco rocas, cinco setos y cinco matas de maleza, y
    // aquí se usaban dos, dos y una. El resto llevaba descargado sin que nadie lo
    // pidiera, y la zona verde salía con la misma piedra repetida por todas
    // partes. Cada tipo es una capa de HISM, o sea un draw call: pasar de ocho a
    // diecisiete cuesta nueve, y el número de instancias no sube porque el tope
    // (MaxInstancias) es global y se reparte entre las capas que haya.
    //
    // La densidad es relativa entre tipos, no absoluta: manda el reparto, así que
    // al añadir variantes de un mismo tipo se baja la de cada una para que la
    // familia siga pesando lo mismo en el conjunto.
    struct FReceta { const TCHAR* Nombre; const TCHAR* Ruta; float Min; float Max; float Densidad; const TCHAR* Tipo; };
    static const FReceta Recetas[] = {
        // Hierba: la capa base, la más densa.
        // No hay grass_02: "diffus" era la textura, no la malla.
        { TEXT("Hierba_Larga"),  TEXT("/multi_stylized_grass/01_d"), 0.5f, 1.5f, 3.0f, TEXT("hierba") },
        { TEXT("Hierba_Media"),  TEXT("/multi_stylized_grass/02_d"), 0.5f, 1.3f, 2.0f, TEXT("hierba") },
        // La malla repite el nombre de la carpeta.
        { TEXT("Hierba_Corta"),  TEXT("/grass_07/grass_07"),         0.3f, 1.0f, 4.0f, TEXT("hierba") },

        // Setos: los cinco del pack. Los Round rematan esquinas y dan variedad de
        // silueta en el mismo seto largo.
        { TEXT("Seto_Largo"),        TEXT("/Hedges/Long/HedgeLong"),           0.8f, 1.2f, 0.25f, TEXT("seto") },
        { TEXT("Seto_Largo_Round"),  TEXT("/Hedges/LongRound/HedgeLongRound"), 0.8f, 1.2f, 0.25f, TEXT("seto") },
        { TEXT("Seto_Pequeno"),      TEXT("/Hedges/Small/HedgeSmall"),         0.6f, 1.0f, 0.35f, TEXT("seto") },
        { TEXT("Seto_Peq_Round"),    TEXT("/Hedges/SmallRound/HedgeSmallRound"), 0.6f, 1.0f, 0.35f, TEXT("seto") },

        // Rocas: las cinco. rock_05 viene con el nombre de fichero con dos puntos
        // (rock_05..fbx) y el .obj sin ellos, así que se apunta al nombre limpio,
        // que es lo que deja el importador.
        { TEXT("Roca_01"), TEXT("/rocks/01/rock_01"), 0.5f, 2.0f, 0.06f, TEXT("roca") },
        { TEXT("Roca_02"), TEXT("/rocks/02/rock_02"), 0.5f, 1.8f, 0.06f, TEXT("roca") },
        { TEXT("Roca_03"), TEXT("/rocks/03/rock_03"), 0.3f, 1.5f, 0.06f, TEXT("roca") },
        { TEXT("Roca_04"), TEXT("/rocks/04/rock_04"), 0.4f, 1.6f, 0.06f, TEXT("roca") },
        { TEXT("Roca_05"), TEXT("/rocks/05/rock_05"), 0.4f, 1.7f, 0.06f, TEXT("roca") },

        // Maleza: las cinco de tiny_weeds_2.
        { TEXT("Maleza_01"), TEXT("/tiny_weeds_2/01"), 0.3f, 0.8f, 0.4f, TEXT("maleza") },
        { TEXT("Maleza_02"), TEXT("/tiny_weeds_2/02"), 0.3f, 0.8f, 0.4f, TEXT("maleza") },
        { TEXT("Maleza_03"), TEXT("/tiny_weeds_2/03"), 0.3f, 0.8f, 0.4f, TEXT("maleza") },
        { TEXT("Maleza_04"), TEXT("/tiny_weeds_2/04"), 0.3f, 0.8f, 0.4f, TEXT("maleza") },
        { TEXT("Maleza_05"), TEXT("/tiny_weeds_2/05"), 0.3f, 0.8f, 0.4f, TEXT("maleza") },

        { TEXT("Hiedra"),    TEXT("/tiny_weeds_3/ivy/ivy_default"), 0.5f, 1.0f, 1.0f, TEXT("hiedra") },
    };

    Tipos.Empty(UE_ARRAY_COUNT(Recetas));
    for (const FReceta& R : Recetas)
    {
        FFoliageTypeData D;
        D.Nombre = R.Nombre;
        D.AssetPath = Base + R.Ruta;
        D.EscalaMin = R.Min;
        D.EscalaMax = R.Max;
        D.Densidad = R.Densidad;
        D.Tipo = R.Tipo;
        Tipos.Add(D);
    }

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
