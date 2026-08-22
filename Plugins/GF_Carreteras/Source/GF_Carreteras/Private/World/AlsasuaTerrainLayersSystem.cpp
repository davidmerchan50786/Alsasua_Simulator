#include "World/AlsasuaTerrainLayersSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "GeoDataAlsasua.h"

void UAlsasuaTerrainLayersSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CrearLayers();
}

void UAlsasuaTerrainLayersSystem::CrearLayers()
{
    Layers.Empty();

    Layers.Add(FBarrioTerrainLayer{TEXT("Herriko"), TEXT("/Game/Materiales/Pavimentos/M_Piedra_Herriko"), 1500.0f, -10.0f});
    Layers.Add(FBarrioTerrainLayer{TEXT("Zelai"), TEXT("/Game/Materiales/Pavimentos/M_Asphalt_Zelai"), 2000.0f, 0.0f});
    Layers.Add(FBarrioTerrainLayer{TEXT("Intxostia"), TEXT("/Game/Materiales/Pavimentos/M_Asphalt_Intxostia"), 2500.0f, 0.0f});
    Layers.Add(FBarrioTerrainLayer{TEXT("SanPedro"), TEXT("/Game/Materiales/Pavimentos/M_Asphalt_SanPedro"), 2000.0f, 0.0f});
    Layers.Add(FBarrioTerrainLayer{TEXT("Errota"), TEXT("/Game/Materiales/Pavimentos/M_Gravel_Errota"), 1500.0f, -5.0f});
    Layers.Add(FBarrioTerrainLayer{TEXT("Harrobieta"), TEXT("/Game/Materiales/Pavimentos/M_Piedra_Harrobieta"), 1500.0f, -10.0f});
    Layers.Add(FBarrioTerrainLayer{TEXT("Ferroviario"), TEXT("/Game/Materiales/Pavimentos/M_Asphalt_Ferroviario"), 2000.0f, 0.0f});
    Layers.Add(FBarrioTerrainLayer{TEXT("Monte"), TEXT("/Game/Materiales/Naturaleza/M_Tierra_Monte"), 3000.0f, -15.0f});
}

int32 UAlsasuaTerrainLayersSystem::PublicarFirmePorBarrio()
{
    return Layers.Num();
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
