#include "DirectorArranque.h"
#include "ArranqueMundo.h"
#include "TerrenoGenerado.h"
#include "TerrenoLejano.h"
#include "MuestreadorAltura.h"
#include "CargadorArboles.h"
#include "CargadorVias.h"
#include "TunelAlsasua.h"
#include "CargadorCalles.h"
#include "CargadorPoligonos.h"
#include "CargadorEdificios.h"
#include "AlsasuaTejadoModular.h"
#include "HerrikoPlazaGenerator.h"
#include "CargadorPuentes.h"
#include "CargadorPOI.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystems/SubsystemCollection.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/StaticMeshActor.h"
#include "ProceduralMeshComponent.h"
#include "CalleGenerada.h"
#include "EdificioGenerado.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GeoDataAlsasua.h"
#include "AlsasuaVegetationSpawner.h"
#include "Arranque/AlsasuaPilarArranque.h"

void ADirectorArranque::BeginPlay()
{
    Super::BeginPlay();
    IniciarConstruccion();
}

void ADirectorArranque::IniciarConstruccion()
{
    ArranqueMundo::HayDirector = true;
    ArranqueMundo::BaselineListo = false;
    ArranqueMundo::SetProgress(0.f);
    UE_LOG(LogTemp, Log, TEXT("[Arranque] %s"), *ArranqueMundo::GetDebugSummary());

    UE_LOG(LogTemp, Log, TEXT("DirectorArranque: Iniciando construccion de Alsasua..."));

    UWorld* World = GetWorld();
    if (!World) return;

    // --- 1. Terreno procedural (heightmap real alsasua_landscape_4033.r16) ---
    ATerrenoGenerado* Terreno = World->SpawnActor<ATerrenoGenerado>(
        ATerrenoGenerado::StaticClass(),
        FVector::ZeroVector, FRotator::ZeroRotator);
    if (Terreno)
    {
#if WITH_EDITOR
        Terreno->SetActorLabel(TEXT("Alsasua_TerrenoProcedural"));
#endif
        UE_LOG(LogTemp, Log, TEXT("DirectorArranque: Terreno procedural spawneado."));
    }

    // --- 1b. Suelos poligonales: 5 plazas + 273 zonas verdes ---
    // Va aquí, pegado al terreno y ANTES que árboles/calles/edificios, porque
    // APoligonoSuelo muestrea Z con un LineTrace por ECC_Visibility que no filtra
    // por actor: coge lo más alto que encuentre. Con el terreno solo, drapea sobre
    // el terreno; detrás de los árboles, un vértice de zona verde bajo una copa
    // drapearía a la altura de la copa. La colisión del terreno ya está lista a
    // estas alturas: los árboles usan este mismo trace en la fase siguiente.
    UCargadorPoligonos* Suelos = World->GetSubsystem<UCargadorPoligonos>();
    if (Suelos)
    {
        const int32 NumSuelos = Suelos->Cargar();
        UE_LOG(LogTemp, Log, TEXT("DirectorArranque: %d suelos poligonales (plazas + zonas verdes)."), NumSuelos);
    }

    // --- 1c. Relieve lejano: anillo de 60 km alrededor del terreno jugable ---
    // Para que el mundo no se corte en seco a 3,6 km. Necesita el terreno ya
    // spawneado, porque le pregunta dónde acaba para dejar ahí su agujero y funde
    // las alturas contra su borde. No tiene colisión, así que no estorba a los
    // muestreos de suelo por LineTrace de las fases siguientes.
    ATerrenoLejano* Lejano = World->SpawnActor<ATerrenoLejano>(
        ATerrenoLejano::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
    if (Lejano)
    {
#if WITH_EDITOR
        Lejano->SetActorLabel(TEXT("Alsasua_RelieveLejano"));
#endif
        const int32 NumTris = Lejano->Construir();
        UE_LOG(LogTemp, Log, TEXT("DirectorArranque: relieve lejano con %d triangulos."), NumTris);
    }

    ArranqueMundo::MarkPhaseComplete(TEXT("Terreno + suelos + relieve lejano"), 0.3f);

    // --- 2. Árboles (2783 posiciones LIDAR reales) ---
    UCargadorArboles* Arboles = World->GetSubsystem<UCargadorArboles>();
    if (Arboles)
    {
        const int32 NumArboles = Arboles->Cargar();
        UE_LOG(LogTemp, Log, TEXT("DirectorArranque: %d árboles cargados (LIDAR real)."), NumArboles);
    }

    ArranqueMundo::MarkPhaseComplete(TEXT("Árboles"), 0.5f);

    // --- 3. Vías férreas (railways_unity.json real) ---
    UCargadorVias* Vias = World->GetSubsystem<UCargadorVias>();
    if (Vias)
    {
        Vias->Cargar();
        UE_LOG(LogTemp, Log, TEXT("DirectorArranque: Vías férreas cargadas."));
    }

    // --- 4. Calles (roads_unity.json real) ---
    UCargadorCalles* Calles = World->GetSubsystem<UCargadorCalles>();
    if (Calles)
    {
        Calles->Cargar();
        UE_LOG(LogTemp, Log, TEXT("DirectorArranque: Calles cargadas."));
    }

    ArranqueMundo::MarkPhaseComplete(TEXT("Vías + calles + edificios"), 0.6f);

    // --- 5. Edificios (2783 footprint LIDAR real) ---
    UCargadorEdificios* Edificios = World->GetSubsystem<UCargadorEdificios>();
    if (Edificios)
    {
        Edificios->Cargar();
        UE_LOG(LogTemp, Log, TEXT("DirectorArranque: Edificios cargados (LIDAR real)."));
    }

    // --- 5b. Remate de tejados con el kit modular (piezas Roof_ de Village) ---
    // Tiene que ir después de los edificios: lee los AEdificioGenerado ya
    // construidos para colocar las piezas en su propio alero y su cumbrera.
    UAlsasuaTejadoModular* Tejados = World->GetSubsystem<UAlsasuaTejadoModular>();
    if (Tejados)
    {
        const int32 NumPiezas = Tejados->Cargar();
        UE_LOG(LogTemp, Log, TEXT("DirectorArranque: %d piezas de tejado modular."), NumPiezas);
    }

    // --- 6. Herriko Plaza (plaza real con mobiliario) ---
    AHerrikoPlazaGenerator* Plaza = World->SpawnActor<AHerrikoPlazaGenerator>(
        AHerrikoPlazaGenerator::StaticClass(), FVector(0, 0, 100), FRotator::ZeroRotator);
    if (Plaza)
    {
        Plaza->Generar();
        UE_LOG(LogTemp, Log, TEXT("DirectorArranque: Herriko Plaza generada."));
    }

    // --- 7. Puentes (waterways_unity.json real) ---
    UCargadorPuentes* Puentes = World->GetSubsystem<UCargadorPuentes>();
    if (Puentes)
    {
        Puentes->GenerarPuentes();
        UE_LOG(LogTemp, Log, TEXT("DirectorArranque: Puentes cargados."));
    }

    // --- 8. POIs (poi_data.json real: Ayuntamiento, Iglesia, ermitas, frontón...) ---
    UCargadorPOI* POIs = World->GetSubsystem<UCargadorPOI>();
    if (POIs)
    {
        POIs->Cargar();
        UE_LOG(LogTemp, Log, TEXT("DirectorArranque: POIs cargados."));
    }

    ArranqueMundo::MarkPhaseComplete(TEXT("POIs + puentes + plaza"), 0.7f);

    // Los ríos los genera CargadorVias como cintas drapeadas sobre el terreno.
    ArranqueMundo::MarkPhaseComplete(TEXT("Vegetación + entorno urbano"), 0.8f);

    // --- 11. Mobiliario urbano (handled by specialized systems) ---
    // Removed: ContainerSystem (papelera), FarolaPlacer (farolas),
    // FountainSystem (fuentes), DetailDressingSystem (banco, bollard, buzon, etc.),
    // TrafficLightSystem (semaforos), GuardrailSystem (barandillas)
    // all read street_furniture.json or roads_unity.json directly.

    // --- 12-52. Pilares GF_* ---
    // Cada plugin activo implementa IAlsasuaPilarArranque en Kernel y ejecuta
    // su propia fase; el director solo ordena por OrdenArranque() y registra.
    // Un pilar ausente (combo sin el) salta su fase sin romper la cadena.
    AlsasuaArranqueFlags::bSemaforos = bSemaforos;

    TArray<USubsystem*> Subsistemas;
    World->ForEachSubsystem<UWorldSubsystem>([&Subsistemas](UWorldSubsystem* Sub)
    {
        Subsistemas.Add(Sub);
    });
    if (UGameInstance* GI = World->GetGameInstance())
    {
        GI->ForEachSubsystem<UGameInstanceSubsystem>([&Subsistemas](UGameInstanceSubsystem* Sub)
        {
            Subsistemas.Add(Sub);
        });
    }

    struct FPilarEnCola
    {
        IAlsasuaPilarArranque* Pilar = nullptr;
        UObject* Ejecutor = nullptr;
        int32 Orden = 500;
    };
    TArray<FPilarEnCola> Pilares;
    PilaresTiqueables.Reset();
    for (USubsystem* Sub : Subsistemas)
    {
        if (!Sub) continue;
        if (Sub->Implements<UAlsasuaPilarTiquear>())
        {
            PilaresTiqueables.Add(Sub);
        }
        if (Sub->Implements<UAlsasuaPilarArranque>())
        {
            IAlsasuaPilarArranque* P = Cast<IAlsasuaPilarArranque>(Sub);
            if (P)
            {
                FPilarEnCola Entrada;
                Entrada.Pilar = P;
                Entrada.Ejecutor = Sub;
                Entrada.Orden = P->OrdenArranque();
                Pilares.Add(Entrada);
            }
        }
    }
    Pilares.Sort([](const FPilarEnCola& A, const FPilarEnCola& B)
    {
        return A.Orden < B.Orden;
    });
    for (const FPilarEnCola& Entrada : Pilares)
    {
        const int32 Num = Entrada.Pilar->EjecutarArranque();
        UE_LOG(LogTemp, Log, TEXT("DirectorArranque: [%s] %d."),
            *Entrada.Pilar->EtiquetaArranque(), Num);
    }

    ArranqueMundo::MarkPhaseComplete(TEXT("Sistema completo"), 1.f);
    ArranqueMundo::BaselineListo = true;
    ArranqueMundo::HayDirector = false;
    bConstruccionCompleta = true;
    UE_LOG(LogTemp, Log, TEXT("DirectorArranque: BaselineListo = true (52 sistemas basados en datos reales)."));
    UE_LOG(LogTemp, Log, TEXT("[Arranque] %s"), *ArranqueMundo::GetDebugSummary());
}

void ADirectorArranque::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bConstruccionCompleta) return;

    for (const TObjectPtr<USubsystem>& Ref : PilaresTiqueables)
    {
        USubsystem* Sub = Ref.Get();
        IAlsasuaPilarTiquear* Pilar = Sub ? Cast<IAlsasuaPilarTiquear>(Sub) : nullptr;
        if (Pilar)
        {
            Pilar->TiquearPilar(DeltaTime);
        }
    }
}
