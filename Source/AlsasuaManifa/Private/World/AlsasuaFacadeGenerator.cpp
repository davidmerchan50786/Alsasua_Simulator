#include "World/AlsasuaFacadeGenerator.h"
#include "World/AlsasuaMallaFab.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"
#include "AlturasLidarComun.h"

void UAlsasuaFacadeGenerator::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CargarFachadas();
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

bool UAlsasuaFacadeGenerator::CargarEdificios()
{
    const FString JsonPath = FPaths::ProjectContentDir() + TEXT("Datos/buildings_final.json");
    TArray<FString> Lineas;
    if (!FFileHelper::LoadFileToStringArray(Lineas, *JsonPath))
    {
        UE_LOG(LogTemp, Error, TEXT("FacadeGenerator: No se pudo cargar buildings_final.json"));
        return false;
    }

    FString JsonStr;
    for (const FString& L : Lineas) JsonStr += L;

    TArray<TSharedPtr<FJsonValue>> RootArr;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    if (!FJsonSerializer::Deserialize(Reader, RootArr) || RootArr.Num() == 0) return false;

    EdificiosCentros.Empty(RootArr.Num());
    for (const auto& Val : RootArr)
    {
        const TSharedPtr<FJsonObject>& Obj = Val->AsObject();
        if (!Obj) continue;

        int32 Id = Obj->GetIntegerField(TEXT("id"));
        const TArray<TSharedPtr<FJsonValue>>* Vs;
        if (!Obj->TryGetArrayField(TEXT("vertices"), Vs) || !Vs || Vs->Num() < 3) continue;

        FVector Centro(0, 0, 0);
        for (const auto& Pv : *Vs)
        {
            const TSharedPtr<FJsonObject> Po = Pv->AsObject();
            if (!Po) continue;
            const FVector M = UAlsasuaGeoData::RelLocalToUE5(FVector(Po->GetNumberField(TEXT("x")), 0.0, Po->GetNumberField(TEXT("z"))));
            Centro += M;
        }
        Centro /= Vs->Num();

        const double AlturaM = Obj->HasField(TEXT("height")) ? Obj->GetNumberField(TEXT("height")) : 6.0;
        EdificiosCentros.Add(Id, FFacadeBuildingInfo(Centro, (float)AlturaM));
    }

    UE_LOG(LogTemp, Log, TEXT("FacadeGenerator: %d centros de edificios calculados"), EdificiosCentros.Num());
    return true;
}

int32 UAlsasuaFacadeGenerator::GenerarFachadasEnMundo()
{
    if (!bCargado)
    {
        UE_LOG(LogTemp, Warning, TEXT("FacadeGenerator: Datos no cargados, llamando CargarFachadas()"));
        if (!CargarFachadas()) return 0;
    }

    if (!EdificiosCentros.Num())
    {
        CargarEdificios();
    }

    UWorld* World = GetWorld();
    if (!World) return 0;

    int32 FacadesGenerated = 0;
    for (const FBuildingFacadeEntry& Fachada : Fachadas)
    {
        FFacadeBuildingInfo* Info = EdificiosCentros.Find(Fachada.BuildingId);
        if (!Info) continue;

        AActor* Actor = World->SpawnActor<AActor>(AActor::StaticClass(), Info->Centro, FRotator::ZeroRotator);
        if (!Actor) continue;

        UProceduralMeshComponent* ProcMesh = NewObject<UProceduralMeshComponent>(Actor);
        ProcMesh->RegisterComponent();
        Actor->SetRootComponent(ProcMesh);

        const FVector FaceDirs[4] = {
            FVector(1, 0, 0),   // +X
            FVector(0, 1, 0),   // +Y
            FVector(-1, 0, 0),  // -X
            FVector(0, -1, 0)   // -Y
        };
        const FRotator FaceRots[4] = {
            FRotator(0, 0, 0),
            FRotator(0, 90, 0),
            FRotator(0, 180, 0),
            FRotator(0, 270, 0)
        };

        const float SideLen = FMath::Sqrt(Fachada.AreaAprox) * 100.0f;
        const float HalfSide = SideLen * 0.5f;

        // La altura y los niveles salen del LiDAR si esa huella está medida, y si
        // no, de building_facades.json como siempre.
        //
        // Tiene que ser la MISMA fuente que usa CargadorEdificios para el volumen.
        // Si el cuerpo del edificio sube a la altura real y las ventanas se quedan
        // con la de OSM (~3 m menos, casi una planta), queda muro desnudo por
        // encima de la última fila: se midió el desajuste en 938 edificios y sólo
        // el 31,6% de los recuentos de niveles coincidía.
        int32 Niveles = Fachada.NumNiveles;
        float AlturaTotalM = Fachada.AlturaTotal;
        float AlturaNivelM = Fachada.AlturaPorNivel;
        {
            float AltLidar = 0.f;
            int32 PlantasLidar = 0;
            if (AlturasLidar::Buscar(FVector2D(Info->Centro.X, Info->Centro.Y), AltLidar, PlantasLidar))
            {
                Niveles = PlantasLidar;
                AlturaTotalM = AltLidar;
                // Se reparte la altura medida entre las plantas medidas en vez de
                // arrastrar el altura_por_nivel del JSON: así la última fila de
                // ventanas cae dentro del muro y no por encima del alero.
                AlturaNivelM = AltLidar / FMath::Max(1, PlantasLidar);
            }
        }

        const float HeightCm = AlturaTotalM * 100.0f;
        const float LevelCm = AlturaNivelM * 100.0f;

        UMaterialInterface* WinMat = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));

        TArray<FVector> Verts;
        TArray<int32> Tris;
        TArray<FVector> Norms;
        TArray<FVector2D> UVs;

        for (int32 Face = 0; Face < 4; ++Face)
        {
            const FVector N = FaceDirs[Face];
            const FRotator R = FaceRots[Face];

            for (int32 Lvl = 0; Lvl < Niveles; ++Lvl)
            {
                const float ZBase = (Lvl + 0.2f) * LevelCm;
                const int32 WinCount = FMath::Max(1, (int32)(SideLen / 250.0f));
                const float Spacing = SideLen / (float)WinCount;

                for (int32 W = 0; W < WinCount; ++W)
                {
                    const FWindowData& V = Fachada.Ventanas[W % Fachada.Ventanas.Num()];
                    const float WCenterX = (W - (WinCount - 1) * 0.5f) * Spacing;
                    const float WinW = V.Ancho * 100.0f;
                    const float WinH = V.Alto * 100.0f;
                    const float DepthOffset = 1.0f;
                    const FVector Base = N * DepthOffset + FVector(WCenterX, 0, ZBase);
                    const FVector UVec = FVector(0, 0, 1);
                    const FVector VAxis = FVector::CrossProduct(N, UVec).GetSafeNormal();

                    const int32 V0 = Verts.Num();
                    Verts.Add(Base - VAxis * WinW * 0.5f);
                    Verts.Add(Base + VAxis * WinW * 0.5f);
                    Verts.Add(Base + VAxis * WinW * 0.5f + UVec * WinH);
                    Verts.Add(Base - VAxis * WinW * 0.5f + UVec * WinH);
                    Tris.Add(V0 + 0); Tris.Add(V0 + 2); Tris.Add(V0 + 1);
                    Tris.Add(V0 + 0); Tris.Add(V0 + 3); Tris.Add(V0 + 2);
                    Norms.Add(N); Norms.Add(N); Norms.Add(N); Norms.Add(N);
                    UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0));
                    UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));
                }
            }

            for (const FTiendaData& Tienda : Fachada.TiendasPlantaBaja)
            {
                const float ShopW = Tienda.AnchoM * 100.0f;
                const float ShopH = Tienda.AlturaM * 100.0f;
                const float DepthOffset = 1.5f;
                const FVector Base = N * DepthOffset + FVector(0, 0, 0);
                const FVector UVec = FVector(0, 0, 1);
                const FVector VAxis = FVector::CrossProduct(N, UVec).GetSafeNormal();

                const int32 V0 = Verts.Num();
                Verts.Add(Base - VAxis * ShopW * 0.5f);
                Verts.Add(Base + VAxis * ShopW * 0.5f);
                Verts.Add(Base + VAxis * ShopW * 0.5f + UVec * ShopH);
                Verts.Add(Base - VAxis * ShopW * 0.5f + UVec * ShopH);
                Tris.Add(V0 + 0); Tris.Add(V0 + 2); Tris.Add(V0 + 1);
                Tris.Add(V0 + 0); Tris.Add(V0 + 3); Tris.Add(V0 + 2);
                Norms.Add(N); Norms.Add(N); Norms.Add(N); Norms.Add(N);
                UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0));
                UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));
            }
        }

        // Una sola sección por edificio: los ~60k draw calls de quads por ventana
        // ahogaban la GPU. 1030 secciones = 1030 draw calls.
        ProcMesh->CreateMeshSection_LinearColor(0,
            Verts, Tris, Norms, UVs, TArray<FLinearColor>(), TArray<FProcMeshTangent>(), true);

        if (WinMat) ProcMesh->SetMaterial(0, WinMat);

#if WITH_EDITOR
        Actor->SetActorLabel(*FString::Printf(TEXT("Fachada_%d"), Fachada.BuildingId));
#endif

        FacadesGenerated++;
    }

    UE_LOG(LogTemp, Log, TEXT("FacadeGenerator: %d fachadas generadas (1 seccion por edificio)"), FacadesGenerated);
    return FacadesGenerated;
}

void UAlsasuaFacadeGenerator::CrearVentanaProcedural(AActor* Owner, const FWindowData& Ventana,
    const FVector& Pos, const FRotator& Rot, float Escala)
{
}

void UAlsasuaFacadeGenerator::CrearBalconProcedural(AActor* Owner, const FBalconData& Balcon,
    const FVector& Pos, const FRotator& Rot)
{
}

void UAlsasuaFacadeGenerator::CrearTiendaProcedural(AActor* Owner, const FTiendaData& Tienda,
    const FVector& Pos, const FRotator& Rot)
{
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
        const FVector Pos = UAlsasuaGeoData::AbsLocalToUE5(FVector(X, 0.0f, Z));

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
        const FVector Pos = UAlsasuaGeoData::LatLonToUE5(Lat, Lon);
        const FVector CentroAlsasua = UAlsasuaGeoData::AbsLocalToUE5(UAlsasuaGeoData::BarrioCenter(TEXT("Herriko")));
        if (FVector::Dist(Pos, CentroAlsasua) > 5000.0f) return;

        AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), Pos, FRotator::ZeroRotator);
        if (!Actor) return;

        Actor->SetMobility(EComponentMobility::Static);
        Actor->SetActorScale3D(FVector(1.5f, 1.5f, 3.0f));

        UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/EngineMeshes/Cube"));
        if (Cube) Actor->GetStaticMeshComponent()->SetStaticMesh(Cube);

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
