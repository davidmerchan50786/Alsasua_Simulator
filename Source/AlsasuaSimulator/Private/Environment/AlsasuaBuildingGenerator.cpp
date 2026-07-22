#include "Environment/AlsasuaBuildingGenerator.h"

AAlsasuaBuildingGenerator::AAlsasuaBuildingGenerator() {
    BuildingInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("BuildingInstances"));
    BuildingInstances->SetMobility(EComponentMobility::Static);
    // Habilitar Nanite en el mesh asignado a este componente para calidad máxima
}

void AAlsasuaBuildingGenerator::SpawnBuildingsFromData(FString JsonPath) {
    UE_LOG(LogTemp, Log, TEXT("[AlsasuaEnv] Cargando polígonos de edificios desde %s"), *JsonPath);
}
