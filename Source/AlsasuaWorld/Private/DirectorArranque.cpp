#include "DirectorArranque.h"
#include "ArranqueMundo.h"
#include "TerrenoGenerado.h"
#include "MuestreadorAltura.h"
#include "CargadorArboles.h"
#include "CargadorVias.h"
#include "CargadorCalles.h"
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
#include "GeoDataAlsasua.h"
#include "AlsasuaVegetationSpawner.h"
#include "World/AlsasuaAtmosphereController.h"
#include "World/AlsasuaZonePostProcess.h"
#include "World/AlsasuaLODConfigComponent.h"
#include "World/AlsasuaVisualEffectsManager.h"
#include "World/AlsasuaBuildingEmissiveComponent.h"
#include "World/AlsasuaBarrioStyleSystem.h"
#include "World/AlsasuaStreetLightController.h"
#include "World/AlsasuaInteriorLightComponent.h"
#include "World/AlsasuaFacadeGenerator.h"
#include "World/AlsasuaSignPlacer.h"
#include "World/AlsasuaTreePlacer.h"
#include "World/AlsasuaFarolaPlacer.h"
#include "World/AlsasuaFoliagePainter.h"
#include "World/AlsasuaTrafficSystem.h"
#include "World/AlsasuaRoadSurfaceSystem.h"
#include "World/AlsasuaNightLightingSystem.h"
#include "World/AlsasuaWeatherSystem.h"
#include "World/AlsasuaAmbientAudioSystem.h"
#include "World/AlsasuaLODManager.h"
#include "World/AlsasuaCollisionSystem.h"
#include "World/AlsasuaVFXManager.h"
#include "World/AlsasuaShopFrontSystem.h"
#include "World/AlsasuaTerrainLayersSystem.h"
#include "World/AlsasuaNPCPedestrianSystem.h"
#include "World/AlsasuaDynamicTrafficSystem.h"
#include "World/AlsasuaSidewalkSystem.h"
#include "World/AlsasuaFountainSystem.h"
#include "World/AlsasuaDetailDressingSystem.h"
#include "World/AlsasuaBuildingInteriorSystem.h"
#include "World/AlsasuaParkingSystem.h"
#include "World/AlsasuaRoadMarkingsSystem.h"
#include "World/AlsasuaStreetArtSystem.h"
#include "World/AlsasuaContainerSystem.h"
#include "World/AlsasuaGuardrailSystem.h"
#include "World/AlsasuaTrafficLightSystem.h"
#include "World/AlsasuaAwningShutterSystem.h"
#include "World/AlsasuaRooftopDetailSystem.h"
#include "World/AlsasuaDoorEntranceSystem.h"
#include "World/AlsasuaOverheadCableSystem.h"
#include "World/AlsasuaPaintedStreetSignSystem.h"

void ADirectorArranque::BeginPlay()
{
    Super::BeginPlay();
    IniciarConstruccion();
}

void ADirectorArranque::IniciarConstruccion()
{
    ArranqueMundo::HayDirector = true;
    ArranqueMundo::BaselineListo = false;
    ArranqueMundo::Progreso = 0.f;

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

    ArranqueMundo::Progreso = 0.3f;

    // --- 2. Árboles (2783 posiciones LIDAR reales) ---
    UCargadorArboles* Arboles = World->GetSubsystem<UCargadorArboles>();
    if (Arboles)
    {
        const int32 NumArboles = Arboles->Cargar();
        UE_LOG(LogTemp, Log, TEXT("DirectorArranque: %d árboles cargados (LIDAR real)."), NumArboles);
    }

    ArranqueMundo::Progreso = 0.5f;

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

    ArranqueMundo::Progreso = 0.6f;

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

    ArranqueMundo::Progreso = 0.7f;

    // Los ríos los genera CargadorVias como cintas drapeadas sobre el terreno.
    ArranqueMundo::Progreso = 0.8f;

    // --- 11. Mobiliario urbano (handled by specialized systems) ---
    // Removed: ContainerSystem (papelera), FarolaPlacer (farolas),
    // FountainSystem (fuentes), DetailDressingSystem (banco, bollard, buzon, etc.),
    // TrafficLightSystem (semaforos), GuardrailSystem (barandillas)
    // all read street_furniture.json or roads_unity.json directly.

    // --- 12. Vegetación (greenspaces_unity.json real) ---
    UAlsasuaVegetationSpawner* Vegetacion = World->GetSubsystem<UAlsasuaVegetationSpawner>();
    if (Vegetacion)
    {
        const int32 NumVegetacion = Vegetacion->SembrarVegetacion();
        UE_LOG(LogTemp, Log, TEXT("DirectorArranque: %d piezas de vegetación sembradas."), NumVegetacion);
    }

    // --- 13. Atmósfera (sol, cielo, niebla — ciclo día/noche) ---
    UAlsasuaAtmosphereController* Atmosfera = World->GetSubsystem<UAlsasuaAtmosphereController>();
    if (Atmosfera)
    {
        UE_LOG(LogTemp, Log, TEXT("DirectorArranque: AtmosphereController inicializado."));
    }

    // --- 14. Post-process por zonas (interior/exterior + barrios) ---
    UAlsasuaZonePostProcess* ZonePP = World->GetSubsystem<UAlsasuaZonePostProcess>();
    if (ZonePP)
    {
        UE_LOG(LogTemp, Log, TEXT("DirectorArranque: ZonePostProcess inicializado."));
    }

    // --- 15. LOD Config: Nanite + HLOD ---
    UAlsasuaLODConfigComponent::ApplyGlobalNaniteSettings(true, 1);
    UAlsasuaLODConfigComponent::ApplyGlobalHLODSettings(true, 0.05f);
    UE_LOG(LogTemp, Log, TEXT("DirectorArranque: Nanite ON, HLOD ON."));

    // --- 16. Visual Effects Manager (MPC global driver) ---
    UAlsasuaVisualEffectsManager* VFXMgr = World->GetSubsystem<UAlsasuaVisualEffectsManager>();
    if (VFXMgr)
    {
        UE_LOG(LogTemp, Log, TEXT("DirectorArranque: VisualEffectsManager inicializado."));
    }

    // --- 17. Estilos de barrio reales (materiales por barrio) ---
    {
        TArray<AActor*> EdificiosArr;
        UGameplayStatics::GetAllActorsOfClass(World, AStaticMeshActor::StaticClass(), EdificiosArr);
        int32 StyleCount = 0;
        for (AActor* Actor : EdificiosArr)
        {
            if (!Actor) continue;
            const FString Label = Actor->GetName();
            if (Label.Contains(TEXT("Edificio")) || Label.Contains(TEXT("Building")))
            {
                UAlsasuaBarrioStyleSystem* Style = NewObject<UAlsasuaBarrioStyleSystem>(Actor);
                if (Style) { Style->RegisterComponent(); StyleCount++; }
            }
        }
        UE_LOG(LogTemp, Log, TEXT("DirectorArranque: %d edificios con estilos de barrio reales."), StyleCount);
    }

    // --- 18. Farolas reales (street_furniture.json) ---
    {
        TArray<AActor*> FarolaActors;
        UGameplayStatics::GetAllActorsOfClass(World, AStaticMeshActor::StaticClass(), FarolaActors);
        int32 FarolaCount = 0;
        for (AActor* Actor : FarolaActors)
        {
            if (!Actor) continue;
            const FString Label = Actor->GetName().ToLower();
            if (Label.Contains(TEXT("farola")))
            {
                UAlsasuaStreetLightController* Light = NewObject<UAlsasuaStreetLightController>(Actor);
                if (Light) { Light->RegisterComponent(); FarolaCount++; }
            }
        }
        UE_LOG(LogTemp, Log, TEXT("DirectorArranque: %d farolas reales con control automático."), FarolaCount);
    }

    // --- 19. Ventanas emissivas nocturnas (edificios reales) ---
    {
        TArray<AActor*> EdificiosArr;
        UGameplayStatics::GetAllActorsOfClass(World, AStaticMeshActor::StaticClass(), EdificiosArr);
        int32 EmissiveCount = 0;
        for (AActor* Actor : EdificiosArr)
        {
            if (!Actor) continue;
            const FString Label = Actor->GetName();
            if (Label.Contains(TEXT("Edificio")) || Label.Contains(TEXT("Building")))
            {
                UAlsasuaBuildingEmissiveComponent* Emissive =
                    NewObject<UAlsasuaBuildingEmissiveComponent>(Actor);
                if (Emissive) { Emissive->RegisterComponent(); EmissiveCount++; }
            }
        }
        UE_LOG(LogTemp, Log, TEXT("DirectorArranque: %d edificios con ventanas emissivas."), EmissiveCount);
    }

    // --- 20. Luces interiores (edificios reales) ---
    {
        TArray<AActor*> EdificiosArr;
        UGameplayStatics::GetAllActorsOfClass(World, AStaticMeshActor::StaticClass(), EdificiosArr);
        int32 InteriorCount = 0;
        for (AActor* Actor : EdificiosArr)
        {
            if (!Actor) continue;
            const FString Label = Actor->GetName();
            if (Label.Contains(TEXT("Edificio")) || Label.Contains(TEXT("Building")))
            {
                UAlsasuaInteriorLightComponent* Interior =
                    NewObject<UAlsasuaInteriorLightComponent>(Actor);
                if (Interior) { Interior->RegisterComponent(); InteriorCount++; }
            }
        }
        UE_LOG(LogTemp, Log, TEXT("DirectorArranque: %d edificios con luces interiores."), InteriorCount);
    }

    // --- 21. Fachadas reales (building_facades.json: ventanas, balcones, tiendas) ---
    {
        UAlsasuaFacadeGenerator* Facades = World->GetGameInstance()->GetSubsystem<UAlsasuaFacadeGenerator>();
        if (Facades)
        {
            const int32 NumFachadas = Facades->GenerarFachadasEnMundo();
            const int32 NumLMK = Facades->ColocarLandmarksReales();
            const int32 NumTransport = Facades->ColocarParadasTransporte();
            UE_LOG(LogTemp, Log, TEXT("DirectorArranque: %d fachadas, %d landmarks, %d paradas transporte."),
                NumFachadas, NumLMK, NumTransport);
        }
    }

    // --- 22. Señalización real (signage_data.json: calles bilingües, tráfico, comercios) ---
    {
        UAlsasuaSignPlacer* Signs = World->GetGameInstance()->GetSubsystem<UAlsasuaSignPlacer>();
        if (Signs)
        {
            const int32 NumSenales = Signs->ColocarSenalesEnMundo();
            UE_LOG(LogTemp, Log, TEXT("DirectorArranque: %d señales reales colocadas."), NumSenales);
        }
    }

    // --- 23. Árboles reales con especies (trees_unity.json + 8 especies de Navarra) ---
    {
        UAlsasuaTreePlacer* Trees = World->GetGameInstance()->GetSubsystem<UAlsasuaTreePlacer>();
        if (Trees)
        {
            const int32 NumArboles = Trees->ColocarArbolesReales();
            UE_LOG(LogTemp, Log, TEXT("DirectorArranque: %d árboles reales con especies."), NumArboles);
        }
    }

    // --- 24. Farolas reales con ubicación exacta (street_furniture.json) ---
    {
        UAlsasuaFarolaPlacer* Farolas = World->GetGameInstance()->GetSubsystem<UAlsasuaFarolaPlacer>();
        if (Farolas)
        {
            const int32 NumFarolas = Farolas->ColocarFarolasEnMundo();
            UE_LOG(LogTemp, Log, TEXT("DirectorArranque: %d farolas reales colocadas."), NumFarolas);
        }
    }

    // --- 25. Foliage procedural (hierba, setos, rocas en zonas verdes) ---
    {
        UAlsasuaFoliagePainter* Foliage = World->GetGameInstance()->GetSubsystem<UAlsasuaFoliagePainter>();
        if (Foliage)
        {
            const int32 NumFoliage = Foliage->PintarFoliageEnZonasVerdes();
            UE_LOG(LogTemp, Log, TEXT("DirectorArranque: %d piezas de foliage procedural."), NumFoliage);
        }
    }

    // --- 26. Superficies de calle (asfalto, adoquín, grava por tipo de vía) ---
    {
        UAlsasuaRoadSurfaceSystem* Roads = World->GetGameInstance()->GetSubsystem<UAlsasuaRoadSurfaceSystem>();
        if (Roads)
        {
            const int32 NumRoads = Roads->AplicarSuperficiesEnMundo();
            UE_LOG(LogTemp, Log, TEXT("DirectorArranque: %d tramos de superficie."), NumRoads);
        }
    }

    // --- 27. Tráfico procedural (coches aparcados, señales de tráfico) ---
    {
        UAlsasuaTrafficSystem* Traffic = World->GetGameInstance()->GetSubsystem<UAlsasuaTrafficSystem>();
        if (Traffic)
        {
            const int32 NumCoches = Traffic->ColocarCocheAparcado();
            const int32 NumTrafico = Traffic->ColocarSenalesTrafico();
            UE_LOG(LogTemp, Log, TEXT("DirectorArranque: %d coches, %d señales tráfico."), NumCoches, NumTrafico);
        }
    }

    // --- 28. Sistema de iluminación nocturna ---
    {
        UAlsasuaNightLightingSystem* NightSys = NewObject<UAlsasuaNightLightingSystem>(
            World->GetWorldSettings());
        if (NightSys)
        {
            NightSys->RegisterComponent();
            UE_LOG(LogTemp, Log, TEXT("DirectorArranque: Sistema nocturno activado."));
        }
    }

    // --- 29. Sistema de clima dinámico ---
    {
        UAlsasuaWeatherSystem* Weather = NewObject<UAlsasuaWeatherSystem>(
            World->GetWorldSettings());
        if (Weather)
        {
            WeatherSystemRef = Weather;
            Weather->RegisterComponent();
            UE_LOG(LogTemp, Log, TEXT("DirectorArranque: Sistema de clima activado."));
        }
    }

    // --- 30. Sistema de audio ambiental ---
    {
        UAlsasuaAmbientAudioSystem* Audio = NewObject<UAlsasuaAmbientAudioSystem>(
            World->GetWorldSettings());
        if (Audio)
        {
            AudioSystemRef = Audio;
            Audio->RegisterComponent();
            UE_LOG(LogTemp, Log, TEXT("DirectorArranque: Sistema de audio ambiental activado."));
        }
    }

    // --- 31. Colisiones de edificios y calles ---
    {
        UAlsasuaCollisionSystem* Collisions = World->GetGameInstance()->GetSubsystem<UAlsasuaCollisionSystem>();
        if (Collisions)
        {
            const int32 ColEdificios = Collisions->GenerarColisionesEdificios();
            const int32 ColCalles = Collisions->GenerarColisionesCalles();
            UE_LOG(LogTemp, Log, TEXT("DirectorArranque: %d colisiones edificios, %d calles."),
                ColEdificios, ColCalles);
        }
    }

    // --- 32. Tiendas reales (signage_data.json: bares, restaurantes, tiendas) ---
    {
        UAlsasuaShopFrontSystem* Shops = World->GetGameInstance()->GetSubsystem<UAlsasuaShopFrontSystem>();
        if (Shops)
        {
            const int32 NumTiendas = Shops->ColocarTiendasEnMundo();
            UE_LOG(LogTemp, Log, TEXT("DirectorArranque: %d tiendas reales colocadas."), NumTiendas);
        }
    }

    // --- 34. Terreno multi-capa por barrio ---
    {
        UAlsasuaTerrainLayersSystem* Terrain = World->GetGameInstance()->GetSubsystem<UAlsasuaTerrainLayersSystem>();
        if (Terrain)
        {
            Terrain->GenerarSueloCiudad();
            UE_LOG(LogTemp, Log, TEXT("DirectorArranque: Terreno multi-capa por barrio generado."));
        }
    }

    // --- 35. NPCs peatones (comportamiento en barrios) ---
    {
        UAlsasuaNPCPedestrianSystem* NPCs = World->GetGameInstance()->GetSubsystem<UAlsasuaNPCPedestrianSystem>();
        if (NPCs)
        {
            NPCSystemRef = NPCs;
            NPCs->GenerarNPCs();
            UE_LOG(LogTemp, Log, TEXT("DirectorArranque: %d peatones generados."), NPCs->GetNPCs().Num());
        }
    }

    // --- 36. Tráfico dinámico (coches en movimiento) ---
    {
        UAlsasuaDynamicTrafficSystem* TrafficDyn = World->GetGameInstance()->GetSubsystem<UAlsasuaDynamicTrafficSystem>();
        if (TrafficDyn)
        {
            TrafficSystemRef = TrafficDyn;
            TrafficDyn->IniciarTrafico();
            UE_LOG(LogTemp, Log, TEXT("DirectorArranque: Tráfico dinámico iniciado (%d vehículos)."),
                TrafficDyn->GetVehiculos().Num());
        }
    }

    // --- 37. Aceras reales (roads_unity.json) ---
    {
        UAlsasuaSidewalkSystem* Sidewalks = World->GetGameInstance()->GetSubsystem<UAlsasuaSidewalkSystem>();
        if (Sidewalks)
        {
            const int32 NumAcera = Sidewalks->GenerarAceras();
            UE_LOG(LogTemp, Log, TEXT("DirectorArranque: %d segmentos de acera."), NumAcera);
        }
    }

    // --- 38. Fuentes de agua reales (8 ubicaciones) ---
    {
        UAlsasuaFountainSystem* Fuentes = World->GetGameInstance()->GetSubsystem<UAlsasuaFountainSystem>();
        if (Fuentes)
        {
            const int32 NumFuentes = Fuentes->ColocarFuentes();
            UE_LOG(LogTemp, Log, TEXT("DirectorArranque: %d fuentes reales."), NumFuentes);
        }
    }

    // --- 39. Dressing de detalle (macetas, buzones, papeleiras, bancos, vallas) ---
    {
        UAlsasuaDetailDressingSystem* Detalle = World->GetGameInstance()->GetSubsystem<UAlsasuaDetailDressingSystem>();
        if (Detalle)
        {
            const int32 NumDetalle = Detalle->ColocarDetalle();
            UE_LOG(LogTemp, Log, TEXT("DirectorArranque: %d items de detalle."), NumDetalle);
        }
    }

    // --- 40. Interiores con iluminación (building_facons.json) ---
    {
        UE_LOG(LogTemp, Log, TEXT("DirectorArranque: skip interiores con iluminación para perfilado"));
    }

    // --- 41. Aparcamiento y garajes ---
    {
        UAlsasuaParkingSystem* Parking = World->GetGameInstance()->GetSubsystem<UAlsasuaParkingSystem>();
        if (Parking)
        {
            const int32 NumPlazas = Parking->GenerarPlazasAparcamiento();
            UE_LOG(LogTemp, Log, TEXT("DirectorArranque: %d plazas de aparcamiento."), NumPlazas);
        }
    }

    // --- 42. Marcas viales y cruces peatonales ---
    {
        UAlsasuaRoadMarkingsSystem* Marcas = World->GetGameInstance()->GetSubsystem<UAlsasuaRoadMarkingsSystem>();
        if (Marcas)
        {
            const int32 NumMarcas = Marcas->GenerarMarcas();
            UE_LOG(LogTemp, Log, TEXT("DirectorArranque: %d marcas viales."), NumMarcas);
        }
    }

    // --- 43. Arte callejero y grafis vascos ---
    {
        UAlsasuaStreetArtSystem* Arte = World->GetGameInstance()->GetSubsystem<UAlsasuaStreetArtSystem>();
        if (Arte)
        {
            const int32 NumArte = Arte->ColocarArteCallejero();
            UE_LOG(LogTemp, Log, TEXT("DirectorArranque: %d piezas de arte callejero."), NumArte);
        }
    }

    // --- 44. Contenedores de residuos por barrio ---
    {
        UAlsasuaContainerSystem* Contenedores = World->GetGameInstance()->GetSubsystem<UAlsasuaContainerSystem>();
        if (Contenedores)
        {
            const int32 NumContenedores = Contenedores->ColocarContenedores();
            UE_LOG(LogTemp, Log, TEXT("DirectorArranque: %d contenedores de residuos."), NumContenedores);
        }
    }

    // --- 45. Barandillas en puentes y zonas de riesgo ---
    {
        UAlsasuaGuardrailSystem* Barandillas = World->GetGameInstance()->GetSubsystem<UAlsasuaGuardrailSystem>();
        if (Barandillas)
        {
            const int32 NumBarandillas = Barandillas->ColocarBarandillas();
            UE_LOG(LogTemp, Log, TEXT("DirectorArranque: %d barandillas."), NumBarandillas);
        }
    }

    // --- 46. Semáforos en intersecciones ---
    {
        UE_LOG(LogTemp, Log, TEXT("DirectorArranque: skip semáforos para perfilado"));
    }

    // --- 47. Toldos y persianas en edificios ---
    {
        UAlsasuaAwningShutterSystem* Awnings = World->GetGameInstance()->GetSubsystem<UAlsasuaAwningShutterSystem>();
        if (Awnings)
        {
            const int32 NumAwnings = Awnings->ColocarToldosYPersianas();
            UE_LOG(LogTemp, Log, TEXT("DirectorArranque: %d toldos + persianas."), NumAwnings);
        }
    }

    // --- 48. Cubiertas: antenas, chimeneas, depósitos, placas solares ---
    {
        UAlsasuaRooftopDetailSystem* Rooftop = World->GetGameInstance()->GetSubsystem<UAlsasuaRooftopDetailSystem>();
        if (Rooftop)
        {
            const int32 NumRooftop = Rooftop->ColocarDetallesCubierta();
            UE_LOG(LogTemp, Log, TEXT("DirectorArranque: %d detalles de cubierta."), NumRooftop);
        }
    }

    // --- 49. Puertas y entradas por edificio ---
    {
        UAlsasuaDoorEntranceSystem* Doors = World->GetGameInstance()->GetSubsystem<UAlsasuaDoorEntranceSystem>();
        if (Doors)
        {
            const int32 NumDoors = Doors->ColocarPuertas();
            UE_LOG(LogTemp, Log, TEXT("DirectorArranque: %d puertas y entradas."), NumDoors);
        }
    }

    // --- 50. Cables aéreos y postes ---
    {
        UAlsasuaOverheadCableSystem* Cables = World->GetGameInstance()->GetSubsystem<UAlsasuaOverheadCableSystem>();
        if (Cables)
        {
            const int32 NumCables = Cables->ColocarCables();
            UE_LOG(LogTemp, Log, TEXT("DirectorArranque: %d cables aéreos."), NumCables);
        }
    }

    // --- 51. Rótulos de calle pintados en muros (bilingües) ---
    {
        UAlsasuaPaintedStreetSignSystem* PaintedSigns = World->GetGameInstance()->GetSubsystem<UAlsasuaPaintedStreetSignSystem>();
        if (PaintedSigns)
        {
            const int32 NumPainted = PaintedSigns->ColocarRotulosPintados();
            UE_LOG(LogTemp, Log, TEXT("DirectorArranque: %d rótulos pintados bilingües."), NumPainted);
        }
    }

    ArranqueMundo::Progreso = 1.f;
    ArranqueMundo::BaselineListo = true;
    ArranqueMundo::HayDirector = false;
    bConstruccionCompleta = true;
    UE_LOG(LogTemp, Log, TEXT("DirectorArranque: BaselineListo = true (51 sistemas basados en datos reales)."));
}

void ADirectorArranque::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bConstruccionCompleta) return;

    if (NPCSystemRef)
    {
        NPCSystemRef->ActualizarNPCs(DeltaTime);
    }

    if (TrafficSystemRef)
    {
        TrafficSystemRef->ActualizarTrafico(DeltaTime);
    }

    if (WeatherSystemRef && AudioSystemRef)
    {
        bool bRaining = WeatherSystemRef->IsRaining();
        bool bStorm = WeatherSystemRef->GetWeather() == EWeatherState::Storm;
        AudioSystemRef->SetWeatherState(bRaining, bStorm, false);
        AudioSystemRef->UpdateAmbientAudio(
            WeatherSystemRef->GameTimeHour,
            WeatherSystemRef->WindSpeed,
            WeatherSystemRef->RainIntensity,
            5000.0f);
    }
}
