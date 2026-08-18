#include "World/AlsasuaTreePlacer.h"
#include "World/AlsasuaMallaFab.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"
#include "CargarJsonComun.h"

void UAlsasuaTreePlacer::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    InicializarEspecies();
}

void UAlsasuaTreePlacer::Deinitialize()
{
    Arboles.Empty();
    Especies.Empty();
    bCargado = false;
    Super::Deinitialize();
}

/**
 * Género del nombre científico -> arquetipo de malla. Los nombres del censo y
 * de las especies vienen completos ("Fagus sylvatica", "Alnus glutinosa") y las
 * mallas están indexadas por género.
 */
static FString ArquetipoDeCientifico(const FString& Cientifico)
{
    if (Cientifico.StartsWith(TEXT("Quercus")))  return TEXT("QuercusRobur");
    if (Cientifico.StartsWith(TEXT("Pinus")))    return TEXT("Pinus");
    if (Cientifico.StartsWith(TEXT("Fagus")))    return TEXT("Fagus");
    if (Cientifico.StartsWith(TEXT("Betula")))   return TEXT("Betula");
    if (Cientifico.StartsWith(TEXT("Populus")))  return TEXT("Populus");
    if (Cientifico.StartsWith(TEXT("Salix")))    return TEXT("Salix");
    if (Cientifico.StartsWith(TEXT("Prunus")))   return TEXT("Prunus");
    if (Cientifico.StartsWith(TEXT("Platanus"))) return TEXT("Platanus");
    if (Cientifico.StartsWith(TEXT("Acer")))     return TEXT("Acer");
    // Aliso y níspero no tienen malla propia; comparten porte con el tilo.
    return TEXT("Tilia");
}

void UAlsasuaTreePlacer::InicializarEspecies()
{
    Especies.Empty(10);

    FTreeSpecies Aliso;
    Aliso.NombreCientifico = TEXT("Alnus glutinosa");
    Aliso.NombreEu = TEXT("Haltza");
    Aliso.NombreEs = TEXT("Aliso negro");
    Aliso.AlturaMedia = 15.0f;
    Aliso.RadioCopa = 6.0f;
    Aliso.ColorFollaje = TEXT("Verde oscuro");
    Aliso.AssetPath = TEXT("");   // no se descargó: cae a la malla procedural
    Especies.Add(Aliso);

    FTreeSpecies Haya;
    Haya.NombreCientifico = TEXT("Fagus sylvatica");
    Haya.NombreEu = TEXT("Hadia");
    Haya.NombreEs = TEXT("Haya europea");
    Haya.AlturaMedia = 25.0f;
    Haya.RadioCopa = 10.0f;
    Haya.ColorFollaje = TEXT("Verde brillante");
    Haya.AssetPath = TEXT("/Game/AssetsImportados/MeshyAI/Arbol_Haya.Arbol_Haya");
    Especies.Add(Haya);

    FTreeSpecies Abedul;
    Abedul.NombreCientifico = TEXT("Betula pendula");
    Abedul.NombreEu = TEXT("Urki");
    Abedul.NombreEs = TEXT("Abedul");
    Abedul.AlturaMedia = 18.0f;
    Abedul.RadioCopa = 5.0f;
    Abedul.ColorFollaje = TEXT("Verde claro");
    Abedul.AssetPath = TEXT("/Game/AssetsImportados/MeshyAI/Arbol_Abedul.Arbol_Abedul");
    Especies.Add(Abedul);

    FTreeSpecies Sauce;
    Sauce.NombreCientifico = TEXT("Salix alba");
    Sauce.NombreEu = TEXT("Sahats");
    Sauce.NombreEs = TEXT("Sauce");
    Sauce.AlturaMedia = 12.0f;
    Sauce.RadioCopa = 7.0f;
    Sauce.ColorFollaje = TEXT("Verde claro");
    Sauce.AssetPath = TEXT("");   // no se descargó: cae a la malla procedural
    Especies.Add(Sauce);

    FTreeSpecies PinoCarrasco;
    PinoCarrasco.NombreCientifico = TEXT("Pinus halepensis");
    PinoCarrasco.NombreEu = TEXT("Pinu horizin");
    PinoCarrasco.NombreEs = TEXT("Pino carrasco");
    PinoCarrasco.AlturaMedia = 12.0f;
    PinoCarrasco.RadioCopa = 4.0f;
    PinoCarrasco.ColorFollaje = TEXT("Verde oscuro perenne");
    PinoCarrasco.AssetPath = TEXT("/Game/AssetsImportados/Mundo/pine/snow_pine_tree.snow_pine_tree");
    Especies.Add(PinoCarrasco);

    FTreeSpecies PinoBaltico;
    PinoBaltico.NombreCientifico = TEXT("Pinus sylvestris");
    PinoBaltico.NombreEu = TEXT("Pinu larrein");
    PinoBaltico.NombreEs = TEXT("Pino baltico");
    PinoBaltico.AlturaMedia = 20.0f;
    PinoBaltico.RadioCopa = 5.0f;
    PinoBaltico.ColorFollaje = TEXT("Verde azulado perenne");
    PinoBaltico.AssetPath = TEXT("/Game/AssetsImportados/Mundo/pine/snow_pine_tree.snow_pine_tree");
    Especies.Add(PinoBaltico);

    FTreeSpecies Roble;
    Roble.NombreCientifico = TEXT("Quercus robur");
    Roble.NombreEu = TEXT("Haritz");
    Roble.NombreEs = TEXT("Roble");
    Roble.AlturaMedia = 22.0f;
    Roble.RadioCopa = 8.0f;
    Roble.ColorFollaje = TEXT("Verde oscuro");
    Roble.AssetPath = TEXT("/Game/AssetsImportados/MeshyAI/Arbol_Roble.Arbol_Roble");
    Especies.Add(Roble);

    FTreeSpecies Alamo;
    Alamo.NombreCientifico = TEXT("Populus tremula");
    Alamo.NombreEu = TEXT("Laranondo");
    Alamo.NombreEs = TEXT("Alamo temblon");
    Alamo.AlturaMedia = 20.0f;
    Alamo.RadioCopa = 6.0f;
    Alamo.ColorFollaje = TEXT("Verde claro");
    Alamo.AssetPath = TEXT("");   // no se descargó: cae a la malla procedural
    Especies.Add(Alamo);

    FTreeSpecies Nispero;
    Nispero.NombreCientifico = TEXT("Eriobotrya japonica");
    Nispero.NombreEu = TEXT("Nespera");
    Nispero.NombreEs = TEXT("Nispero");
    Nispero.AlturaMedia = 6.0f;
    Nispero.RadioCopa = 3.0f;
    Nispero.ColorFollaje = TEXT("Verde oscuro");
    Nispero.AssetPath = TEXT("");   // no se descargó: cae a la malla procedural
    Especies.Add(Nispero);
}

FString UAlsasuaTreePlacer::AsignarEspecie(float AlturaLIDAR) const
{
    if (AlturaLIDAR < 5.0f) return TEXT("Nispero");
    if (AlturaLIDAR < 8.0f) return TEXT("Sauce");
    if (AlturaLIDAR < 12.0f) return TEXT("Aliso");
    if (AlturaLIDAR < 16.0f) return TEXT("PinoCarrasco");
    if (AlturaLIDAR < 20.0f) return TEXT("Abedul");
    if (AlturaLIDAR < 25.0f) return TEXT("Alamo");
    if (AlturaLIDAR < 30.0f) return TEXT("Roble");
    return TEXT("Haya");
}

bool UAlsasuaTreePlacer::CargarArboles()
{
    // trees_unity.json es un array de 2783 árboles en la raíz, no un objeto con
    // campo "trees". La deserialización a FJsonObject fallaba y esto salía por
    // aquí: el pueblo se quedaba sin los árboles con especie ni un aviso.
    TArray<TSharedPtr<FJsonValue>> Arr;
    if (!JsonDatos::CargarArray(TEXT("Datos/trees_unity.json"), Arr, { TEXT("trees") }))
    {
        UE_LOG(LogTemp, Error, TEXT("TreePlacer: sin árboles en trees_unity.json"));
        return false;
    }

    Arboles.Empty(Arr.Num());
    for (const auto& Val : Arr)
    {
        const TSharedPtr<FJsonObject>& Obj = Val->AsObject();
        if (!Obj) continue;

        FTreePlacement Tree;
        const float X = Obj->GetNumberField(TEXT("x"));
        const float Z = Obj->GetNumberField(TEXT("z"));
        const float Altura = Obj->HasField(TEXT("altura")) ? Obj->GetNumberField(TEXT("altura")) : 10.0f;

        // trees_unity.json es local ABSOLUTO. Esto pasaba (este, norte, 0) a una
        // función que espera (este, arriba, norte): la coordenada norte acababa
        // en el eje vertical. Mientras el cargador estuvo roto no se notó.
        Tree.PosicionUnreal = UAlsasuaGeoData::AbsLocalToUE5(FVector(X, 0.0, Z));
        Tree.Especie = AsignarEspecie(Altura);
        Tree.Escala = FMath::Clamp(Altura / 15.0f, 0.5f, 2.0f);
        Tree.Rotacion = FMath::FRandRange(0.0f, 360.0f);

        Arboles.Add(Tree);
    }

    bCargado = true;
    UE_LOG(LogTemp, Log, TEXT("TreePlacer: %d arboles cargados con especies reales"), Arboles.Num());
    return true;
}

int32 UAlsasuaTreePlacer::ColocarArbolesReales()
{
    if (!bCargado && !CargarArboles()) return 0;

    UWorld* World = GetWorld();
    if (!World) return 0;

    TMap<FString, UStaticMesh*> SpecieMeshes;
    for (const FTreeSpecies& Sp : Especies)
    {
        // Cuatro de las nueve especies no se llegaron a descargar (aliso,
        // sauce, álamo y níspero: no están en Datos/asset_manifest.json), y las
        // que sí están tienen otro nombre del que pedía el código. Las que
        // falten caen a la malla procedural de su género.
        UStaticMesh* Mesh = nullptr;
        if (!Sp.AssetPath.IsEmpty())
        {
            Mesh = LoadObject<UStaticMesh>(nullptr, *Sp.AssetPath);
        }
        if (!Mesh)
        {
            Mesh = AlsasuaMallaFab::Resolver(ArquetipoDeCientifico(Sp.NombreCientifico), nullptr);
        }
        SpecieMeshes.Add(Sp.NombreCientifico, Mesh);
    }

    int32 Placed = 0;
    int32 WithMesh = 0;
    for (const FTreePlacement& Tree : Arboles)
    {
        AStaticMeshActor* TreeActor = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(),
            Tree.PosicionUnreal,
            FRotator(0, Tree.Rotacion, 0));
        if (TreeActor)
        {
            TreeActor->SetMobility(EComponentMobility::Movable);
            TreeActor->SetActorScale3D(FVector(Tree.Escala));

            UStaticMesh** FoundMesh = SpecieMeshes.Find(Tree.Especie);
            if (FoundMesh && *FoundMesh)
            {
                TreeActor->GetStaticMeshComponent()->SetStaticMesh(*FoundMesh);
                WithMesh++;
            }

#if WITH_EDITOR
            TreeActor->SetActorLabel(*FString::Printf(TEXT("Arbol_%s_%d"),
                *Tree.Especie, Placed));
#endif
            Placed++;
        }
    }

    UE_LOG(LogTemp, Log, TEXT("TreePlacer: %d arboles reales, %d con malla real"), Placed, WithMesh);
    return Placed;
}
