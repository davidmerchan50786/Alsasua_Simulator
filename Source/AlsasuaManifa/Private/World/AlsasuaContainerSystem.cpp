#include "World/AlsasuaContainerSystem.h"
#include "World/AlsasuaMallaFab.h"
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
    /** Alto de un contenedor de acera, en cm. La malla básica es un cubo de 1 m. */
    constexpr float AltoContenedorCm = 110.0f;

    /** Material de cada tipo. Los crea UCreadorMaterialesSimples. */
    const TCHAR* MaterialDe(const FString& Tipo)
    {
        if (Tipo == TEXT("papel"))    return TEXT("/Game/Materiales/M_Contenedor_Papel");
        if (Tipo == TEXT("envases"))  return TEXT("/Game/Materiales/M_Contenedor_Envases");
        if (Tipo == TEXT("vidrio"))   return TEXT("/Game/Materiales/M_Contenedor_Vidrio");
        if (Tipo == TEXT("organico")) return TEXT("/Game/Materiales/M_Contenedor_Organico");
        return TEXT("/Game/Materiales/M_Contenedor_Resto");
    }
}

void UAlsasuaContainerSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

bool UAlsasuaContainerSystem::PrepararHost()
{
    UWorld* World = GetWorld();
    if (!World) return false;

    if (Host) Host->Destroy();
    Capas.Empty();

    Host = World->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
    if (!Host) return false;
    Host->SetRootComponent(NewObject<USceneComponent>(Host, TEXT("Raiz")));
    Host->GetRootComponent()->RegisterComponent();
#if WITH_EDITOR
    Host->SetActorLabel(TEXT("Contenedores"));
#endif
    return true;
}

UHierarchicalInstancedStaticMeshComponent* UAlsasuaContainerSystem::CapaDe(const FString& Tipo)
{
    if (!Host) return nullptr;

    if (TObjectPtr<UHierarchicalInstancedStaticMeshComponent>* Ya = Capas.Find(Tipo))
        return Ya->Get();

    // La ruta de CitySample no está en el repo: sin fallback, los cien
    // contenedores eran actores sin malla, o sea invisibles.
    UStaticMesh* Malla = AlsasuaMallaFab::Resolver(TEXT("papelera"),
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!Malla) return nullptr;

    UHierarchicalInstancedStaticMeshComponent* C =
        NewObject<UHierarchicalInstancedStaticMeshComponent>(Host, *FString::Printf(TEXT("ISM_%s"), *Tipo));
    C->SetStaticMesh(Malla);
    if (UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, MaterialDe(Tipo)))
        C->SetMaterial(0, Mat);
    C->SetupAttachment(Host->GetRootComponent());
    C->SetMobility(EComponentMobility::Static);
    C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    C->RegisterComponent();

    Capas.Add(Tipo, C);
    return C;
}

int32 UAlsasuaContainerSystem::ColocarContenedores()
{
    UWorld* World = GetWorld();
    if (!World) return 0;

    Contenedores.Empty();

    TArray<TSharedPtr<FJsonValue>> Mobiliario;
    if (!JsonDatos::CargarArray(TEXT("Datos/street_furniture.json"), Mobiliario, { TEXT("mobiliario") }))
    {
        UE_LOG(LogTemp, Warning, TEXT("Containers: sin street_furniture.json, se usa el fallback procedural"));
        return ColocarContenedoresFallback();
    }

    if (!PrepararHost()) return 0;

    int32 Placed = 0;
    int32 Reciclaje = 0;
    int32 Fuera = 0;

    for (const TSharedPtr<FJsonValue>& Val : Mobiliario)
    {
        const TSharedPtr<FJsonObject> Obj = Val->AsObject();
        if (!Obj.IsValid()) continue;

        FString Type;
        Obj->TryGetStringField(TEXT("type"), Type);
        const bool bReciclaje = (Type == TEXT("papelera_reciclaje"));
        if (Type != TEXT("papelera") && !bReciclaje) continue;

        double X = 0.0, Z = 0.0;
        if (!Obj->TryGetNumberField(TEXT("x"), X) || !Obj->TryGetNumberField(TEXT("z"), Z)) continue;

        FString Barrio, Calle;
        Obj->TryGetStringField(TEXT("barrio"), Barrio);
        Obj->TryGetStringField(TEXT("calle"), Calle);
        double Rot = 0.0;
        Obj->TryGetNumberField(TEXT("rotacion"), Rot);

        // El tipo de contenedor está en el dato. Sólo las de reciclaje lo traen;
        // una papelera de calle es de resto, no del vidrio.
        FString Tipo = TEXT("resto");
        if (bReciclaje)
        {
            FString TipoDato;
            if (Obj->TryGetStringField(TEXT("tipo"), TipoDato) && !TipoDato.IsEmpty())
                Tipo = TipoDato;
            ++Reciclaje;
        }

        // street_furniture.json mezcla marcos: 191 piezas en local relativo y 29
        // en absoluto. Convertirlas todas como relativas manda esas 29 a 8,6 km.
        FVector Pos = UAlsasuaGeoData::MobiliarioAUE5(FVector(X, 0.0, Z));

        // MobiliarioAUE5 propaga a Z el segundo componente, que aquí es cero:
        // hay que apoyarlo en el terreno o el contenedor sale a cota cero, 531 m
        // por debajo del pueblo.
        Pos.Z = UAlsasuaGeoData::AlturaSueloUE5(World, Pos.X, Pos.Y) + AltoContenedorCm * 0.5f;

        if (!UAlsasuaGeoData::DentroDelTerreno(Pos))
        {
            ++Fuera;
            continue;
        }

        UHierarchicalInstancedStaticMeshComponent* Capa = CapaDe(Tipo);
        if (!Capa) continue;

        Capa->AddInstance(FTransform(
            FRotator(0.0f, static_cast<float>(Rot), 0.0f), Pos,
            FVector(0.85f, 0.65f, AltoContenedorCm / 100.0f)), /*bWorldSpace=*/true);

        FContainer Cont;
        Cont.Tipo = Tipo;
        Cont.Posicion = Pos;
        Cont.Rotacion = static_cast<float>(Rot);
        Cont.Barrio = Barrio;
        Cont.Calle = Calle;
        Contenedores.Add(MoveTemp(Cont));
        ++Placed;
    }

    UE_LOG(LogTemp, Log,
        TEXT("Containers: %d contenedores (%d de reciclaje) en %d capas; %d descartados por caer fuera del terreno"),
        Placed, Reciclaje, Capas.Num(), Fuera);
    return Placed;
}

int32 UAlsasuaContainerSystem::ColocarContenedoresFallback()
{
    UWorld* World = GetWorld();
    if (!World) return 0;

    Contenedores.Empty();
    if (!PrepararHost()) return 0;

    const TArray<FString> Barrios = {
        TEXT("Herriko"), TEXT("Zelai"), TEXT("Intxostia"), TEXT("SanPedro"),
        TEXT("Errota"), TEXT("Harrobieta"), TEXT("Ferroviario"), TEXT("Monte")
    };
    const TArray<FString> Tipos = {
        TEXT("resto"), TEXT("papel"), TEXT("envases"), TEXT("vidrio"), TEXT("organico")
    };

    // Semilla fija: el fallback no tiene por qué ser irrepetible. Con FRand, los
    // contenedores cambiaban de sitio en cada arranque y una captura no valía
    // para comparar con la siguiente.
    FRandomStream Sorteo(20250816);

    int32 Placed = 0;
    for (int32 i = 0; i < MaxContenedores; ++i)
    {
        const FString& Barrio = Barrios[Sorteo.RandHelper(Barrios.Num())];
        const FString& Tipo = Tipos[Sorteo.RandHelper(Tipos.Num())];

        FVector Pos = UAlsasuaGeoData::AbsLocalToUE5(UAlsasuaGeoData::BarrioCenter(Barrio));
        Pos.X += Sorteo.FRandRange(-1000.0f, 1000.0f);
        Pos.Y += Sorteo.FRandRange(-1000.0f, 1000.0f);
        Pos.Z = UAlsasuaGeoData::AlturaSueloUE5(World, Pos.X, Pos.Y) + AltoContenedorCm * 0.5f;

        const float Rot = Sorteo.FRandRange(0.0f, 360.0f);

        UHierarchicalInstancedStaticMeshComponent* Capa = CapaDe(Tipo);
        if (!Capa) continue;

        Capa->AddInstance(FTransform(FRotator(0.0f, Rot, 0.0f), Pos,
            FVector(0.85f, 0.65f, AltoContenedorCm / 100.0f)), /*bWorldSpace=*/true);

        FContainer Cont;
        Cont.Tipo = Tipo;
        Cont.Posicion = Pos;
        Cont.Rotacion = Rot;
        Cont.Barrio = Barrio;
        Contenedores.Add(MoveTemp(Cont));
        ++Placed;
    }

    UE_LOG(LogTemp, Log, TEXT("Containers: %d contenedores del fallback procedural (sin dato real)"), Placed);
    return Placed;
}
