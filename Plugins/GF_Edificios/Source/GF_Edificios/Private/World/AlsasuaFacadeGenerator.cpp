#include "World/AlsasuaFacadeGenerator.h"
#include "AlsasuaMallaFab.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"
#include "AlsasuaServiceRegistry.h"

void UAlsasuaFacadeGenerator::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CargarFachadas();

    // Publish as IBuildingQueryService
    UWorld* W = GetWorld();
    if (W)
    {
        UAlsasuaServiceRegistry* Reg = UAlsasuaServiceRegistry::Get(W);
        if (Reg) Reg->Publicar(FName("Buildings"), this);
    }
}

const FBuildingFacadeEntry* UAlsasuaFacadeGenerator::De(int32 BuildingId) const
{
    for (const FBuildingFacadeEntry& E : Fachadas)
    {
        if (E.BuildingId == BuildingId) return &E;
    }
    return nullptr;
}

void UAlsasuaFacadeGenerator::Deinitialize()
{
    Fachadas.Empty();
    bCargado = false;
    Super::Deinitialize();
}

bool UAlsasuaFacadeGenerator::CargarFachadas()
{
    const FString JsonPath = FPaths::ProjectContentDir() + TEXT("Datos/building_facades.json");
    TArray<FString> Lineas;
    if (!FFileHelper::LoadFileToStringArray(Lineas, *JsonPath))
    {
        UE_LOG(LogTemp, Error, TEXT("FacadeGenerator: No se pudo cargar building_facades.json"));
        return false;
    }

    FString JsonStr;
    for (const FString& L : Lineas) JsonStr += L;

    TArray<TSharedPtr<FJsonValue>> RootArr;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    if (!FJsonSerializer::Deserialize(Reader, RootArr) || RootArr.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("FacadeGenerator: JSON invalido o array vacio"));
        return false;
    }

    Fachadas.Empty(RootArr.Num());
    for (const auto& Val : RootArr)
    {
        const TSharedPtr<FJsonObject>& Obj = Val->AsObject();
        if (!Obj) continue;

        FBuildingFacadeEntry Entry;
        Entry.BuildingId = Obj->GetIntegerField(TEXT("building_id"));
        Entry.Barrio = Obj->GetStringField(TEXT("barrio"));
        Entry.MaterialFachada = Obj->GetStringField(TEXT("material_fachada"));

        const TArray<TSharedPtr<FJsonValue>>* ColArr;
        if (Obj->TryGetArrayField(TEXT("color_fachada"), ColArr))
        {
            for (const auto& C : *ColArr)
                Entry.ColorFachada.Add(C->AsNumber());
        }

        Entry.Estilo = Obj->GetStringField(TEXT("estilo"));
        Entry.NumNiveles = Obj->GetIntegerField(TEXT("num_niveles"));
        Entry.AlturaTotal = Obj->GetNumberField(TEXT("altura_total"));
        Entry.AlturaPorNivel = Obj->GetNumberField(TEXT("altura_por_nivel"));
        Entry.PerimetroAprox = Obj->GetNumberField(TEXT("perimetro_aprox"));
        Entry.AreaAprox = Obj->GetNumberField(TEXT("area_aprox"));
        Entry.MaterialTejado = Obj->GetStringField(TEXT("material_tejado"));

        const TArray<TSharedPtr<FJsonValue>>* ColTejado;
        if (Obj->TryGetArrayField(TEXT("color_tejado"), ColTejado))
        {
            for (const auto& C : *ColTejado)
                Entry.ColorTejado.Add(C->AsNumber());
        }

        const TArray<TSharedPtr<FJsonValue>>* VentArr;
        if (Obj->TryGetArrayField(TEXT("ventanas"), VentArr))
        {
            for (const auto& V : *VentArr)
            {
                const TSharedPtr<FJsonObject>& VO = V->AsObject();
                if (!VO) continue;
                FWindowData W;
                W.Tipo = VO->GetStringField(TEXT("tipo"));
                W.Ancho = VO->GetNumberField(TEXT("ancho"));
                W.Alto = VO->GetNumberField(TEXT("alto"));
                W.MaterialMarcos = VO->GetStringField(TEXT("material_marcos"));
                W.ColorMarcos = VO->GetStringField(TEXT("color_marcos"));
                W.bConPersiana = VO->GetBoolField(TEXT("con_persiana"));
                W.bConBalcon = VO->GetBoolField(TEXT("con_balcon"));
                Entry.Ventanas.Add(W);
            }
        }

        const TArray<TSharedPtr<FJsonValue>>* BalArr;
        if (Obj->TryGetArrayField(TEXT("balcones"), BalArr))
        {
            for (const auto& B : *BalArr)
            {
                const TSharedPtr<FJsonObject>& BO = B->AsObject();
                if (!BO) continue;
                FBalconData Bal;
                Bal.Tipo = BO->GetStringField(TEXT("tipo"));
                Bal.Ancho = BO->GetNumberField(TEXT("ancho"));
                Bal.Profundidad = BO->GetNumberField(TEXT("profundidad"));
                Bal.Barandilla = BO->GetStringField(TEXT("barandilla"));
                Entry.Balcones.Add(Bal);
            }
        }

        const TArray<TSharedPtr<FJsonValue>>* TieArr;
        if (Obj->TryGetArrayField(TEXT("tiendas_planta_baja"), TieArr))
        {
            for (const auto& T : *TieArr)
            {
                const TSharedPtr<FJsonObject>& TO = T->AsObject();
                if (!TO) continue;
                FTiendaData Tie;
                Tie.Nombre = TO->GetStringField(TEXT("nombre"));
                Tie.Tipo = TO->GetStringField(TEXT("tipo"));
                Tie.AnchoM = TO->GetNumberField(TEXT("ancho_m"));
                Tie.AlturaM = TO->GetNumberField(TEXT("altura_m"));
                Tie.MaterialFachada = TO->GetStringField(TEXT("material_fachada"));
                Tie.bConToldo = TO->GetBoolField(TEXT("con_toldo"));
                Tie.ColorToldo = TO->GetStringField(TEXT("color_toldo"));
                Entry.TiendasPlantaBaja.Add(Tie);
            }
        }

        Fachadas.Add(Entry);
    }

    // Cross-reference centroids from buildings_final.json
    const FString BldPath = FPaths::ProjectContentDir() + TEXT("Datos/buildings_final.json");
    TArray<FString> BldLines;
    if (FFileHelper::LoadFileToStringArray(BldLines, *BldPath))
    {
        FString BldStr;
        for (const FString& L : BldLines) BldStr += L;
        TArray<TSharedPtr<FJsonValue>> BldRoot;
        TSharedRef<TJsonReader<>> BldReader = TJsonReaderFactory<>::Create(BldStr);
        if (FJsonSerializer::Deserialize(BldReader, BldRoot))
        {
            for (const auto& BVal : BldRoot)
            {
                const TSharedPtr<FJsonObject>& BObj = BVal->AsObject();
                if (!BObj) continue;
                const int32 BId = BObj->GetIntegerField(TEXT("id"));
                const TArray<TSharedPtr<FJsonValue>>* Vs;
                if (!BObj->TryGetArrayField(TEXT("vertices"), Vs) || Vs->Num() < 3) continue;

                FVector Centroid = FVector::ZeroVector;
                for (const auto& Pv : *Vs)
                {
                    const TSharedPtr<FJsonObject>& Po = Pv->AsObject();
                    if (!Po) continue;
                    const FVector Loc = UAlsasuaGeoData::RelLocalToUE5(FVector(
                        Po->GetNumberField(TEXT("x")), 0.0, Po->GetNumberField(TEXT("z"))));
                    Centroid += FVector(Loc.X, Loc.Y, 0.f);
                }
                Centroid /= Vs->Num();

                for (FBuildingFacadeEntry& E : Fachadas)
                {
                    if (E.BuildingId == BId) { E.Centro = Centroid; break; }
                }
            }
        }
    }

    bCargado = true;
    UE_LOG(LogTemp, Log, TEXT("FacadeGenerator: %d fachadas cargadas de building_facades.json"), Fachadas.Num());
    return true;
}






int32 UAlsasuaFacadeGenerator::ColocarLandmarksReales()
{
    UWorld* World = GetWorld();
    if (!World) return 0;

    const FString JsonPath = FPaths::ProjectContentDir() + TEXT("Datos/landmarks_real.json");
    TArray<FString> Lines;
    if (!FFileHelper::LoadFileToStringArray(Lines, *JsonPath)) return 0;

    FString Js;
    for (const FString& L : Lines) Js += L;

    TArray<TSharedPtr<FJsonValue>> Arr;
    TSharedRef<TJsonReader<>> Rd = TJsonReaderFactory<>::Create(Js);
    if (!FJsonSerializer::Deserialize(Rd, Arr)) return 0;

    int32 Placed = 0;
    for (const auto& LV : Arr)
    {
        const TSharedPtr<FJsonObject>& LO = LV->AsObject();
        if (!LO) continue;

        const FString Nombre = LO->GetStringField(TEXT("nombre"));
        const FString Tipo = LO->GetStringField(TEXT("tipo"));

        // Por lat/lon, no por x/z.
        //
        // Los x/z de landmarks_real.json no cuadran con el pueblo: contra las
        // lat/lon del mismo fichero, las distancias entre landmarks salen diez
        // veces más pequeñas y la z va al revés (el factor es -10, no +10). Con
        // x/z, los 19 quedan apiñados en 121×134 m y a 120 m de mediana del
        // edificio más cercano —ninguno cae sobre uno—; por lat/lon la mediana
        // baja a 29 m y seis caen justo encima de su footprint: la iglesia, el
        // ayuntamiento, la biblioteca, el mercado. poi_data.json arrastra lo
        // mismo. Tools/VerificarDatasets.py contrasta los dos marcos.
        double Lat = 0.0, Lon = 0.0;
        FVector Pos;
        if (LO->TryGetNumberField(TEXT("lat"), Lat) && LO->TryGetNumberField(TEXT("lon"), Lon))
        {
            Pos = UAlsasuaGeoData::LatLonToUE5(Lat, Lon);
        }
        else
        {
            Pos = UAlsasuaGeoData::AbsLocalToUE5(
                FVector(LO->GetNumberField(TEXT("x")), 0.0f, LO->GetNumberField(TEXT("z"))));
        }

        // Apoyado en el terreno: con Z = 0 los 19 landmarks quedaban medio
        // kilómetro por debajo del pueblo.
        Pos.Z = UAlsasuaGeoData::AlturaSueloUE5(World, Pos.X, Pos.Y);

        AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), Pos, FRotator::ZeroRotator);
        if (!Actor) continue;

        Actor->SetMobility(EComponentMobility::Static);

        // El "tipo" del JSON se leía y no se usaba: los 19 landmarks salían
        // como el mismo cubo gris de 4x4x5 m, una iglesia igual que una
        // ikastola y a 4 m de ancho. Ahora cada tipo tiene su arquetipo, y si
        // hay algo bajado de Fab para ese tipo, se prefiere.
        UStaticMesh* Malla = AlsasuaMallaFab::Resolver(Tipo, nullptr);
        if (!Malla)
        {
            // "parque" no es un edificio y no le corresponde malla.
            Actor->Destroy();
            continue;
        }

        Actor->GetStaticMeshComponent()->SetStaticMesh(Malla);
        // Los arquetipos ya vienen a tamaño real, en centímetros: sin escalar.
        Actor->SetActorScale3D(FVector::OneVector);

#if WITH_EDITOR
        Actor->SetActorLabel(*FString::Printf(TEXT("LMK_%s"), *Nombre));
#endif

        Placed++;
    }

    UE_LOG(LogTemp, Log, TEXT("Landmarks: %d landmarks reales colocados"), Placed);
    return Placed;
}

int32 UAlsasuaFacadeGenerator::ColocarParadasTransporte()
{
    UWorld* World = GetWorld();
    if (!World) return 0;

    const FString JsonPath = FPaths::ProjectContentDir() + TEXT("Datos/transport_alsasua.json");
    FString Js;
    if (!FFileHelper::LoadFileToString(Js, *JsonPath)) return 0;

    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Rd = TJsonReaderFactory<>::Create(Js);
    if (!FJsonSerializer::Deserialize(Rd, Root) || !Root.IsValid()) return 0;

    int32 Placed = 0, Lejos = 0, SinCoords = 0, Repetidas = 0;

    // Marquesina de UCreadorMallaMobiliario (o lo que haya de Fab). Fuera del
    // bucle: se resolvía una vez por parada.
    UStaticMesh* MallaParada = AlsasuaMallaFab::Resolver(TEXT("parada_bus"),
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!MallaParada) return 0;

    // La misma parada de Altsasu aparece en cuatro rutas de bus_routes, así que
    // sin esto salen cuatro marquesinas una dentro de otra.
    TSet<FIntPoint> YaPuestas;

    auto PlaceStop = [&](const TSharedPtr<FJsonObject>& Stop, const FString& Tipo)
    {
        if (!Stop.IsValid()) return;

        // El taxi de transport_alsasua.json no trae lat/lon. GetNumberField
        // sobre un campo que no está devuelve 0, así que la parada acababa en
        // (0, 0): el golfo de Guinea. Sin una línea en el log.
        double Lat = 0.0, Lon = 0.0;
        if (!Stop->TryGetNumberField(TEXT("lat"), Lat) || !Stop->TryGetNumberField(TEXT("lon"), Lon))
        {
            ++SinCoords;
            return;
        }

        FString Name;
        Stop->TryGetStringField(TEXT("name"), Name);

        FVector Pos = UAlsasuaGeoData::LatLonToUE5(Lat, Lon);

        // El filtro era un radio de 5000 cm —cincuenta metros— alrededor de
        // BarrioCenter("Herriko"), y ninguna de las 27 paradas del fichero cae
        // dentro: esta fase no colocaba absolutamente nada. El radio tenía
        // sentido en su intención, porque bus_routes lista las paradas de las
        // rutas interurbanas enteras —Pamplona, Vitoria, Donostia, y Madrid a
        // 303 km—, pero se llevaba por delante también las cuatro de Altsasu,
        // que están a 135-397 m del centro. El filtro bueno es el del terreno,
        // que ya es de todos.
        if (!UAlsasuaGeoData::DentroDelTerreno(Pos)) { ++Lejos; return; }

        // Rejilla de 5 m para el duplicado: las cuatro rutas dan la misma
        // parada con coordenadas idénticas, pero redondear evita depender de eso.
        const FIntPoint Celda(FMath::RoundToInt(Pos.X / 500.0), FMath::RoundToInt(Pos.Y / 500.0));
        if (YaPuestas.Contains(Celda)) { ++Repetidas; return; }
        YaPuestas.Add(Celda);

        Pos.Z = UAlsasuaGeoData::AlturaSueloUE5(World, Pos.X, Pos.Y);

        AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), Pos, FRotator::ZeroRotator);
        if (!Actor) return;

        Actor->SetMobility(EComponentMobility::Static);
        Actor->GetStaticMeshComponent()->SetStaticMesh(MallaParada);

#if WITH_EDITOR
        Actor->SetActorLabel(*FString::Printf(TEXT("STOP_%s_%s"), *Tipo, *Name.Left(20)));
#endif
        Placed++;
    };

    const TArray<TSharedPtr<FJsonValue>>* BusRoutes;
    if (Root->TryGetArrayField(TEXT("bus_routes"), BusRoutes))
    {
        for (const auto& RV : *BusRoutes)
        {
            const TSharedPtr<FJsonObject>& Route = RV->AsObject();
            if (!Route) continue;
            const TArray<TSharedPtr<FJsonValue>>* Stops;
            if (Route->TryGetArrayField(TEXT("stops"), Stops))
            {
                for (const auto& SV : *Stops)
                    PlaceStop(SV->AsObject(), TEXT("Bus"));
            }
        }
    }

    // Las cuatro paradas de bus DEL PUEBLO están en "bus_stop_points", y no las
    // leía nadie: la función sólo recorría bus_routes, que son las cabeceras de
    // las líneas interurbanas. O sea que las únicas paradas que de verdad tocan
    // a Altsasu —Surbound, Northbound, Foruen Plaza y la estación— se quedaban
    // fuera aunque el radio hubiera sido el bueno.
    const TArray<TSharedPtr<FJsonValue>>* PuntosParada;
    if (Root->TryGetArrayField(TEXT("bus_stop_points"), PuntosParada))
    {
        for (const auto& PV : *PuntosParada)
            PlaceStop(PV->AsObject(), TEXT("Bus"));
    }

    const TSharedPtr<FJsonObject>* Transport;
    if (Root->TryGetObjectField(TEXT("transport"), Transport))
    {
        const TSharedPtr<FJsonObject>* TrainSt;
        if ((*Transport)->TryGetObjectField(TEXT("train_station"), TrainSt))
            PlaceStop(*TrainSt, TEXT("Tren"));

        // bus_station tampoco lo leía nadie.
        const TSharedPtr<FJsonObject>* BusSt;
        if ((*Transport)->TryGetObjectField(TEXT("bus_station"), BusSt))
            PlaceStop(*BusSt, TEXT("Autobuses"));

        const TSharedPtr<FJsonObject>* TaxiSt;
        if ((*Transport)->TryGetObjectField(TEXT("taxi"), TaxiSt))
            PlaceStop(*TaxiSt, TEXT("Taxi"));
    }

    UE_LOG(LogTemp, Log,
        TEXT("Transport: %d paradas colocadas; %d fuera del terreno (cabeceras interurbanas), %d repetidas, %d sin lat/lon"),
        Placed, Lejos, Repetidas, SinCoords);
    return Placed;
}

// ── IBuildingQueryService ───────────────────────────────────────────────────

int32 UAlsasuaFacadeGenerator::GetBuildingCount() const
{
    return Fachadas.Num();
}

FName UAlsasuaFacadeGenerator::GetBarrioAt(const FVector& Location) const
{
    float BestDistSq = MAX_FLT;
    FName BestBarrio;
    for (const FBuildingFacadeEntry& E : Fachadas)
    {
        const float D = FVector::DistSquared(Location, E.Centro);
        if (D < BestDistSq) { BestDistSq = D; BestBarrio = *E.Barrio; }
    }
    return BestBarrio;
}

bool UAlsasuaFacadeGenerator::GetBuildingAt(const FVector& Location, float Radius, FVector& OutCenter, float& OutHeight) const
{
    const float RadiusSq = FMath::Square(Radius);
    float BestDistSq = MAX_FLT;
    const FBuildingFacadeEntry* Best = nullptr;
    for (const FBuildingFacadeEntry& E : Fachadas)
    {
        const float D = FVector::DistSquared(Location, E.Centro);
        if (D < RadiusSq && D < BestDistSq) { BestDistSq = D; Best = &E; }
    }
    if (!Best) return false;
    OutCenter = Best->Centro;
    OutHeight = Best->AlturaTotal;
    return true;
}

bool UAlsasuaFacadeGenerator::IsInteriorAt(const FVector& Location) const
{
    // Approximate: if within 200cm of any building centroid, consider interior
    const float ThresholdSq = FMath::Square(200.f);
    for (const FBuildingFacadeEntry& E : Fachadas)
    {
        if (FVector::DistSquared(Location, E.Centro) < ThresholdSq) return true;
    }
    return false;
}
