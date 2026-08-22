#include "World/AlsasuaPaintedStreetSignSystem.h"
#include "World/AlsasuaMuros.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Components/SceneComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Math/RandomStream.h"
#include "GeoDataAlsasua.h"
#include "CargarJsonComun.h"

void UAlsasuaPaintedStreetSignSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

int32 UAlsasuaPaintedStreetSignSystem::ColocarRotulosPintados()
{
    UWorld* World = GetWorld();
    if (!World) return 0;

    TArray<TSharedPtr<FJsonValue>> Vias;
    if (!JsonDatos::CargarArray(TEXT("Datos/roads_unity.json"), Vias, { TEXT("roads") }))
        return 0;

    Rotulos.Empty();

    if (Host) Host->Destroy();
    Host = World->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
    if (!Host) return 0;
    Host->SetRootComponent(NewObject<USceneComponent>(Host, TEXT("Raiz")));
    Host->GetRootComponent()->RegisterComponent();
#if WITH_EDITOR
    Host->SetActorLabel(TEXT("RotulosDeCalle"));
#endif

    // El cubo, no el plano: /Engine/BasicShapes/Plane es un plano en XY mirando
    // hacia arriba y escalarle la Z no lo pone de pie. La placa quedaba tumbada
    // en el suelo, en el eje de la calzada y a 2,5 m de altura.
    UStaticMesh* Cubo = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!Cubo) return 0;

    UHierarchicalInstancedStaticMeshComponent* Capa =
        NewObject<UHierarchicalInstancedStaticMeshComponent>(Host, TEXT("ISM_Rotulos"));
    Capa->SetStaticMesh(Cubo);
    // Fuera del bucle: antes se resolvía una vez por rótulo.
    if (UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Materiales/M_Rotulo_Pared")))
        Capa->SetMaterial(0, Mat);
    Capa->SetupAttachment(Host->GetRootComponent());
    Capa->SetMobility(EComponentMobility::Static);
    Capa->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Capa->SetCastShadow(false);
    Capa->RegisterComponent();

    // Una placa por calle, no por tramo. Los 489 trazados de roads_unity.json
    // son 231 calles: sin agrupar, el tope de 40 se gastaba en repetir la placa
    // de las primeras calles del fichero, que además no es orden geográfico.
    // La calle se marca vista salga o no la placa: si sólo se marcara al
    // colocarla, una calle sin edificio cerca volvería a barrer los 6000 tramos
    // de muro en cada uno de sus trazados, y además se contaría varias veces.
    TSet<FString> YaVista;
    int32 Placed = 0, SinMuro = 0;

    for (const TSharedPtr<FJsonValue>& VV : Vias)
    {
        if (Placed >= MaxRotulos) break;

        const TSharedPtr<FJsonObject> Via = VV->AsObject();
        if (!Via.IsValid()) continue;

        FString NombreES, NombreEU, Barrio;
        Via->TryGetStringField(TEXT("name"), NombreES);
        Via->TryGetStringField(TEXT("name_eu"), NombreEU);
        Via->TryGetStringField(TEXT("barrio"), Barrio);
        if (NombreES.IsEmpty()) continue;
        if (YaVista.Contains(NombreES)) continue;

        const TArray<TSharedPtr<FJsonValue>>* Pts = nullptr;
        if (!Via->TryGetArrayField(TEXT("points"), Pts) || !Pts || Pts->Num() < 2) continue;

        const TSharedPtr<FJsonObject> P0 = (*Pts)[0]->AsObject();
        if (!P0.IsValid()) continue;

        YaVista.Add(NombreES);

        const FVector2D InicioCalle(P0->GetNumberField(TEXT("x")), P0->GetNumberField(TEXT("z")));

        // La placa de una calle va en la esquina que da a esa calle. Antes se
        // clavaba en el eje de la calzada con un giro sorteado: flotando en
        // mitad de la vía y mirando a donde cayera.
        const float AnchoCm = FMath::Max(NombreES.Len(), NombreEU.Len()) * 25.0f + 50.0f;
        const AlsasuaMuros::FMuro* Muro =
            AlsasuaMuros::MasCercano(InicioCalle, /*RadioMaxM=*/30.0f, /*LargoMinimoM=*/AnchoCm * 0.01f);
        if (!Muro)
        {
            // Calle sin edificio a menos de 30 m: carretera de salida, camino o
            // vía de servicio. No hay muro donde pintar, y colgar la placa del
            // aire es exactamente lo que hacía antes.
            ++SinMuro;
            continue;
        }

        const int32 IdVia = Via->HasField(TEXT("id")) ? Via->GetIntegerField(TEXT("id")) : 0;
        FRandomStream Sorteo(IdVia * 2654435761u + 53);

        // Punto del muro más cercano al inicio de la calle, sin salirse por los
        // extremos: la placa va en la esquina, pero entera dentro del paño.
        const FVector2D AB = Muro->B - Muro->A;
        const float Len2 = AB.SizeSquared();
        const float MargenT = (Len2 > KINDA_SMALL_NUMBER)
            ? (AnchoCm * 0.5f * 0.01f) / FMath::Sqrt(Len2) : 0.5f;
        const float T = (Len2 > KINDA_SMALL_NUMBER)
            ? FMath::Clamp(FVector2D::DotProduct(InicioCalle - Muro->A, AB) / Len2,
                           MargenT, 1.0f - MargenT)
            : 0.5f;

        const FVector2D XZ = Muro->A + AB * T + Muro->Fuera * 0.03f;

        FVector Loc = UAlsasuaGeoData::RelLocalToUE5(FVector(XZ.X, 0.0f, XZ.Y));
        // 2,6 m: altura de placa de calle, por encima de una persona y por
        // debajo del forjado de la primera planta.
        Loc.Z = UAlsasuaGeoData::AlturaSueloUE5(World, Loc.X, Loc.Y) + 260.0f;

        FPaintedSignEntry Rotulo;
        Rotulo.NombreES = NombreES;
        Rotulo.NombreEU = NombreEU;
        Rotulo.Posicion = Loc;
        Rotulo.Rotacion = Muro->Yaw;
        Rotulo.Ancho = AnchoCm;
        Rotulo.Altura = 60.0f;
        Rotulo.Barrio = Barrio.IsEmpty() ? Muro->Barrio : Barrio;
        // El color era un sorteo por rótulo, así que la misma calle salía de un
        // color distinto en cada arranque. Sembrado por id de vía.
        static const TArray<FString> ColoresSign = {
            TEXT("azul"), TEXT("blanco"), TEXT("verde"), TEXT("marron")
        };
        Rotulo.Color = ColoresSign[Sorteo.RandHelper(ColoresSign.Num())];

        // Grueso en X, que es el eje que mira afuera del muro; ancho en Y.
        Capa->AddInstance(FTransform(
            FRotator(0.0f, Muro->Yaw, 0.0f), Loc,
            FVector(0.03f, Rotulo.Ancho / 100.0f, Rotulo.Altura / 100.0f)), /*bWorldSpace=*/true);

        Rotulos.Add(MoveTemp(Rotulo));
        ++Placed;
    }

    UE_LOG(LogTemp, Log,
        TEXT("PaintedSigns: %d placas de calle sobre muro (bilingües eu/es); %d calles sin edificio a menos de 30 m"),
        Placed, SinMuro);
    return Placed;
}
