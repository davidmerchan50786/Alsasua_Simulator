#include "World/AlsasuaAwningShutterSystem.h"
#include "World/AlsasuaMallaFab.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"
#include "Components/SceneComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Math/RandomStream.h"
#include "AlturasLidarComun.h"

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
    int32 Descartadas = 0;

    // --- Dos capas instanciadas, no un actor por pieza --------------------
    // Son 17537 persianas y del orden de 700 toldos: con un AStaticMeshActor
    // cada uno eran más de dieciocho mil actores, la regla 0 por goleada.
    UStaticMesh* MallaToldo = AlsasuaMallaFab::Resolver(TEXT("toldo"),
        TEXT("/Engine/BasicShapes/Plane.Plane"));
    // "persiana", no "ventana": la clave de ventana devuelve el hueco del kit
    // modular, y lo que se coloca aquí es la persiana de delante. Meshy generó
    // Persiana_Española para este pueblo y ninguna clave la alcanzaba.
    UStaticMesh* MallaPersiana = AlsasuaMallaFab::Resolver(TEXT("persiana"),
        TEXT("/Engine/BasicShapes/Plane.Plane"));
    if (!MallaToldo || !MallaPersiana) return 0;

    if (Host) Host->Destroy();
    Host = World->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
    if (!Host) return 0;
    Host->SetRootComponent(NewObject<USceneComponent>(Host, TEXT("Raiz")));
    Host->GetRootComponent()->RegisterComponent();
#if WITH_EDITOR
    Host->SetActorLabel(TEXT("ToldosYPersianas"));
#endif

    auto CrearCapa = [&](const TCHAR* Nombre, UStaticMesh* M)
    {
        UHierarchicalInstancedStaticMeshComponent* C =
            NewObject<UHierarchicalInstancedStaticMeshComponent>(Host, Nombre);
        C->SetStaticMesh(M);
        C->SetupAttachment(Host->GetRootComponent());
        C->SetMobility(EComponentMobility::Static);
        // Chapa pegada a la fachada: ni colisión (ya la tiene el muro) ni
        // sombra dinámica, que a este número es lo que se come el frame.
        C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        C->SetCastShadow(false);
        C->RegisterComponent();
        return C;
    };
    UHierarchicalInstancedStaticMeshComponent* CapaToldos    = CrearCapa(TEXT("ISM_Toldos"), MallaToldo);
    UHierarchicalInstancedStaticMeshComponent* CapaPersianas = CrearCapa(TEXT("ISM_Persianas"), MallaPersiana);

    for (const auto& BldVal : BuildingsArr)
    {
        const TSharedPtr<FJsonObject>& Bld = BldVal->AsObject();
        if (!Bld) continue;

        const int32 Id = Bld->GetIntegerField(TEXT("id"));
        float Height = Bld->HasField(TEXT("height")) ? Bld->GetNumberField(TEXT("height")) : 10.0f;

        const TArray<TSharedPtr<FJsonValue>>* VertsArr;
        if (!Bld->TryGetArrayField(TEXT("vertices"), VertsArr) || !VertsArr || VertsArr->Num() < 3) continue;

        float CX = 0, CZ = 0;
        // El perímetro hace falta entero: las persianas van en la fachada, no en
        // el centro del edificio (ver más abajo).
        TArray<FVector2D> Contorno;
        for (const auto& V : *VertsArr)
        {
            const TSharedPtr<FJsonObject>& Vert = V->AsObject();
            if (!Vert) continue;
            const double VX = Vert->GetNumberField(TEXT("x"));
            const double VZ = Vert->GetNumberField(TEXT("z"));
            CX += VX; CZ += VZ;
            const FVector M = UAlsasuaGeoData::RelLocalToUE5(FVector(VX, 0.0, VZ));
            Contorno.Add(FVector2D(M.X, M.Y));
        }
        CX /= VertsArr->Num();
        CZ /= VertsArr->Num();
        if (Contorno.Num() < 3) continue;
        // La cota sale del terreno: antes era 0 y los toldos y persianas de los
        // 1030 edificios quedaban medio kilómetro bajo el pueblo (está a 531 m).
        FVector Center = UAlsasuaGeoData::RelLocalASueloUE5(GetWorld(), FVector(CX, 0.0f, CZ));

        // Misma altura medida que el resto: las persianas se reparten por planta
        // (Height/3), así que con la altura de OSM faltaban las de arriba.
        {
            float AltLidar = 0.f;
            int32 PlantasLidar = 0;
            if (AlturasLidar::Buscar(FVector2D(Center.X, Center.Y), AltLidar, PlantasLidar))
                Height = AltLidar;
        }

        bool bHasAwning = false;
        if (bool* Found = TieneToldo.Find(Id)) bHasAwning = *Found;

        // Sorteo por id, no FRand: si no, el toldo aparece y desaparece entre
        // ejecuciones y el pueblo no es el mismo dos veces.
        FRandomStream Sorteo(Id * 2654435761u + 17);

        if (bHasAwning && Sorteo.GetFraction() < 0.7f)
        {
            // El toldo va sobre un tramo de fachada, mirando afuera, a la altura
            // del dintel de planta baja. Antes iba en el centroide del edificio
            // con rotación al azar: o sea dentro del edificio y torcido.
            const int32 Lado = Sorteo.RandHelper(Contorno.Num());
            const FVector2D A = Contorno[Lado];
            const FVector2D B = Contorno[(Lado + 1) % Contorno.Num()];
            const FVector2D Medio = (A + B) * 0.5f;
            const FVector2D Dir = (B - A).GetSafeNormal();
            const FVector2D Fuera(Dir.Y, -Dir.X);   // normal saliente del tramo

            float Rot = FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X));
            FVector ToldoPos(Medio.X + Fuera.X * 60.f, Medio.Y + Fuera.Y * 60.f, 0.f);
            ToldoPos.Z = UAlsasuaGeoData::AlturaSueloUE5(World, ToldoPos.X, ToldoPos.Y) + 300.0f;

            FAwningEntry Toldo;
            Toldo.BuildingId = Id;
            Toldo.Posicion = ToldoPos;
            Toldo.Rotacion = Rot;
            Toldo.Ancho = Sorteo.FRandRange(150.0f, 400.0f);
            Toldo.Profundidad = Sorteo.FRandRange(80.0f, 150.0f);
            Toldo.ColorToldo = TEXT("crema");
            Toldo.Barrio = TEXT("");
            Toldo.bPlegado = (Sorteo.GetFraction() < 0.3f);

            CapaToldos->AddInstance(FTransform(FRotator(0.f, Rot, 0.f), ToldoPos,
                FVector(Toldo.Ancho / 100.0f,
                        Toldo.bPlegado ? 0.1f : Toldo.Profundidad / 100.0f,
                        0.05f)), /*bWorldSpace=*/true);

            Toldos.Add(Toldo);
            Placed++;
        }

        int32 NumPersianasReales = 0;
        if (int32* Found = VentanasConPersiana.Find(Id))
            NumPersianasReales = *Found;

        if (NumPersianasReales == 0)
            NumPersianasReales = FMath::Max(0, FMath::CeilToInt(Height / 3.0f) - 1);

        // ── Reparto de persianas por la fachada ───────────────────────────
        //
        // Antes se apilaban en el centroide del edificio, una cada 3 m de altura
        // y sin tope: el edificio 297389260 mide 7,7 m y tiene 132 ventanas con
        // persiana, así que le salía una columna de 398 m de alto atravesando el
        // tejado y subiendo medio pueblo. Y las que no sobresalían quedaban
        // dentro del edificio, invisibles y pagándose igual.
        //
        // Ahora van donde van: repartidas por el perímetro y por planta, a ras de
        // fachada y mirando afuera. Lo que no cabe en el edificio no se coloca.
        const int32 Plantas = FMath::Max(1, FMath::FloorToInt(Height / 3.0f));

        // Longitud del contorno, para saber cuántas caben por planta.
        float Perimetro = 0.f;
        for (int32 v = 0; v < Contorno.Num(); ++v)
            Perimetro += FVector2D::Distance(Contorno[v], Contorno[(v + 1) % Contorno.Num()]);

        // Una ventana cada 250 cm de fachada, que es el paso habitual del casco.
        const int32 PorPlanta = FMath::Max(1, FMath::FloorToInt(Perimetro / 250.f));
        const int32 Caben = Plantas * PorPlanta;
        const int32 AColocar = FMath::Min(NumPersianasReales, Caben);

        for (int32 p = 0; p < AColocar; ++p)
        {
            const int32 Planta = p / PorPlanta;
            const int32 Hueco  = p % PorPlanta;

            // Punto del perímetro a esa distancia recorrida.
            const float S = Perimetro * ((float)Hueco + 0.5f) / (float)PorPlanta;
            float Acum = 0.f;
            FVector2D Pos = Contorno[0], Dir(1.f, 0.f);
            for (int32 v = 0; v < Contorno.Num(); ++v)
            {
                const FVector2D A = Contorno[v];
                const FVector2D B = Contorno[(v + 1) % Contorno.Num()];
                const float L = FVector2D::Distance(A, B);
                if (Acum + L >= S && L > 1.f)
                {
                    Dir = (B - A).GetSafeNormal();
                    Pos = A + Dir * (S - Acum);
                    break;
                }
                Acum += L;
            }
            const FVector2D Fuera(Dir.Y, -Dir.X);

            FShutterEntry Persiana;
            Persiana.BuildingId = Id;
            // 1,2 m sobre el suelo de su planta: alféizar de ventana.
            Persiana.Posicion = FVector(Pos.X + Fuera.X * 15.f, Pos.Y + Fuera.Y * 15.f,
                Center.Z + Planta * 300.f + 120.f);
            Persiana.Rotacion = FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X));
            Persiana.Color = TEXT("blanco");
            Persiana.bAbierto = (Sorteo.GetFraction() < 0.5f);

            CapaPersianas->AddInstance(FTransform(
                FRotator(0.f, Persiana.Rotacion, 0.f), Persiana.Posicion,
                FVector(1.0f, 0.05f, Persiana.bAbierto ? 0.05f : 1.2f)), /*bWorldSpace=*/true);

            Persianas.Add(Persiana);
            Placed++;
        }

        Descartadas += NumPersianasReales - AColocar;
    }

    UE_LOG(LogTemp, Log,
        TEXT("Awnings: %d toldos + %d persianas en 2 capas instanciadas (%d persianas no caben en su fachada)."),
        Toldos.Num(), Persianas.Num(), Descartadas);
    return Placed;
}
