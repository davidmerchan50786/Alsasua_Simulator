#include "World/AlsasuaStreetArtSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Components/SceneComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Math/RandomStream.h"
#include "GeoDataAlsasua.h"
#include "CargarJsonComun.h"

namespace
{
    /** Un muro del footprint, ya en coordenadas de mundo y con su normal. */
    struct FLienzo
    {
        int32 EdificioId = -1;
        FString Barrio;
        FVector2D A;        // extremo del tramo, local relativo (X = x, Y = z)
        FVector2D B;
        float LargoM = 0.0f;
        FVector2D Fuera;    // normal saliente, en local relativo
        float Yaw = 0.0f;   // yaw de UE5 que mira hacia afuera
    };

    /**
     * Muros candidatos: tramos del perímetro de cada edificio, con su normal
     * saliente y su largo.
     *
     * El sentido de la normal se decide contra el centroide del footprint, que
     * es lo único que distingue "afuera" de "adentro" sin saber el orden de giro
     * del polígono, y buildings_final.json no lo garantiza.
     */
    void RecogerLienzos(float LargoMinimoM, TArray<FLienzo>& Out)
    {
        Out.Reset();

        TArray<TSharedPtr<FJsonValue>> Edificios;
        if (!JsonDatos::CargarArray(TEXT("Datos/buildings_final.json"), Edificios, { TEXT("buildings") }))
            return;

        for (const TSharedPtr<FJsonValue>& EV : Edificios)
        {
            const TSharedPtr<FJsonObject> Edif = EV->AsObject();
            if (!Edif.IsValid()) continue;

            const TArray<TSharedPtr<FJsonValue>>* Verts = nullptr;
            if (!Edif->TryGetArrayField(TEXT("vertices"), Verts) || !Verts || Verts->Num() < 3) continue;

            const int32 Id = Edif->HasField(TEXT("id")) ? Edif->GetIntegerField(TEXT("id")) : -1;
            FString Barrio;
            Edif->TryGetStringField(TEXT("barrio"), Barrio);

            TArray<FVector2D> Contorno;
            Contorno.Reserve(Verts->Num());
            FVector2D Centro(0.0f, 0.0f);
            for (const TSharedPtr<FJsonValue>& V : *Verts)
            {
                const TSharedPtr<FJsonObject> Vert = V->AsObject();
                if (!Vert.IsValid()) continue;
                const FVector2D P(Vert->GetNumberField(TEXT("x")), Vert->GetNumberField(TEXT("z")));
                Contorno.Add(P);
                Centro += P;
            }
            if (Contorno.Num() < 3) continue;
            Centro /= Contorno.Num();

            for (int32 v = 0; v < Contorno.Num(); ++v)
            {
                const FVector2D A = Contorno[v];
                const FVector2D B = Contorno[(v + 1) % Contorno.Num()];
                const float L = FVector2D::Distance(A, B);
                if (L < LargoMinimoM) continue;

                const FVector2D Dir = (B - A) / L;
                FVector2D N(Dir.Y, -Dir.X);
                // Si la normal apunta al centroide, es la de dentro: se le da la
                // vuelta. Sin esto, la mitad de las pintadas quedan pintadas por
                // el lado de dentro del muro.
                const FVector2D Medio = (A + B) * 0.5f;
                if (FVector2D::DotProduct(N, Medio - Centro) < 0.0f) N = -N;

                FLienzo Li;
                Li.EdificioId = Id;
                Li.Barrio = Barrio;
                Li.A = A;
                Li.B = B;
                Li.LargoM = L;
                Li.Fuera = N;
                Li.Yaw = FMath::RadiansToDegrees(FMath::Atan2(N.Y, N.X));
                Out.Add(MoveTemp(Li));
            }
        }
    }
}

void UAlsasuaStreetArtSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

int32 UAlsasuaStreetArtSystem::ColocarArteCallejero()
{
    UWorld* World = GetWorld();
    if (!World) return 0;

    Arte.Empty();

    const TArray<TPair<FString, FString>> MensajesMurales = {
        {TEXT("Gora Euskal Herria"), TEXT("rojo")},
        {TEXT("Askatasuna"), TEXT("amarillo")},
        {TEXT("Eskubide Sozialak"), TEXT("azul")},
        {TEXT("Altsasu Bizirik"), TEXT("verde")},
        {TEXT("Herri Antzinakoa"), TEXT("naranja")},
        {TEXT("Navarra Aske"), TEXT("rojo")},
        {TEXT("Sakana Gure Ametxea"), TEXT("amarillo")},
        {TEXT("Euskara Mundura"), TEXT("azul")},
    };

    const TArray<TPair<FString, FString>> Grafitis = {
        {TEXT("KA"), TEXT("blanco")},
        {TEXT("ALTSASU"), TEXT("rojo")},
        {TEXT("NAZK"), TEXT("amarillo")},
        {TEXT("ETA"), TEXT("negro")},
        {TEXT("BARRIKA"), TEXT("azul")},
        {TEXT("SARE"), TEXT("verde")},
        {TEXT("HERRIAK"), TEXT("naranja")},
        {TEXT("ASKI"), TEXT("blanco")},
        {TEXT("ERNAI"), TEXT("rojo")},
        {TEXT("GURE"), TEXT("amarillo")},
        {TEXT("HERRIA"), TEXT("azul")},
        {TEXT("EUSKAL"), TEXT("verde")},
        {TEXT("NAFARROA"), TEXT("naranja")},
        {TEXT("SAKANA"), TEXT("blanco")},
        {TEXT("MUNDUA"), TEXT("rojo")},
    };

    // Un mural necesita paño; una pintada cabe en cualquier muro. Se piden por
    // separado para no meter un mural de 5 m en una medianera de 3.
    TArray<FLienzo> MurosLargos, MurosCualquiera;
    RecogerLienzos(6.0f, MurosLargos);
    RecogerLienzos(2.5f, MurosCualquiera);

    if (MurosCualquiera.Num() == 0)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("StreetArt: sin footprints en buildings_final.json, no hay muro donde pintar"));
        return 0;
    }

    if (Host) Host->Destroy();
    Host = World->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
    if (!Host) return 0;
    Host->SetRootComponent(NewObject<USceneComponent>(Host, TEXT("Raiz")));
    Host->GetRootComponent()->RegisterComponent();
#if WITH_EDITOR
    Host->SetActorLabel(TEXT("ArteCallejero"));
#endif

    // El cubo, no el plano: /Engine/BasicShapes/Plane es un plano en XY mirando
    // hacia arriba, y escalarle la Z no lo pone de pie. La pintada es una capa
    // fina sobre el muro, así que es un cubo con 3 cm de grueso.
    UStaticMesh* Cubo = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!Cubo) return 0;

    auto CrearCapa = [&](const TCHAR* Nombre, const TCHAR* RutaMat)
        -> UHierarchicalInstancedStaticMeshComponent*
    {
        UHierarchicalInstancedStaticMeshComponent* C =
            NewObject<UHierarchicalInstancedStaticMeshComponent>(Host, Nombre);
        C->SetStaticMesh(Cubo);
        // Fuera del bucle: antes se resolvía una vez por pieza.
        if (UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, RutaMat))
            C->SetMaterial(0, M);
        C->SetupAttachment(Host->GetRootComponent());
        C->SetMobility(EComponentMobility::Static);
        // Pintura sobre un muro: sin colisión propia, o los trazos de altura de
        // otros sistemas se subirían encima.
        C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        C->SetCastShadow(false);
        C->RegisterComponent();
        return C;
    };

    UHierarchicalInstancedStaticMeshComponent* CapaMurales =
        CrearCapa(TEXT("ISM_Murales"), TEXT("/Game/Materiales/M_Mural_Pared"));
    UHierarchicalInstancedStaticMeshComponent* CapaGrafitis =
        CrearCapa(TEXT("ISM_Grafitis"), TEXT("/Game/Materiales/M_Grafiti"));

    int32 Placed = 0;

    // Pinta una pieza sobre un muro. Devuelve false si no cupo.
    auto Pintar = [&](const TArray<FLienzo>& Muros, UHierarchicalInstancedStaticMeshComponent* Capa,
                      FRandomStream& Sorteo, const FString& Tipo, const TPair<FString, FString>& Texto,
                      float AnchoMin, float AnchoMax, float AltoMin, float AltoMax,
                      float BaseCm, float GruesoCm) -> bool
    {
        if (!Capa || Muros.Num() == 0) return false;

        const FLienzo& L = Muros[Sorteo.RandHelper(Muros.Num())];

        float AnchoCm = Sorteo.FRandRange(AnchoMin, AnchoMax);
        // No pintar más ancho que el muro: medio metro de margen a cada lado.
        AnchoCm = FMath::Min(AnchoCm, FMath::Max(50.0f, L.LargoM * 100.0f - 100.0f));
        const float AltoCm = Sorteo.FRandRange(AltoMin, AltoMax);

        // Punto sobre el tramo, dejando el ancho de la pieza dentro del muro.
        const float MargenM = AnchoCm * 0.5f * 0.01f;
        const float Recorrido = FMath::Max(0.0f, L.LargoM - 2.0f * MargenM);
        const FVector2D Dir = (L.B - L.A).GetSafeNormal();
        const FVector2D XZ = L.A + Dir * (MargenM + Sorteo.FRandRange(0.0f, Recorrido))
                           + L.Fuera * (GruesoCm * 0.5f * 0.01f);

        FVector Pos = UAlsasuaGeoData::RelLocalToUE5(FVector(XZ.X, 0.0f, XZ.Y));
        Pos.Z = UAlsasuaGeoData::AlturaSueloUE5(World, Pos.X, Pos.Y) + BaseCm + AltoCm * 0.5f;

        // El eje local X del cubo mira hacia afuera del muro: grueso en X, ancho
        // en Y, alto en Z.
        Capa->AddInstance(FTransform(
            FRotator(0.0f, L.Yaw, 0.0f), Pos,
            FVector(GruesoCm / 100.0f, AnchoCm / 100.0f, AltoCm / 100.0f)), /*bWorldSpace=*/true);

        FStreetArt Art;
        Art.Tipo = Tipo;
        Art.Mensaje = Texto.Key;
        Art.Color = Texto.Value;
        Art.Posicion = Pos;
        Art.Rotacion = L.Yaw;
        Art.Ancho = AnchoCm;
        Art.Altura = AltoCm;
        Art.Barrio = L.Barrio;
        Art.EdificioId = L.EdificioId;
        Arte.Add(MoveTemp(Art));
        ++Placed;
        return true;
    };

    for (int32 i = 0; i < MaxMurales; ++i)
    {
        // Semilla por índice: el mismo mural cae en el mismo muro en cada
        // arranque. Antes era FRand y el pueblo no era el mismo dos veces.
        FRandomStream Sorteo(9001 + i * 7919);
        Pintar(MurosLargos.Num() ? MurosLargos : MurosCualquiera, CapaMurales, Sorteo,
               TEXT("mural"), MensajesMurales[i % MensajesMurales.Num()],
               /*AnchoMin=*/250.0f, /*AnchoMax=*/500.0f,
               /*AltoMin=*/150.0f, /*AltoMax=*/300.0f,
               /*BaseCm=*/60.0f, /*GruesoCm=*/5.0f);
    }

    for (int32 i = 0; i < MaxGrafitis; ++i)
    {
        FRandomStream Sorteo(4201 + i * 6151);
        Pintar(MurosCualquiera, CapaGrafitis, Sorteo,
               TEXT("grafiti"), Grafitis[i % Grafitis.Num()],
               /*AnchoMin=*/60.0f, /*AnchoMax=*/180.0f,
               /*AltoMin=*/40.0f, /*AltoMax=*/120.0f,
               /*BaseCm=*/40.0f, /*GruesoCm=*/3.0f);
    }

    UE_LOG(LogTemp, Log,
        TEXT("StreetArt: %d piezas sobre muro real (%d paños de 6 m o más, %d de 2,5 m o más)"),
        Placed, MurosLargos.Num(), MurosCualquiera.Num());
    return Placed;
}
