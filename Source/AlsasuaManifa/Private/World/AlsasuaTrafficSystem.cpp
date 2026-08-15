#include "World/AlsasuaTrafficSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"
#include "CargarJsonComun.h"
#include "AjusteMallaComun.h"
#include "Components/StaticMeshComponent.h"
#include "World/AlsasuaMallaFab.h"

void UAlsasuaTrafficSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    // Los coches y las señales se calculan al colocarlos, no aquí: en
    // Initialize todavía no hay terreno sobre el que apoyarlos.
}

void UAlsasuaTrafficSystem::Deinitialize()
{
    Coches.Empty();
    SenalesTrafico.Empty();
    Super::Deinitialize();
}

void UAlsasuaTrafficSystem::GenerarCochesDesdeCalles()
{
    // La raíz de roads_unity.json es un array, no un objeto con campo "roads".
    // Leerla como FJsonObject devolvía false y esta función salía sin generar
    // nada: cero coches aparcados en todo el pueblo, sin un aviso en el log.
    TArray<TSharedPtr<FJsonValue>> Arr;
    if (!JsonDatos::CargarArray(TEXT("Datos/roads_unity.json"), Arr, { TEXT("roads") })) return;

    FRandomStream Rng(12345);
    const FString Colors[] = {TEXT("blanco"), TEXT("negro"), TEXT("gris"), TEXT("rojo"), TEXT("azul"),
        TEXT("plata"), TEXT("azul_oscuro"), TEXT("verde")};
    const int32 NumColors = 8;

    for (const auto& Val : Arr)
    {
        const TSharedPtr<FJsonObject>& Obj = Val->AsObject();
        if (!Obj) continue;

        const FString Tipo = Obj->HasField(TEXT("type")) ? Obj->GetStringField(TEXT("type")) : TEXT("residential");
        const float AnchoVia = Obj->HasField(TEXT("width")) ? Obj->GetNumberField(TEXT("width")) : 8.0f;

        if (Tipo == TEXT("pedestrian") || Tipo == TEXT("path") || Tipo == TEXT("footway")) continue;
        if (AnchoVia < 5.0f) continue;

        const TArray<TSharedPtr<FJsonValue>>* PointsArr;
        if (!Obj->TryGetArrayField(TEXT("points"), PointsArr) || !PointsArr || PointsArr->Num() == 0) continue;
        const TSharedPtr<FJsonObject>& FirstPt = (*PointsArr)[0]->AsObject();
        if (!FirstPt) continue;
        const float RawX = FirstPt->GetNumberField(TEXT("x"));
        const float RawZ = FirstPt->GetNumberField(TEXT("z"));
        const float X = RawX + UAlsasuaGeoData::OX;
        const float Z = RawZ + UAlsasuaGeoData::OZ;

        int32 NumCoches = (AnchoVia > 10.0f) ? 2 : 1;
        for (int32 i = 0; i < NumCoches; i++)
        {
            if (Rng.GetFraction() > 0.3f) continue;

            FParkedCar Car;
            Car.Calle = Obj->HasField(TEXT("name")) ? Obj->GetStringField(TEXT("name")) : TEXT("");
            Car.X = X + Rng.FRandRange(-2.0f, 2.0f);
            Car.Z = Z + Rng.FRandRange(-2.0f, 2.0f);
            Car.Rotacion = Rng.FRandRange(0.0f, 360.0f);
            Car.Color = Colors[Rng.RandRange(0, NumColors - 1)];
            Car.Tipo = (Rng.GetFraction() < 0.8f) ? TEXT("coche") : TEXT("furgoneta");

            Coches.Add(Car);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("TrafficSystem: %d coches aparcados generados"), Coches.Num());
}

void UAlsasuaTrafficSystem::GenerarSenalesDesdeCalles()
{
    // Misma raíz, mismo fallo silencioso que en GenerarCochesDesdeCalles: ni una
    // señal de tráfico se llegaba a generar.
    TArray<TSharedPtr<FJsonValue>> Arr2;
    if (!JsonDatos::CargarArray(TEXT("Datos/roads_unity.json"), Arr2, { TEXT("roads") })) return;

    FRandomStream Rng(54321);

    for (const auto& Val : Arr2)
    {
        const TSharedPtr<FJsonObject>& Obj = Val->AsObject();
        if (!Obj) continue;

        const float AnchoVia = Obj->HasField(TEXT("width")) ? Obj->GetNumberField(TEXT("width")) : 8.0f;

        const TArray<TSharedPtr<FJsonValue>>* PointsArr;
        if (!Obj->TryGetArrayField(TEXT("points"), PointsArr) || !PointsArr || PointsArr->Num() == 0) continue;
        const TSharedPtr<FJsonObject>& FirstPt = (*PointsArr)[0]->AsObject();
        if (!FirstPt) continue;
        const float X = FirstPt->GetNumberField(TEXT("x")) + UAlsasuaGeoData::OX;
        const float Z = FirstPt->GetNumberField(TEXT("z")) + UAlsasuaGeoData::OZ;

        if (AnchoVia > 8.0f && Rng.GetFraction() < 0.3f)
        {
            FTrafficSign Sign;
            Sign.Tipo = TEXT("ceda_el_paso");
            Sign.X = X;
            Sign.Z = Z;
            Sign.Rotacion = Rng.FRandRange(0.0f, 360.0f);
            SenalesTrafico.Add(Sign);
        }
        if (Rng.GetFraction() < 0.15f)
        {
            FTrafficSign Sign;
            Sign.Tipo = TEXT("velocidad_30");
            Sign.X = X + Rng.FRandRange(-1.0f, 1.0f);
            Sign.Z = Z + Rng.FRandRange(-1.0f, 1.0f);
            Sign.Rotacion = Rng.FRandRange(0.0f, 360.0f);
            SenalesTrafico.Add(Sign);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("TrafficSystem: %d señales de tráfico generadas"), SenalesTrafico.Num());
}

int32 UAlsasuaTrafficSystem::ColocarCocheAparcado()
{
    UWorld* World = GetWorld();
    if (!World) return 0;

    if (Coches.Num() == 0) GenerarCochesDesdeCalles();

    // Una resolución por tipo, fuera del bucle: antes se hacía un LoadObject por
    // coche de una ruta constante. Y sobre todo, esa ruta era
    // /Game/AssetsImportados/Casas/HousePack/House1 — cada coche aparcado del pueblo
    // era una CASA. Viene de arrastrar la ruta del sistema de casas; el comentario
    // que había sólo corregía la ruta, no que fuese el asset equivocado.
    // AlsasuaMallaFab busca por palabra clave y cae a forma básica si no hay nada,
    // que es el patrón de degradación del proyecto. Y como lo que devuelva puede
    // venir a cualquier escala, se ajusta al tamaño real del vehículo: un turismo
    // mide 4,3 m y una furgoneta 5,4 m, no lo que midiera el modelo de origen.
    struct FModelo { UStaticMesh* Malla; AjusteMalla::FColocacion Col; };
    auto Preparar = [](const TCHAR* Tipo, const FVector& MedidasM)
    {
        FModelo M;
        M.Malla = AlsasuaMallaFab::Resolver(Tipo, TEXT("/Engine/BasicShapes/Cube.Cube"));
        M.Col = AjusteMalla::Calcular(M.Malla, MedidasM, AlsasuaMallaFab::VieneDeFab(Tipo));
        return M;
    };
    const FModelo Turismo   = Preparar(TEXT("coche"),     FVector(4.3f, 1.8f, 1.5f));
    const FModelo Furgoneta = Preparar(TEXT("furgoneta"), FVector(5.4f, 2.0f, 2.3f));

    int32 Placed = 0;
    for (const FParkedCar& Car : Coches)
    {
        const FModelo& M = (Car.Tipo == TEXT("furgoneta")) ? Furgoneta : Turismo;
        if (!M.Col.bValido) continue;

        // Car.X/Car.Z son local ABSOLUTO en metros (ya llevan OX/OZ sumados), así
        // que van por AbsLocalToUE5. Antes pasaban por UnityaUnreal(X, Z, 0), que
        // interpreta el segundo componente como altura: los coches salían todos
        // sobre la línea norte=0 y flotando a la altura de su coordenada norte,
        // ochenta y tantos metros en el aire. No se veía porque el cargador de
        // calles fallaba antes y la lista de coches estaba vacía.
        FVector Loc = UAlsasuaGeoData::AbsLocalToUE5(FVector(Car.X, 0.0, Car.Z));
        Loc.Z = UAlsasuaGeoData::AlturaSueloUE5(World, Loc.X, Loc.Y) + M.Col.SubirCm;

        AStaticMeshActor* CarActor = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), Loc, FRotator(0, Car.Rotacion + M.Col.YawExtra, 0));
        if (CarActor)
        {
            CarActor->SetMobility(EComponentMobility::Movable);
            CarActor->SetActorScale3D(M.Col.Escala);
            CarActor->GetStaticMeshComponent()->SetStaticMesh(M.Malla);

#if WITH_EDITOR
            CarActor->SetActorLabel(*FString::Printf(TEXT("Coche_%s_%s"), *Car.Color, *Car.Tipo));
#endif
            Placed++;
        }
    }

    UE_LOG(LogTemp, Log, TEXT("TrafficSystem: %d coches aparcados"), Placed);
    return Placed;
}

int32 UAlsasuaTrafficSystem::ColocarSenalesTrafico()
{
    UWorld* World = GetWorld();
    if (!World) return 0;

    if (SenalesTrafico.Num() == 0) GenerarSenalesDesdeCalles();

    // Estos actores se creaban sin malla ninguna: aunque hubieran llegado a
    // colocarse, habrían sido actores vacíos e invisibles. Cada tipo tiene su
    // clave en AlsasuaMallaFab; el conjunto poste+placa mide 2,4 m de alto, que
    // es la medida que manda al escalar una señal.
    struct FModelo { UStaticMesh* Malla; AjusteMalla::FColocacion Col; };
    auto Preparar = [](const TCHAR* Tipo)
    {
        FModelo M;
        M.Malla = AlsasuaMallaFab::Resolver(Tipo, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
        M.Col = AjusteMalla::Calcular(M.Malla, FVector(0.7f, 0.15f, 2.4f),
            AlsasuaMallaFab::VieneDeFab(Tipo), AjusteMalla::EEncaje::Alto);
        return M;
    };
    const FModelo Ceda      = Preparar(TEXT("señal_stop"));
    const FModelo Velocidad = Preparar(TEXT("señal_velocidad"));

    int32 Placed = 0;
    for (const FTrafficSign& Sign : SenalesTrafico)
    {
        const FModelo& M = (Sign.Tipo == TEXT("velocidad_30")) ? Velocidad : Ceda;
        if (!M.Col.bValido) continue;

        // Mismo arreglo que en los coches: local absoluto por AbsLocalToUE5 y la
        // cota por trazo contra el terreno. El "Loc.Z += 200" de antes sumaba dos
        // metros sobre una Z que ya venía mal.
        FVector Loc = UAlsasuaGeoData::AbsLocalToUE5(FVector(Sign.X, 0.0, Sign.Z));
        Loc.Z = UAlsasuaGeoData::AlturaSueloUE5(World, Loc.X, Loc.Y) + M.Col.SubirCm;

        AStaticMeshActor* SignActor = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), Loc, FRotator(0, Sign.Rotacion + M.Col.YawExtra, 0));
        if (SignActor)
        {
            SignActor->SetMobility(EComponentMobility::Movable);
            SignActor->SetActorScale3D(M.Col.Escala);
            SignActor->GetStaticMeshComponent()->SetStaticMesh(M.Malla);
#if WITH_EDITOR
            SignActor->SetActorLabel(*FString::Printf(TEXT("Trafico_%s"), *Sign.Tipo));
#endif
            Placed++;
        }
    }

    UE_LOG(LogTemp, Log, TEXT("TrafficSystem: %d señales de tráfico colocadas"), Placed);
    return Placed;
}
