#include "World/AlsasuaTerrainLayersSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

void UAlsasuaTerrainLayersSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CrearLayers();
}

void UAlsasuaTerrainLayersSystem::CrearLayers()
{
    Layers.Empty();

    // Los ocho barrios de nighborhoods.json, con el firme que se ve en cada uno.
    // Los materiales los crea UCreadorMaterialesSimples; sin ellos, quien los
    // pida se queda con el material de su malla y no se rompe nada.
    Layers.Add({ TEXT("Herriko"),     TEXT("/Game/Materiales/Pavimentos/M_Piedra_Herriko"),      1500.0f, -10.0f, true  });
    Layers.Add({ TEXT("Zelai"),       TEXT("/Game/Materiales/Pavimentos/M_Asphalt_Zelai"),       2000.0f,   0.0f, false });
    Layers.Add({ TEXT("Intxostia"),   TEXT("/Game/Materiales/Pavimentos/M_Asphalt_Intxostia"),   2500.0f,   0.0f, false });
    Layers.Add({ TEXT("SanPedro"),    TEXT("/Game/Materiales/Pavimentos/M_Asphalt_SanPedro"),    2000.0f,   0.0f, false });
    Layers.Add({ TEXT("Errota"),      TEXT("/Game/Materiales/Pavimentos/M_Gravel_Errota"),       1500.0f,  -5.0f, false });
    Layers.Add({ TEXT("Harrobieta"),  TEXT("/Game/Materiales/Pavimentos/M_Piedra_Harrobieta"),   1500.0f, -10.0f, true  });
    Layers.Add({ TEXT("Ferroviario"), TEXT("/Game/Materiales/Pavimentos/M_Asphalt_Ferroviario"), 2000.0f,   0.0f, false });
    // La ruta era /Game/Materiales/Naturaleza/M_Tierra_Monte y el generador lo
    // crea en Pavimentos: la carpeta existe, el asset no, y quien lo pide
    // comprueba el null y sigue, así que Monte se quedaba sin firme en silencio.
    Layers.Add({ TEXT("Monte"),       TEXT("/Game/Materiales/Pavimentos/M_Tierra_Monte"),        3000.0f, -15.0f, false });
}

FString UAlsasuaTerrainLayersSystem::MaterialDeBarrio(const FString& Barrio) const
{
    for (const FBarrioTerrainLayer& L : Layers)
    {
        if (L.Barrio == Barrio) return L.MaterialPath;
    }
    return FString();
}

bool UAlsasuaTerrainLayersSystem::BarrioEmpedrado(const FString& Barrio) const
{
    for (const FBarrioTerrainLayer& L : Layers)
    {
        if (L.Barrio == Barrio) return L.bEmpedrado;
    }
    return false;
}

int32 UAlsasuaTerrainLayersSystem::PublicarFirmePorBarrio()
{
    // No se construye nada a propósito. Ver la cabecera: esto ponía nueve planos
    // opacos de un kilómetro, ocho de ellos apilados en el origen del mundo.
    int32 Empedrados = 0;
    for (const FBarrioTerrainLayer& L : Layers) if (L.bEmpedrado) ++Empedrados;

    UE_LOG(LogTemp, Log,
        TEXT("TerrainLayers: firme publicado para %d barrios (%d empedrados). No crea geometría: lo consume quien pavimenta."),
        Layers.Num(), Empedrados);
    return Layers.Num();
}
