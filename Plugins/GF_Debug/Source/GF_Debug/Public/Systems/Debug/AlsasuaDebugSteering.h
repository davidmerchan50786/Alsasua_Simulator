#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AlsasuaDebugSteering.generated.h"

UCLASS()
class GF_DEBUG_API UAlsasuaDebugSteering : public UWorldSubsystem {
    GENERATED_BODY()
public:
    // Comandos de consola (Exec)

    // Detona todos los explosivos del mapa
    UFUNCTION(Exec, Category="AAA|Debug")
    void Alsasua_DetonateAll();

    // Limpia todos los fuegos activos
    UFUNCTION(Exec, Category="AAA|Debug")
    void Alsasua_ClearAllFire();

    // Fuerza una cifra de PopularSupport (0-100)
    UFUNCTION(Exec, Category="AAA|Debug")
    void Alsasua_SetSupport(float NewValue);

    // Muestra/Oculta los radios de explosión y anclajes de vehículos
    UFUNCTION(Exec, Category="AAA|Debug")
    void Alsasua_ToggleDebugVisuals();

    bool bShowDebugVisuals = false;
};