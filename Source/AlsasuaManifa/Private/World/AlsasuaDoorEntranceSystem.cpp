#include "World/AlsasuaDoorEntranceSystem.h"
#include "World/AlsasuaMallaFab.h"
#include "World/AlsasuaDirecciones.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "CollisionQueryParams.h"
#include "Math/RandomStream.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"
#include "Components/SceneComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

namespace
{
    /**
     * Cota del suelo bajo un punto del mundo.
     *
     * Las puertas salían a Z = 110 cm absolutos, y Herriko Plaza está a 531 m:
     * las 1030 estaban medio kilómetro por debajo del pueblo. Si falla el trazo
     * se usa la cota de la plaza, que al menos deja la puerta en el pueblo.
     */
    float AlturaSuelo(UWorld* World, const FVector2D& XY)
    {
        FHitResult Hit;
        const FCollisionQueryParams Q(SCENE_QUERY_STAT(AlturaPuerta), true);
        if (World && World->LineTraceSingleByChannel(Hit,
                FVector(XY.X, XY.Y, UAlsasuaGeoData::TraceUp),
                FVector(XY.X, XY.Y, UAlsasuaGeoData::TraceDown), ECC_Visibility, Q))
        {
            return Hit.Location.Z;
        }
        return UAlsasuaGeoData::CotaPlazaCm;
    }
}

void UAlsasuaDoorEntranceSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

int32 UAlsasuaDoorEntranceSystem::ColocarPuertas()
{
    const FString JsonPath = FPaths::ProjectContentDir() + TEXT("Datos/buildings_final.json");
    FString JsonStr;
    if (!FFileHelper::LoadFileToString(JsonStr, *JsonPath)) return 0;

    TSharedPtr<FJsonValue> RootVal;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    if (!FJsonSerializer::Deserialize(Reader, RootVal) || !RootVal.IsValid()) return 0;

    const TArray<TSharedPtr<FJsonValue>>* BuildingsArr;
    if (!RootVal->TryGetArray(BuildingsArr)) return 0;

    UWorld* World = GetWorld();
    if (!World) return 0;

    Puertas.Empty();
    int32 Placed = 0;
    int32 ConCalle = 0;
    int32 Rotulos = 0;

    const TArray<FString> ColoresPuerta = {
        TEXT("marron"), TEXT("verde_oscuro"), TEXT("azul_oscuro"),
        TEXT("rojo"), TEXT("gris"), TEXT("negro"), TEXT("blanco")
    };

    // Dos capas instanciadas: una de puertas (hasta 1030) y otra de toldos de
    // entrada. Antes era un AStaticMeshActor por pieza, con el LoadObject del
    // material dentro del bucle.
    //
    // El número de portal NO se puede instanciar —es un UTextRenderComponent—
    // así que sigue siendo un componente, pero colgado del actor anfitrión en
    // vez de uno propio por puerta. Son los mismos que antes; lo que desaparece
    // son los mil actores que los sostenían.
    UStaticMesh* MallaPuerta = AlsasuaMallaFab::Resolver(TEXT("puerta"),
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    UStaticMesh* MallaToldo = AlsasuaMallaFab::Resolver(TEXT("toldo"),
        TEXT("/Engine/BasicShapes/Plane.Plane"));
    if (!MallaPuerta) return 0;

    UMaterialInterface* MatPuerta = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_Puerta"));
    if (!MatPuerta) MatPuerta = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_Madera"));
    UMaterialInterface* MatToldo = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_Toldo"));

    if (Host) Host->Destroy();
    Host = World->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
    if (!Host) return 0;
    Host->SetRootComponent(NewObject<USceneComponent>(Host, TEXT("Raiz")));
    Host->GetRootComponent()->RegisterComponent();
#if WITH_EDITOR
    Host->SetActorLabel(TEXT("PuertasYPortales"));
#endif

    auto CrearCapa = [&](const TCHAR* Nombre, UStaticMesh* M, UMaterialInterface* Mat)
        -> UHierarchicalInstancedStaticMeshComponent*
    {
        if (!M) return nullptr;
        UHierarchicalInstancedStaticMeshComponent* C =
            NewObject<UHierarchicalInstancedStaticMeshComponent>(Host, Nombre);
        C->SetStaticMesh(M);
        if (Mat) C->SetMaterial(0, Mat);
        C->SetupAttachment(Host->GetRootComponent());
        C->SetMobility(EComponentMobility::Static);
        C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        C->SetCastShadow(false);
        C->RegisterComponent();
        return C;
    };
    UHierarchicalInstancedStaticMeshComponent* CapaPuertas = CrearCapa(TEXT("ISM_Puertas"), MallaPuerta, MatPuerta);
    UHierarchicalInstancedStaticMeshComponent* CapaToldos  = CrearCapa(TEXT("ISM_ToldosEntrada"), MallaToldo, MatToldo);
    if (!CapaPuertas) return 0;

    for (const auto& BldVal : *BuildingsArr)
    {
        const TSharedPtr<FJsonObject>& Bld = BldVal->AsObject();
        if (!Bld) continue;

        const int32 Id = Bld->HasField(TEXT("id")) ? Bld->GetIntegerField(TEXT("id")) : -1;
        const FString Barrio = Bld->HasField(TEXT("barrio")) ? Bld->GetStringField(TEXT("barrio")) : TEXT("Herriko");

        const TArray<TSharedPtr<FJsonValue>>* VertsArr;
        if (!Bld->TryGetArrayField(TEXT("vertices"), VertsArr) || !VertsArr || VertsArr->Num() < 3) continue;

        // Caja del footprint, en local relativo: X = x, Y = z.
        FVector2D Min2(TNumericLimits<float>::Max(), TNumericLimits<float>::Max());
        FVector2D Max2(TNumericLimits<float>::Lowest(), TNumericLimits<float>::Lowest());
        for (const auto& V : *VertsArr)
        {
            const TSharedPtr<FJsonObject>& Vert = V->AsObject();
            if (!Vert) continue;
            const FVector2D P(Vert->GetNumberField(TEXT("x")), Vert->GetNumberField(TEXT("z")));
            Min2.X = FMath::Min(Min2.X, P.X); Min2.Y = FMath::Min(Min2.Y, P.Y);
            Max2.X = FMath::Max(Max2.X, P.X); Max2.Y = FMath::Max(Max2.Y, P.Y);
        }

        // Sorteos por id: el color y el toldo eran FRand, así que cambiaban en
        // cada arranque y no se podía razonar sobre lo que se veía.
        FRandomStream Sorteo(Id * 2654435761u + 31);

        // Fachada de entrada: la que da a su calle. La elige AlsasuaDirecciones,
        // que es de donde sale el punto de calle de OSM; está ahí y no aquí
        // porque la misma fachada la usa la puerta de garaje del sistema de
        // aparcamiento, y tenerla en dos sitios es garantizar que se separen.
        const AlsasuaDirecciones::FDireccion* Dir = AlsasuaDirecciones::De(Id);
        const AlsasuaDirecciones::FFachada Fachada =
            AlsasuaDirecciones::LadoDeEntrada(Id, Min2, Max2, Sorteo);

        const FVector2D PuertaXZ = Fachada.Punto;
        const float DoorRot = Fachada.Yaw;
        const bool bHaciaCalle = Fachada.bHaciaCalle;

        // El tercer componente es la altura, no la z local: antes iba
        // DoorOffset.Z, que nunca se rellenaba, y las 1030 puertas acababan
        // sobre la línea z=0 del pueblo en vez de en su edificio.
        FVector DoorPos = UAlsasuaGeoData::RelLocalToUE5(FVector(PuertaXZ.X, 0.f, PuertaXZ.Y));
        DoorPos.Z = AlturaSuelo(World, FVector2D(DoorPos.X, DoorPos.Y)) + 110.0f;

        FDoorEntry Puerta;
        Puerta.BuildingId = Id;
        Puerta.Posicion = DoorPos;
        Puerta.Rotacion = DoorRot;
        Puerta.Tipo = (Barrio == TEXT("Herriko") || Barrio == TEXT("Harrobieta")) ?
            TEXT("madera_vieja") : TEXT("moderna");
        Puerta.Color = ColoresPuerta[Sorteo.RandHelper(ColoresPuerta.Num())];
        Puerta.Barrio = Barrio;
        if (Dir)
        {
            Puerta.Calle = Dir->Calle;
            Puerta.Portal = Dir->Portal;
        }
        if (bHaciaCalle) ++ConCalle;

        // El eje local X del cubo apunta hacia afuera de la fachada, que es lo
        // que fija DoorRot. Así que el grueso de la hoja va en X y el ancho en
        // Y: la escala era (1.0, 0.1, 2.2), o sea una puerta de 10 cm de ancho y
        // un metro de fondo, clavada de canto en el muro.
        CapaPuertas->AddInstance(FTransform(FRotator(0.f, DoorRot, 0.f), DoorPos,
            FVector(0.1f, 1.0f, 2.2f)), /*bWorldSpace=*/true);

        // Número de portal en la fachada, junto a la puerta.
        if (!Puerta.Portal.IsEmpty())
        {
            UTextRenderComponent* Rotulo = NewObject<UTextRenderComponent>(Host);
            Rotulo->RegisterComponent();
            Rotulo->AttachToComponent(Host->GetRootComponent(),
                FAttachmentTransformRules::KeepWorldTransform);
            Rotulo->SetText(FText::FromString(Puerta.Portal));
            Rotulo->SetWorldSize(24.f);
            Rotulo->SetTextRenderColor(FColor(240, 238, 230));
            Rotulo->SetHorizontalAlignment(EHTA_Center);
            Rotulo->SetVerticalAlignment(EVRTA_TextCenter);

            const FRotator Frente(0.f, DoorRot, 0.f);
            const FVector Fuera = Frente.Vector();
            const FVector Lateral = FVector::CrossProduct(FVector::UpVector, Fuera);
            Rotulo->SetWorldLocation(DoorPos + Fuera * 8.f + Lateral * 75.f + FVector(0.f, 0.f, 95.f));
            // Si el número saliera del revés, es este giro: UTextRender mira por
            // su +X y basta sumar 180 al yaw.
            Rotulo->SetWorldRotation(Frente);
            Rotulo->SetCullDistance(8000.f);   // un portal no se lee a 80 m
            ++Rotulos;
        }

        if (Barrio == TEXT("Herriko") && Sorteo.GetFraction() < 0.3f)
        {
            // Un toldo vuela hacia la calle y es más ancho que hondo: 1 m de
            // vuelo por 2 de ancho, no al revés. Y va centrado en su vuelo, no
            // en la puerta, o la mitad se queda dentro del edificio.
            const FRotator Frente(0.f, DoorRot, 0.f);
            const FVector ToldoPos = DoorPos + Frente.Vector() * 50.f + FVector(0.f, 0.f, 130.f);

            if (CapaToldos)
            {
                CapaToldos->AddInstance(FTransform(Frente, ToldoPos,
                    FVector(1.0f, 2.0f, 0.05f)), /*bWorldSpace=*/true);
            }
        }

        Puertas.Add(Puerta);
        Placed++;
    }

    UE_LOG(LogTemp, Log, TEXT("Doors: %d puertas (%d en la fachada de su calle, %d con número de portal)"),
        Placed, ConCalle, Rotulos);
    return Placed;
}
