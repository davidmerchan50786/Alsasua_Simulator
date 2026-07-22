#include "Environment/AlsasuaRoadManager.h"

AAlsasuaRoadManager::AAlsasuaRoadManager() {
    RoadSpline = CreateDefaultSubobject<USplineComponent>(TEXT("RoadSpline"));
    RootComponent = RoadSpline;
}

void AAlsasuaRoadManager::GenerateRoadMesh() {
    // Aquí se implementará la generación de mallas procedimentales 
    // que se fusionan con el RVT del terreno.
    UE_LOG(LogTemp, Log, TEXT("[AlsasuaEnv] Generando malla de carretera AAA..."));
}
