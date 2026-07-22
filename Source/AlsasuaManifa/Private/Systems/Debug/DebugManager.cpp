#include "Systems/Debug/DebugManager.h"
#include "EngineUtils.h"
#include "Vehicle/BaseVehicle.h"
#include "AlsasuaCharacter.h"

void UDebugManager::OnWorldBeginPlay(UWorld& InWorld) {
    UE_LOG(LogTemp, Warning, TEXT("Debug Manager Activado: usa 'DumpGameState' en consola."));
}

void UDebugManager::DumpGameState() {
    int32 VehCount = 0;
    for (TActorIterator<ABaseVehicle> It(GetWorld()); It; ++It) VehCount++;

    int32 CharCount = 0;
    for (TActorIterator<AAlsasuaCharacter> It(GetWorld()); It; ++It) CharCount++;

    UE_LOG(LogTemp, Warning, TEXT("--- Estatus Altsasu Manifa ---"));
    UE_LOG(LogTemp, Warning, TEXT("Vehiculos Activos: %d"), VehCount);
    UE_LOG(LogTemp, Warning, TEXT("Personajes Activos: %d"), CharCount);
    UE_LOG(LogTemp, Warning, TEXT("-----------------------------"));
}