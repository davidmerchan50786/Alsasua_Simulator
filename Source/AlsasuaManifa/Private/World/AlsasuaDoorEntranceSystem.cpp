#include "World/AlsasuaDoorEntranceSystem.h"
#include "World/AlsasuaMallaFab.h"
#include "World/AlsasuaDirecciones.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "CollisionQueryParams.h"
#include "Math/RandomStream.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"

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

    for (const auto& BldVal : *BuildingsArr)
    {
        const TSharedPtr<FJsonObject>& Bld = BldVal->AsObject();
        if (!Bld) continue;

        const int32 Id = Bld->HasField(TEXT("id")) ? Bld->GetIntegerField(TEXT("id")) : -1;
        const FString Barrio = Bld->HasField(TEXT("barrio")) ? Bld->GetStringField(TEXT("barrio")) : TEXT("Herriko");

        const TArray<TSharedPtr<FJsonValue>>* VertsArr;
        if (!Bld->TryGetArrayField(TEXT("vertices"), VertsArr) || !VertsArr || VertsArr->Num() < 3) continue;

        // Centroide y caja del footprint, en local relativo: X = x, Y = z.
        FVector2D Centro(0.f, 0.f);
        FVector2D Min2(TNumericLimits<float>::Max(), TNumericLimits<float>::Max());
        FVector2D Max2(TNumericLimits<float>::Lowest(), TNumericLimits<float>::Lowest());
        for (const auto& V : *VertsArr)
        {
            const TSharedPtr<FJsonObject>& Vert = V->AsObject();
            if (!Vert) continue;
            const FVector2D P(Vert->GetNumberField(TEXT("x")), Vert->GetNumberField(TEXT("z")));
            Centro += P;
            Min2.X = FMath::Min(Min2.X, P.X); Min2.Y = FMath::Min(Min2.Y, P.Y);
            Max2.X = FMath::Max(Max2.X, P.X); Max2.Y = FMath::Max(Max2.Y, P.Y);
        }
        Centro /= VertsArr->Num();

        // Sorteos por id: el color y el toldo eran FRand, así que cambiaban en
        // cada arranque y no se podía razonar sobre lo que se veía.
        FRandomStream Sorteo(Id * 2654435761u + 31);

        // Las cuatro fachadas candidatas: centro de cada lado de la caja, con
        // el yaw que las hace mirar afuera (+X del mundo es el este, +Y el norte).
        const FVector2D Lados[4] = {
            FVector2D(Max2.X, Centro.Y), FVector2D(Min2.X, Centro.Y),
            FVector2D(Centro.X, Max2.Y), FVector2D(Centro.X, Min2.Y) };
        const float Yaws[4] = { 0.f, 180.f, 90.f, 270.f };

        // Fachada de entrada: la que da a su calle. Antes era una moneda al aire
        // (dos FRand por edificio), así que la puerta cambiaba de fachada en cada
        // arranque; con addr:street de OSM se elige el lado más cercano al eje
        // de su calle. 374 edificios lo tienen; el resto cae al lado largo.
        const AlsasuaDirecciones::FDireccion* Dir = AlsasuaDirecciones::De(Id);
        const bool bHaciaCalle = Dir && Dir->bTienePuntoCalle;

        int32 Lado = 0;
        if (bHaciaCalle)
        {
            float MejorDist2 = TNumericLimits<float>::Max();
            for (int32 i = 0; i < 4; ++i)
            {
                const float D2 = FVector2D::DistSquared(Lados[i], Dir->PuntoCalle);
                if (D2 < MejorDist2) { MejorDist2 = D2; Lado = i; }
            }
        }
        else
        {
            // Sin calle conocida: el lado largo, con el sentido sorteado por id.
            const bool bLadoEnX = (Max2.X - Min2.X) >= (Max2.Y - Min2.Y);
            Lado = (bLadoEnX ? 0 : 2) + (Sorteo.GetFraction() < 0.5f ? 0 : 1);
        }

        const FVector2D PuertaXZ = Lados[Lado];
        const float DoorRot = Yaws[Lado];

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

        AStaticMeshActor* PuertaActor = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), DoorPos, FRotator(0, DoorRot, 0));
        if (PuertaActor)
        {
            PuertaActor->SetMobility(EComponentMobility::Static);
            PuertaActor->SetActorScale3D(FVector(1.0f, 0.1f, 2.2f));

            UStaticMesh* CubeMesh = AlsasuaMallaFab::Resolver(TEXT("puerta"),
                    TEXT("/Engine/BasicShapes/Cube.Cube"));
            if (CubeMesh)
                PuertaActor->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);

            UMaterialInterface* PuertaMat = LoadObject<UMaterialInterface>(nullptr,
                TEXT("/Game/Materiales/M_Puerta"));
            if (!PuertaMat)
                PuertaMat = LoadObject<UMaterialInterface>(nullptr,
                    TEXT("/Game/Materiales/M_Madera"));

            if (PuertaMat)
                PuertaActor->GetStaticMeshComponent()->SetMaterial(0, PuertaMat);

#if WITH_EDITOR
            PuertaActor->SetActorLabel(*FString::Printf(TEXT("Puerta_%d_%s%s"), Id, *Barrio.Left(6),
                Puerta.Portal.IsEmpty() ? TEXT("") : *FString::Printf(TEXT("_%s"), *Puerta.Portal)));
#endif

            // Número de portal en la fachada, junto a la puerta.
            if (!Puerta.Portal.IsEmpty())
            {
                UTextRenderComponent* Rotulo = NewObject<UTextRenderComponent>(PuertaActor);
                Rotulo->RegisterComponent();
                Rotulo->AttachToComponent(PuertaActor->GetRootComponent(),
                    FAttachmentTransformRules::KeepWorldTransform);
                // El actor de la puerta va escalado (1, 0.1, 2.2): sin escala
                // absoluta el número saldría aplastado diez veces en un eje.
                Rotulo->SetUsingAbsoluteScale(true);
                Rotulo->SetText(FText::FromString(Puerta.Portal));
                Rotulo->SetWorldSize(24.f);
                Rotulo->SetTextRenderColor(FColor(240, 238, 230));
                Rotulo->SetHorizontalAlignment(EHTA_Center);
                Rotulo->SetVerticalAlignment(EVRTA_TextCenter);

                const FRotator Frente(0.f, DoorRot, 0.f);
                const FVector Fuera = Frente.Vector();
                const FVector Lateral = FVector::CrossProduct(FVector::UpVector, Fuera);
                Rotulo->SetWorldLocation(DoorPos + Fuera * 8.f + Lateral * 75.f + FVector(0.f, 0.f, 95.f));
                // Si el número saliera del revés, es este giro: UTextRender
                // mira por su +X y basta sumar 180 al yaw.
                Rotulo->SetWorldRotation(Frente);
                Rotulo->SetCullDistance(8000.f);   // un portal no se lee a 80 m
                ++Rotulos;
            }
        }

        if (Barrio == TEXT("Herriko") && Sorteo.GetFraction() < 0.3f)
        {
            FVector ToldoPos = DoorPos;
            ToldoPos.Z += 130.0f;

            AStaticMeshActor* ToldoActor = World->SpawnActor<AStaticMeshActor>(
                AStaticMeshActor::StaticClass(), ToldoPos, FRotator(0, DoorRot, 0));
            if (ToldoActor)
            {
                ToldoActor->SetMobility(EComponentMobility::Static);
                ToldoActor->SetActorScale3D(FVector(2.0f, 1.0f, 0.05f));

                UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr,
                    TEXT("/Engine/BasicShapes/Plane.Plane"));
                if (PlaneMesh)
                    ToldoActor->GetStaticMeshComponent()->SetStaticMesh(PlaneMesh);

                UMaterialInterface* ToldoMat = LoadObject<UMaterialInterface>(nullptr,
                    TEXT("/Game/Materiales/M_Toldo"));
                if (ToldoMat)
                    ToldoActor->GetStaticMeshComponent()->SetMaterial(0, ToldoMat);

#if WITH_EDITOR
                ToldoActor->SetActorLabel(*FString::Printf(TEXT("ToldoEntrada_%d"), Id));
#endif
            }
        }

        Puertas.Add(Puerta);
        Placed++;
    }

    UE_LOG(LogTemp, Log, TEXT("Doors: %d puertas (%d en la fachada de su calle, %d con número de portal)"),
        Placed, ConCalle, Rotulos);
    return Placed;
}
