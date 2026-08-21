#include "World/AlsasuaFacadeGenerator.h"
#include "World/AlsasuaMallaFab.h"
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

void UAlsasuaFacadeGenerator::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CargarFachadas();
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
        const float X = LO->GetNumberField(TEXT("x"));
        const float Z = LO->GetNumberField(TEXT("z"));
        // Apoyado en el terreno: con Z = 0 los 19 landmarks quedaban medio
        // kilómetro por debajo del pueblo.
        FVector Pos = UAlsasuaGeoData::AbsLocalToUE5(FVector(X, 0.0f, Z));
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

    int32 Placed = 0;

    auto PlaceStop = [&](const TSharedPtr<FJsonObject>& Stop, const FString& Tipo)
    {
        if (!Stop) return;
        const float Lat = Stop->GetNumberField(TEXT("lat"));
        const float Lon = Stop->GetNumberField(TEXT("lon"));
        const FString Name = Stop->GetStringField(TEXT("name"));
        FVector Pos = UAlsasuaGeoData::LatLonToUE5(Lat, Lon);
        const FVector CentroAlsasua = UAlsasuaGeoData::AbsLocalToUE5(UAlsasuaGeoData::BarrioCenter(TEXT("Herriko")));
        // La distancia se mide en planta: si no, la diferencia de cota entre el
        // punto (Z = 0) y el centro decidía qué paradas se descartan.
        if (FVector2D::Distance(FVector2D(Pos.X, Pos.Y), FVector2D(CentroAlsasua.X, CentroAlsasua.Y)) > 5000.0f) return;
        Pos.Z = UAlsasuaGeoData::AlturaSueloUE5(World, Pos.X, Pos.Y);

        AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), Pos, FRotator::ZeroRotator);
        if (!Actor) return;

        Actor->SetMobility(EComponentMobility::Static);

        // Marquesina de UCreadorMallaMobiliario (o lo que haya de Fab) en vez
        // de un cubo del motor de 1,5 x 1,5 x 3 m.
        UStaticMesh* Malla = AlsasuaMallaFab::Resolver(TEXT("parada_bus"),
            TEXT("/Engine/BasicShapes/Cube.Cube"));
        if (Malla) Actor->GetStaticMeshComponent()->SetStaticMesh(Malla);

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

    const TSharedPtr<FJsonObject>* Transport;
    if (Root->TryGetObjectField(TEXT("transport"), Transport))
    {
        const TSharedPtr<FJsonObject>* TrainSt;
        if ((*Transport)->TryGetObjectField(TEXT("train_station"), TrainSt))
            PlaceStop(*TrainSt, TEXT("Tren"));

        const TSharedPtr<FJsonObject>* TaxiSt;
        if ((*Transport)->TryGetObjectField(TEXT("taxi"), TaxiSt))
            PlaceStop(*TaxiSt, TEXT("Taxi"));
    }

    UE_LOG(LogTemp, Log, TEXT("Transport: %d paradas de transporte colocadas"), Placed);
    return Placed;
}
