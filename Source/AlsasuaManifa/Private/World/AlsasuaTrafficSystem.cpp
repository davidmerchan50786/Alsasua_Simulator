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
#include "Components/SceneComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "World/AlsasuaMallaFab.h"
#include "World/AlsasuaParkingSystem.h"
#include "Engine/GameInstance.h"
#include "Math/RandomStream.h"

// Coches y señales tienen anfitrión propio: si compartieran uno, la segunda
// llamada lo recrearía y se llevaría por delante las capas de la primera.
bool UAlsasuaTrafficSystem::PrepararHost(UWorld* World, const TCHAR* Etiqueta,
    TObjectPtr<AActor>& Slot)
{
    if (!World) return false;
    if (Slot) Slot->Destroy();

    Slot = World->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
    if (!Slot) return false;
    Slot->SetRootComponent(NewObject<USceneComponent>(Slot, TEXT("Raiz")));
    Slot->GetRootComponent()->RegisterComponent();
#if WITH_EDITOR
    Slot->SetActorLabel(Etiqueta);
#else
    (void)Etiqueta;
#endif
    return true;
}

UHierarchicalInstancedStaticMeshComponent* UAlsasuaTrafficSystem::CrearCapa(
    AActor* Anfitrion, const TCHAR* Nombre, UStaticMesh* Malla)
{
    if (!Anfitrion || !Malla) return nullptr;

    UHierarchicalInstancedStaticMeshComponent* C =
        NewObject<UHierarchicalInstancedStaticMeshComponent>(Anfitrion, Nombre);
    C->SetStaticMesh(Malla);
    C->SetupAttachment(Anfitrion->GetRootComponent());
    C->SetMobility(EComponentMobility::Static);
    C->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    C->RegisterComponent();
    return C;
}

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
    // Un coche aparcado va en una plaza de aparcamiento, y las plazas las tiene
    // AlsasuaParkingSystem: espaciadas por el bordillo, retranqueadas según el
    // ancho real de la vía y ya con el bOcupado sorteado por tramo.
    //
    // Antes se sacaban de roads_unity.json aquí mismo, y salía mal por tres
    // sitios a la vez:
    //
    //  - El coche iba al PRIMER PUNTO del trazado con ±2 m de ruido. OSM parte
    //    las vías en los cruces, así que eso es el nudo: los coches aparcados
    //    en mitad de las intersecciones.
    //  - El giro era FRandRange(0, 360): apuntando a cualquier lado, atravesados
    //    en la calzada.
    //  - El filtro era `AnchoVia < 5 → fuera`, y los anchos que hay son 1.5, 2,
    //    3, 3.5, 4, 4.5, 5, 6 y 11. O sea que aparcaba en las tertiary, en la
    //    A-10 y en sus enlaces, y en ninguna de las 194 calles residenciales,
    //    que van a 4,5.
    const UAlsasuaParkingSystem* Parking = GetGameInstance()
        ? GetGameInstance()->GetSubsystem<UAlsasuaParkingSystem>() : nullptr;
    if (!Parking)
    {
        UE_LOG(LogTemp, Warning, TEXT("TrafficSystem: sin AlsasuaParkingSystem, no hay plazas donde aparcar"));
        return;
    }

    const TArray<FParkingSpot>& Plazas = Parking->GetPlazas();
    if (Plazas.Num() == 0)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("TrafficSystem: no hay plazas; ¿va la fase de aparcamiento antes que ésta?"));
        return;
    }

    const FString Colors[] = {TEXT("blanco"), TEXT("negro"), TEXT("gris"), TEXT("rojo"), TEXT("azul"),
        TEXT("plata"), TEXT("azul_oscuro"), TEXT("verde")};
    const int32 NumColors = 8;

    int32 Indice = 0;
    for (const FParkingSpot& Plaza : Plazas)
    {
        ++Indice;
        if (Plaza.Tipo != TEXT("calle") || !Plaza.bOcupado) continue;

        // Sembrado por índice de plaza: el mismo coche del mismo color en la
        // misma plaza en cada arranque.
        FRandomStream Rng(Indice * 2654435761u + 11);

        FParkedCar Car;
        Car.Calle = Plaza.Barrio;
        Car.Mundo = Plaza.Posicion;
        // A lo largo de la calzada, no atravesado. Media vuelta la mitad de las
        // veces, que es como está aparcada una calle de verdad.
        Car.Rotacion = Plaza.Rotacion + (Rng.GetFraction() < 0.5f ? 0.0f : 180.0f);
        Car.Color = Colors[Rng.RandRange(0, NumColors - 1)];
        Car.Tipo = (Rng.GetFraction() < 0.8f) ? TEXT("coche") : TEXT("furgoneta");

        Coches.Add(MoveTemp(Car));
    }

    UE_LOG(LogTemp, Log, TEXT("TrafficSystem: %d coches en plaza ocupada, de %d plazas"),
        Coches.Num(), Plazas.Num());
}

void UAlsasuaTrafficSystem::GenerarSenalesDesdeCalles()
{
    // Misma raíz, mismo fallo silencioso que en GenerarCochesDesdeCalles: ni una
    // señal de tráfico se llegaba a generar.
    TArray<TSharedPtr<FJsonValue>> Arr2;
    if (!JsonDatos::CargarArray(TEXT("Datos/roads_unity.json"), Arr2, { TEXT("roads") })) return;

    for (const auto& Val : Arr2)
    {
        const TSharedPtr<FJsonObject>& Obj = Val->AsObject();
        if (!Obj) continue;

        FString Tipo;
        Obj->TryGetStringField(TEXT("type"), Tipo);
        // El "ceda el paso" iba con AnchoVia > 8, y el único ancho por encima de
        // 8 en roads_unity.json es el 11 de la autovía: la señal se ponía sólo en
        // la A-10, donde no hay cedas. Y el límite de 30 salía en cualquier vía,
        // caminos y autovía incluidos. Van donde van: el ceda a la salida de una
        // calle residencial, el 30 en la propia calle.
        const bool bResidencial = (Tipo == TEXT("residential"));
        if (!bResidencial && Tipo != TEXT("tertiary")) continue;

        const TArray<TSharedPtr<FJsonValue>>* PointsArr;
        if (!Obj->TryGetArrayField(TEXT("points"), PointsArr) || !PointsArr || PointsArr->Num() < 2) continue;

        const int32 IdVia = Obj->HasField(TEXT("id")) ? Obj->GetIntegerField(TEXT("id")) : 0;
        FRandomStream Rng(IdVia * 2654435761u + 23);

        double AnchoM = 4.5;
        Obj->TryGetNumberField(TEXT("width"), AnchoM);

        // Punto y dirección en el extremo del trazado, que es donde OSM corta en
        // el cruce. La señal mira al conductor que llega, o sea en contra de la
        // marcha, y va al borde derecho de la calzada; antes el giro era
        // FRandRange(0, 360) y la señal quedaba de espaldas o de canto.
        auto Extremo = [&](int32 iA, int32 iB, FVector2D& OutPos, float& OutYaw)
        {
            const TSharedPtr<FJsonObject> A = (*PointsArr)[iA]->AsObject();
            const TSharedPtr<FJsonObject> B = (*PointsArr)[iB]->AsObject();
            if (!A.IsValid() || !B.IsValid()) return false;
            const FVector2D PA(A->GetNumberField(TEXT("x")), A->GetNumberField(TEXT("z")));
            const FVector2D PB(B->GetNumberField(TEXT("x")), B->GetNumberField(TEXT("z")));
            const FVector2D Dir = (PA - PB).GetSafeNormal();   // hacia el extremo
            if (Dir.IsNearlyZero()) return false;
            const FVector2D Derecha(-Dir.Y, Dir.X);
            OutPos = PA + Derecha * (static_cast<float>(AnchoM) * 0.5f + 0.8f)
                        - Dir * 1.5f;
            // Mirando en contra de la marcha, para que se lea al llegar.
            OutYaw = FMath::RadiansToDegrees(FMath::Atan2(-Dir.Y, -Dir.X));
            return true;
        };

        FVector2D Pos;
        float Yaw = 0.0f;

        if (bResidencial && Rng.GetFraction() < 0.25f
            && Extremo(PointsArr->Num() - 1, PointsArr->Num() - 2, Pos, Yaw))
        {
            FTrafficSign Sign;
            Sign.Tipo = TEXT("ceda_el_paso");
            Sign.X = Pos.X + UAlsasuaGeoData::OX;
            Sign.Z = Pos.Y + UAlsasuaGeoData::OZ;
            Sign.Rotacion = Yaw;
            SenalesTrafico.Add(Sign);
        }

        if (bResidencial && Rng.GetFraction() < 0.15f && Extremo(0, 1, Pos, Yaw))
        {
            FTrafficSign Sign;
            Sign.Tipo = TEXT("velocidad_30");
            Sign.X = Pos.X + UAlsasuaGeoData::OX;
            Sign.Z = Pos.Y + UAlsasuaGeoData::OZ;
            Sign.Rotacion = Yaw;
            SenalesTrafico.Add(Sign);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("TrafficSystem: %d señales de tráfico generadas en el borde de la calzada"),
        SenalesTrafico.Num());
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
    if (!Turismo.Col.bValido && !Furgoneta.Col.bValido) return 0;

    // Dos capas instanciadas, una por tipo. Con las 1800 plazas de calle y un
    // 25% ocupadas salen unos 450 coches, que a actor por pieza son 450 draw
    // calls sobre los ~819 de referencia del RESUMEN_TECNICO: más de la mitad
    // del presupuesto en coches aparcados.
    //
    // El color no se pierde por instanciar: Car.Color no se aplicaba a ningún
    // material, sólo salía en la etiqueta del actor en el editor. Se conserva en
    // la ficha para cuando haya materiales de coche por color.
    if (!PrepararHost(World, TEXT("CochesAparcados"), HostCoches)) return 0;
    UHierarchicalInstancedStaticMeshComponent* CapaTurismo =
        CrearCapa(HostCoches, TEXT("ISM_Coche"), Turismo.Malla);
    UHierarchicalInstancedStaticMeshComponent* CapaFurgoneta =
        CrearCapa(HostCoches, TEXT("ISM_Furgoneta"), Furgoneta.Malla);

    int32 Placed = 0;
    for (const FParkedCar& Car : Coches)
    {
        const bool bFurgo = (Car.Tipo == TEXT("furgoneta"));
        const FModelo& M = bFurgo ? Furgoneta : Turismo;
        UHierarchicalInstancedStaticMeshComponent* Capa = bFurgo ? CapaFurgoneta : CapaTurismo;
        if (!M.Col.bValido || !Capa) continue;

        // La plaza ya viene en coordenadas de mundo y con su cota muestreada en
        // el propio bordillo, así que aquí sólo falta subir el coche lo que pida
        // su malla.
        FVector Loc = Car.Mundo;
        Loc.Z += M.Col.SubirCm;

        Capa->AddInstance(FTransform(
            FRotator(0.f, Car.Rotacion + M.Col.YawExtra, 0.f), Loc, M.Col.Escala),
            /*bWorldSpace=*/true);
        Placed++;
    }

    UE_LOG(LogTemp, Log, TEXT("TrafficSystem: %d coches aparcados en 2 capas instanciadas"), Placed);
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
    if (!Ceda.Col.bValido && !Velocidad.Col.bValido) return 0;

    if (!PrepararHost(World, TEXT("SenalesTrafico"), HostSenales)) return 0;
    UHierarchicalInstancedStaticMeshComponent* CapaCeda =
        CrearCapa(HostSenales, TEXT("ISM_Ceda"), Ceda.Malla);
    UHierarchicalInstancedStaticMeshComponent* CapaVelocidad =
        CrearCapa(HostSenales, TEXT("ISM_Velocidad30"), Velocidad.Malla);

    int32 Placed = 0;
    for (const FTrafficSign& Sign : SenalesTrafico)
    {
        const bool bVel = (Sign.Tipo == TEXT("velocidad_30"));
        const FModelo& M = bVel ? Velocidad : Ceda;
        UHierarchicalInstancedStaticMeshComponent* Capa = bVel ? CapaVelocidad : CapaCeda;
        if (!M.Col.bValido || !Capa) continue;

        // Mismo arreglo que en los coches: local absoluto por AbsLocalToUE5 y la
        // cota por trazo contra el terreno. El "Loc.Z += 200" de antes sumaba dos
        // metros sobre una Z que ya venía mal.
        FVector Loc = UAlsasuaGeoData::AbsLocalToUE5(FVector(Sign.X, 0.0, Sign.Z));
        Loc.Z = UAlsasuaGeoData::AlturaSueloUE5(World, Loc.X, Loc.Y) + M.Col.SubirCm;

        Capa->AddInstance(FTransform(
            FRotator(0.f, Sign.Rotacion + M.Col.YawExtra, 0.f), Loc, M.Col.Escala),
            /*bWorldSpace=*/true);
        Placed++;
    }

    UE_LOG(LogTemp, Log, TEXT("TrafficSystem: %d señales de tráfico en 2 capas instanciadas"), Placed);
    return Placed;
}
