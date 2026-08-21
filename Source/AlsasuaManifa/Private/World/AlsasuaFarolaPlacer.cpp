#include "World/AlsasuaFarolaPlacer.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"
#include "CargarJsonComun.h"
#include "AjusteMallaComun.h"
#include "World/AlsasuaMallaFab.h"
#include "Engine/StaticMesh.h"

void UAlsasuaFarolaPlacer::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CargarFarolas();
}

void UAlsasuaFarolaPlacer::Deinitialize()
{
    Farolas.Empty();
    bCargado = false;
    Super::Deinitialize();
}

bool UAlsasuaFarolaPlacer::CargarFarolas()
{
    // street_furniture.json es un array de 220 piezas en la raíz, no un objeto con
    // campo "items". Salía por aquí y no colocaba ninguna farola.
    TArray<TSharedPtr<FJsonValue>> ItemsArr;
    if (!JsonDatos::CargarArray(TEXT("Datos/street_furniture.json"), ItemsArr, { TEXT("items") }))
    {
        UE_LOG(LogTemp, Error, TEXT("FarolaPlacer: sin mobiliario en street_furniture.json"));
        return false;
    }

    Farolas.Empty(ItemsArr.Num());
    for (const auto& Val : ItemsArr)
    {
        const TSharedPtr<FJsonObject>& Obj = Val->AsObject();
        if (!Obj) continue;

        const FString Type = Obj->GetStringField(TEXT("type"));
        if (Type != TEXT("farola") && Type != TEXT("farola_decorativa")) continue;

        FFarolaEntry F;
        F.Calle = Obj->HasField(TEXT("calle")) ? Obj->GetStringField(TEXT("calle")) : TEXT("");
        F.X = Obj->GetNumberField(TEXT("x"));
        F.Z = Obj->GetNumberField(TEXT("z"));
        F.Rotacion = Obj->HasField(TEXT("rotacion")) ? Obj->GetNumberField(TEXT("rotacion")) : 0.0f;
        F.TipoFarola = Type.Contains(TEXT("decorativa")) ? TEXT("forjado_tradicional") : TEXT("tradicional");
        F.AlturaM = Obj->HasField(TEXT("altura_m")) ? Obj->GetNumberField(TEXT("altura_m")) : 3.5f;

        Farolas.Add(F);
    }

    bCargado = true;
    UE_LOG(LogTemp, Log, TEXT("FarolaPlacer: %d farolas reales cargadas"), Farolas.Num());
    return true;
}

int32 UAlsasuaFarolaPlacer::ColocarFarolasEnMundo()
{
    if (!bCargado && !CargarFarolas()) return 0;

    UWorld* World = GetWorld();
    if (!World) return 0;

    int32 Placed = 0;

    // La malla era /Game/CitySample/..., que no está en el repo ni se baja con
    // él, y no había respaldo: las farolas se creaban como actores sin malla,
    // invisibles, contándose en el log como colocadas. Por AlsasuaMallaFab entra
    // la farola de Fab si está, la propia de /Game/Mobiliario si no, y el
    // cilindro del motor como último recurso.
    UStaticMesh* Malla = AlsasuaMallaFab::Resolver(TEXT("farola_decorativa"),
        TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (!Malla)
    {
        UE_LOG(LogTemp, Warning, TEXT("FarolaPlacer: sin malla; no se colocan las %d farolas."), Farolas.Num());
        return 0;
    }
    const bool bDeFab = AlsasuaMallaFab::VieneDeFab(TEXT("farola_decorativa"));

    // Fallback: cylinder como poste si no hay meshes de CitySample.
    if (!LampMesh1)
        LampMesh1 = LoadObject<UStaticMesh>(nullptr,
            TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (!LampMesh2)
        LampMesh2 = LampMesh1;

    for (const FFarolaEntry& F : Farolas)
    {
        // Sin esto, una farola de Fab entra con su tamaño de catálogo y el
        // cilindro del motor con su metro de alto: la altura del dataset no la
        // aplicaba nadie.
        const AjusteMalla::FColocacion Col = AjusteMalla::Calcular(
            Malla, FVector(0.25f, 0.25f, FMath::Max(2.0f, F.AlturaM)),
            bDeFab, AjusteMalla::EEncaje::Alto);
        if (!Col.bValido) continue;

        // Media altura sobre el terreno, no sobre el nivel del mar.
        // Las dos farolas del dataset están en relativo, pero street_furniture
        // mezcla marcos: si mañana se añade una en absoluto, esto la coloca donde
        // toca en vez de a 8,6 km. La cota se apoya después.
        FVector Loc = UAlsasuaGeoData::MobiliarioAUE5(FVector(F.X, 0.0f, F.Z));
        Loc.Z = UAlsasuaGeoData::AlturaSueloUE5(GetWorld(), Loc.X, Loc.Y) + Col.SubirCm;

        AStaticMeshActor* FarolaActor = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), Loc,
            FRotator(0.f, F.Rotacion + Col.YawExtra, 0.f));
        if (FarolaActor)
        {
            // Una farola no se mueve: era Movable, que además la deja fuera de
            // la iluminación estática.
            FarolaActor->SetMobility(EComponentMobility::Static);
            FarolaActor->SetActorScale3D(Col.Escala);
            FarolaActor->GetStaticMeshComponent()->SetStaticMesh(Malla);

            // Por aquí las encuentra ADirectorArranque para colgarles el
            // controlador que las enciende de noche. Antes las buscaba por
            // GetName().Contains("farola"), y GetName() devuelve el nombre de
            // objeto ("StaticMeshActor_42"), no la etiqueta del editor: no
            // encontraba ninguna y las farolas no se encendían nunca. Tags sí
            // sobrevive a un build de juego.
            FarolaActor->Tags.Add(FName(TEXT("Farola")));

            // Luz volumétrica cálida tipo sodio (Plan Fase 4).
            UPointLightComponent* Light = NewObject<UPointLightComponent>(FarolaActor);
            Light->SetRelativeLocation(FVector(0, 0, F.AlturaM * 50.0f - Loc.Z + 20.0f));
            Light->SetIntensity(3000.0f);
            Light->SetLightColor(FLinearColor(1.0f, 0.85f, 0.55f));  // sodio cálido ~2700K
            Light->SetAttenuationRadius(1500.0f);
            Light->SetSourceRadius(10.0f);
            Light->SetCastShadows(false);
            Light->SetVolumetricScatteringIntensity(2.0f);
            Light->SetIntensityUnits(ELightUnits::Lumens);
            Light->AttachToComponent(FarolaActor->GetRootComponent(),
                FAttachmentTransformRules::KeepRelativeTransform);
            Light->RegisterComponent();

#if WITH_EDITOR
            FarolaActor->SetActorLabel(*FString::Printf(TEXT("Farola_%s_%d"),
                *F.Calle, Placed));
#endif
            Placed++;
        }
    }

    UE_LOG(LogTemp, Log, TEXT("FarolaPlacer: %d farolas reales colocadas"), Placed);
    return Placed;
}
