#include "World/AlsasuaParkingSystem.h"
#include "World/AlsasuaDirecciones.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Components/SceneComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Math/RandomStream.h"
#include "GeoDataAlsasua.h"
#include "CargarJsonComun.h"
#include "CargarMaterialComun.h"
#include "HAL/ConsoleManager.h"

static TAutoConsoleVariable<int32> CVarSkipParkingGeneration(
    TEXT("alsasua.SkipParkingGeneration"),
    0,
    TEXT("Skips parking generation for profiling"),
    ECVF_Cheat);

namespace
{
    /**
     * Vías donde se aparca en línea: las 194 residenciales, 24,9 km.
     *
     * Antes se sorteaba entre las 489 de roads_unity.json, que incluyen la A-10
     * (39 tramos), la N-1 con sus 50 enlaces, los 56 caminos y las 31 calles
     * peatonales del casco: batería de plazas pintadas sobre la autovía.
     *
     * Los 44 tramos "service" quedan fuera aunque en OSM a veces sean pasillos
     * de aparcamiento: aquí son de 3,5 m y la mitad están en Monte, o sea pistas
     * y accesos, no calle. Y los "tertiary" son las carreteras de salida
     * (Gipuzkoa, Urdiain), donde tampoco se aparca en línea.
     */
    bool AdmiteAparcamiento(const FString& Tipo)
    {
        return Tipo == TEXT("residential");
    }
}

void UAlsasuaParkingSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    // Las calles se leen al generar las plazas, no aquí: en Initialize todavía
    // no hay terreno y los puntos se quedarían a la cota de la plaza.
}

int32 UAlsasuaParkingSystem::PintarPlazasDeCalle(UHierarchicalInstancedStaticMeshComponent* Capa)
{
    UWorld* World = GetWorld();
    if (!World || !Capa) return 0;

    TArray<TSharedPtr<FJsonValue>> Vias;
    if (!JsonDatos::CargarArray(TEXT("Datos/roads_unity.json"), Vias, { TEXT("roads") }))
        return 0;

    // Primera pasada: dónde cabría una plaza. Sólo aritmética, sin un solo
    // trazo de suelo, porque las 5950 que salen no se colocan todas.
    struct FCandidata
    {
        FVector2D XZ;      // local relativo
        float Yaw = 0.0f;
        FString Barrio;
        uint32 Semilla = 0;
        /** Tramo y lado a los que pertenece: las plazas se cogen por hileras
         *  enteras, no sueltas. Una plaza aislada cada cincuenta metros no se
         *  lee como aparcamiento, se lee como un rectángulo blanco perdido. */
        int32 Hilera = 0;
    };
    TArray<FCandidata> Candidatas;
    int32 SiguienteHilera = 0;

    for (const TSharedPtr<FJsonValue>& VV : Vias)
    {
        const TSharedPtr<FJsonObject> Via = VV->AsObject();
        if (!Via.IsValid()) continue;

        FString Tipo;
        Via->TryGetStringField(TEXT("type"), Tipo);
        if (!AdmiteAparcamiento(Tipo)) continue;

        const TArray<TSharedPtr<FJsonValue>>* Pts = nullptr;
        if (!Via->TryGetArrayField(TEXT("points"), Pts) || !Pts || Pts->Num() < 2) continue;

        const int32 IdVia = Via->HasField(TEXT("id")) ? Via->GetIntegerField(TEXT("id")) : 0;
        FString Barrio = TEXT("Herriko");
        Via->TryGetStringField(TEXT("barrio"), Barrio);

        double AnchoM = 4.5;
        Via->TryGetNumberField(TEXT("width"), AnchoM);
        const float SemiAnchoM = static_cast<float>(AnchoM) * 0.5f;

        // Una calle de menos de 4,5 m no da para aparcar a los dos lados sin
        // cerrar el paso; ahí se aparca sólo en un lado.
        const bool bDosLados = AnchoM >= 4.5;

        TArray<FVector2D> Eje;
        Eje.Reserve(Pts->Num());
        for (const TSharedPtr<FJsonValue>& PV : *Pts)
        {
            const TSharedPtr<FJsonObject> PO = PV->AsObject();
            if (!PO.IsValid()) continue;
            Eje.Add(FVector2D(PO->GetNumberField(TEXT("x")), PO->GetNumberField(TEXT("z"))));
        }
        if (Eje.Num() < 2) continue;

        for (int32 i = 0; i + 1 < Eje.Num(); ++i)
        {
            const FVector2D A = Eje[i];
            const FVector2D B = Eje[i + 1];
            const float LargoM = FVector2D::Distance(A, B);
            if (LargoM < 1.0f) continue;

            const FVector2D Dir = (B - A) / LargoM;
            const FVector2D Normal(-Dir.Y, Dir.X);

            // Plazas espaciadas a lo largo del tramo, con medio hueco de margen
            // en cada extremo para no pisar el cruce.
            const float PasoM = PasoPlazaCm * 0.01f;
            const int32 Cuantas = FMath::FloorToInt((LargoM - PasoM) / PasoM);
            if (Cuantas <= 0) continue;

            const float Sobra = LargoM - Cuantas * PasoM;
            const float YawVia = FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X));
            const float SepM = SemiAnchoM + RetranqueoCm * 0.01f;

            for (int32 s = 0; s < (bDosLados ? 2 : 1); ++s)
            {
                const int32 Hilera = SiguienteHilera++;
                for (int32 k = 0; k < Cuantas; ++k)
                {
                    const FVector2D SobreEje = A + Dir * (Sobra * 0.5f + (k + 0.5f) * PasoM);

                    FCandidata C;
                    C.XZ = SobreEje + Normal * ((s == 0) ? SepM : -SepM);
                    C.Yaw = YawVia;
                    C.Barrio = Barrio;
                    C.Semilla = static_cast<uint32>(IdVia) * 2654435761u
                              + static_cast<uint32>(i * 64 + k * 2 + s);
                    C.Hilera = Hilera;
                    Candidatas.Add(MoveTemp(C));
                }
            }
        }
    }

    if (Candidatas.Num() == 0) return 0;

    // Segunda pasada: si sobran, se descartan hileras enteras repartidas por
    // toda la lista, no plazas sueltas.
    //
    // Dos razones para no cortar por el tope con un `break`: roads_unity.json no
    // viene ordenado por nada geográfico, así que el pueblo se quedaría con las
    // primeras treinta calles llenas y el resto sin una plaza; y una plaza suelta
    // cada cincuenta metros no parece aparcamiento. Se lleva la cuenta de lo
    // colocado y se admite la siguiente hilera sólo mientras vaya por detrás de
    // su cuota, que reparte los huecos por todo el trazado.
    const int32 Total = Candidatas.Num();
    const int32 Objetivo = FMath::Min(MaxPlazasCalle, Total);

    int32 Puestas = 0;
    int32 HileraActual = -1;
    bool bHileraAdmitida = false;
    int32 Vistas = 0;

    for (int32 i = 0; i < Total; ++i)
    {
        const FCandidata& C = Candidatas[i];

        if (C.Hilera != HileraActual)
        {
            HileraActual = C.Hilera;
            // Cuántas plazas tocarían ya, si el reparto fuese perfecto.
            const int32 Cuota = static_cast<int32>(static_cast<int64>(Vistas) * Objetivo / Total);
            bHileraAdmitida = (Puestas <= Cuota);
        }
        ++Vistas;
        if (!bHileraAdmitida) continue;

        // La cota se muestrea en la propia plaza, no en el eje de la calle: el
        // bordillo de una calle en pendiente no está a la altura del centro de
        // la calzada.
        // Y de la superficie, no del terreno: la plaza se pinta sobre el firme,
        // que va 12 cm por encima del suelo (ACalleGenerada::EpsilonCm).
        const FVector Pos = UAlsasuaGeoData::RelLocalASuperficieUE5(
            World, FVector(C.XZ.X, 0.0f, C.XZ.Y));

        FRandomStream Sorteo(static_cast<int32>(C.Semilla));

        FParkingSpot Plaza;
        Plaza.Posicion = Pos;
        Plaza.Rotacion = C.Yaw;
        Plaza.Tipo = TEXT("calle");
        Plaza.bOcupado = (Sorteo.GetFraction() < 0.25f);
        Plaza.Barrio = C.Barrio;
        Plazas.Add(Plaza);

        // 2 cm sobre el firme: la plaza va pintada sobre el asfalto y a ras
        // compite en z-fighting con la calzada.
        Capa->AddInstance(FTransform(
            FRotator(0.0f, C.Yaw, 0.0f),
            Pos + FVector(0.0f, 0.0f, 2.0f),
            FVector(4.6f, 2.3f, 1.0f)), /*bWorldSpace=*/true);

        ++Puestas;
    }

    UE_LOG(LogTemp, Log, TEXT("Parking: %d plazas en línea de %d posibles, repartidas por las calles residential/service"),
        Puestas, Total);
    return Puestas;
}

int32 UAlsasuaParkingSystem::ColocarPuertasGaraje(UHierarchicalInstancedStaticMeshComponent* Capa)
{
    UWorld* World = GetWorld();
    if (!World || !Capa) return 0;

    TArray<TSharedPtr<FJsonValue>> Edificios;
    if (!JsonDatos::CargarArray(TEXT("Datos/buildings_final.json"), Edificios, { TEXT("buildings") }))
        return 0;

    int32 Puestas = 0;
    for (const TSharedPtr<FJsonValue>& EV : Edificios)
    {
        if (Puestas >= MaxGarajes) break;

        const TSharedPtr<FJsonObject> Edif = EV->AsObject();
        if (!Edif.IsValid()) continue;

        const int32 Id = Edif->HasField(TEXT("id")) ? Edif->GetIntegerField(TEXT("id")) : -1;
        if (Id < 0) continue;

        // Garaje de portal: en vivienda. Ni la iglesia ni el colegio ni la
        // estación tienen puerta de garaje, y las 58 naves industriales tienen
        // portón, que no es lo mismo ni mide lo mismo.
        FString TipoEdif;
        Edif->TryGetStringField(TEXT("type"), TipoEdif);
        const bool bVivienda = TipoEdif == TEXT("apartments") || TipoEdif == TEXT("house")
            || TipoEdif == TEXT("detached") || TipoEdif == TEXT("terrace")
            || TipoEdif == TEXT("yes");   // 414 sin clasificar en OSM, casi todo vivienda
        if (!bVivienda) continue;

        // Sorteo propio, distinto del de las puertas de entrada: si compartieran
        // semilla, el garaje caería siempre en los mismos edificios que el toldo.
        FRandomStream Sorteo(Id * 2654435761u + 97);

        // Sólo una parte de los portales tiene garaje. Determinista por id.
        if (Sorteo.GetFraction() > 0.08f) continue;

        const TArray<TSharedPtr<FJsonValue>>* Verts = nullptr;
        if (!Edif->TryGetArrayField(TEXT("vertices"), Verts) || !Verts || Verts->Num() < 3) continue;

        FVector2D Min2(TNumericLimits<float>::Max(), TNumericLimits<float>::Max());
        FVector2D Max2(TNumericLimits<float>::Lowest(), TNumericLimits<float>::Lowest());
        for (const TSharedPtr<FJsonValue>& V : *Verts)
        {
            const TSharedPtr<FJsonObject> Vert = V->AsObject();
            if (!Vert.IsValid()) continue;
            const FVector2D P(Vert->GetNumberField(TEXT("x")), Vert->GetNumberField(TEXT("z")));
            Min2.X = FMath::Min(Min2.X, P.X); Min2.Y = FMath::Min(Min2.Y, P.Y);
            Max2.X = FMath::Max(Max2.X, P.X); Max2.Y = FMath::Max(Max2.Y, P.Y);
        }

        // Un portal de 6 m de fachada no tiene puerta de garaje de 3.
        const float LadoM = FMath::Max(Max2.X - Min2.X, Max2.Y - Min2.Y);
        if (LadoM < 12.0f) continue;

        // La misma fachada que la puerta de entrada, corrida 4 m a un lado para
        // no solaparse con ella.
        const AlsasuaDirecciones::FFachada Fachada =
            AlsasuaDirecciones::LadoDeEntrada(Id, Min2, Max2, Sorteo);

        const FVector2D Lateral(-Fachada.Fuera.Y, Fachada.Fuera.X);
        const float Desvio = (Sorteo.GetFraction() < 0.5f ? -4.0f : 4.0f);
        const FVector2D XZ = Fachada.Punto + Lateral * Desvio;

        FVector Pos = UAlsasuaGeoData::RelLocalASueloUE5(World, FVector(XZ.X, 0.0f, XZ.Y));
        Pos.Z += 150.0f;   // centro del panel: 3 m de alto apoyado en el suelo

        // 10 cm hacia afuera del plano de fachada, o se hunde en el muro.
        Pos.X += Fachada.Fuera.X * 10.0f;
        Pos.Y += Fachada.Fuera.Y * 10.0f;

        FParkingSpot Garaje;
        Garaje.Posicion = Pos;
        Garaje.Rotacion = Fachada.Yaw;
        Garaje.Tipo = TEXT("garaje");
        Garaje.bOcupado = true;
        Edif->TryGetStringField(TEXT("barrio"), Garaje.Barrio);
        Plazas.Add(Garaje);

        // El eje local X del cubo mira hacia afuera de la fachada, así que el
        // grueso del panel va en X y el ancho en Y: 15 cm de grueso, 3,2 m de
        // ancho, 3 m de alto.
        Capa->AddInstance(FTransform(
            FRotator(0.0f, Fachada.Yaw, 0.0f), Pos,
            FVector(0.15f, 3.2f, 3.0f)), /*bWorldSpace=*/true);

        ++Puestas;
    }

    UE_LOG(LogTemp, Log, TEXT("Parking: %d puertas de garaje en fachada"), Puestas);
    return Puestas;
}

int32 UAlsasuaParkingSystem::GenerarPlazasAparcamiento()
{
    if (CVarSkipParkingGeneration.GetValueOnAnyThread() != 0)
    {
        UE_LOG(LogTemp, Log, TEXT("Parking generation skipped by alsasua.SkipParkingGeneration"));
        return 0;
    }

    UWorld* World = GetWorld();
    if (!World) return 0;

    Plazas.Empty();

    UStaticMesh* Plano = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
    UStaticMesh* Cubo = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!Plano && !Cubo) return 0;

    // Fuera del bucle: antes esto se resolvía 100 veces, una por actor.
    UMaterialInterface* MatPlaza = CargarMaterialConFallback(
        TEXT("/Game/Road/Material/MI/M_Asphalt_Master_Inst_ParkingLots.M_Asphalt_Master_Inst_ParkingLots"),
        TEXT("/Game/Materiales/M_Terreno_Calles.M_Terreno_Calles"),
        TEXT("/Game/Materiales/M_Edificio.M_Edificio"));
    UMaterialInterface* MatGaraje = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Materiales/M_Metal_Guardia"));
    if (!MatGaraje) MatGaraje = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Materiales/M_Hormigon_Garaje"));

    if (Host) Host->Destroy();
    Host = World->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
    if (!Host) return 0;
    Host->SetRootComponent(NewObject<USceneComponent>(Host, TEXT("Raiz")));
    Host->GetRootComponent()->RegisterComponent();
#if WITH_EDITOR
    Host->SetActorLabel(TEXT("Aparcamiento"));
#endif

    auto CrearCapa = [&](const TCHAR* Nombre, UStaticMesh* M, UMaterialInterface* Mat, bool bColision)
        -> UHierarchicalInstancedStaticMeshComponent*
    {
        if (!M) return nullptr;
        UHierarchicalInstancedStaticMeshComponent* C =
            NewObject<UHierarchicalInstancedStaticMeshComponent>(Host, Nombre);
        C->SetStaticMesh(M);
        if (Mat) C->SetMaterial(0, Mat);
        C->SetupAttachment(Host->GetRootComponent());
        C->SetMobility(EComponentMobility::Static);
        // Las plazas van sin colisión: son pintura sobre el asfalto, y con ella
        // los LineTrace de altura de otros sistemas se subirían encima.
        C->SetCollisionEnabled(bColision ? ECollisionEnabled::QueryAndPhysics
                                         : ECollisionEnabled::NoCollision);
        C->SetCastShadow(bColision);
        C->RegisterComponent();
        return C;
    };

    UHierarchicalInstancedStaticMeshComponent* CapaPlazas =
        CrearCapa(TEXT("ISM_Plazas"), Plano, MatPlaza, /*bColision=*/false);
    UHierarchicalInstancedStaticMeshComponent* CapaGarajes =
        CrearCapa(TEXT("ISM_PuertasGaraje"), Cubo, MatGaraje, /*bColision=*/false);

    const int32 DeCalle = PintarPlazasDeCalle(CapaPlazas);
    const int32 Garajes = ColocarPuertasGaraje(CapaGarajes);

    UE_LOG(LogTemp, Log, TEXT("Parking: %d plazas (%d calle + %d garajes) en 2 capas instanciadas"),
        DeCalle + Garajes, DeCalle, Garajes);
    return DeCalle + Garajes;
}
