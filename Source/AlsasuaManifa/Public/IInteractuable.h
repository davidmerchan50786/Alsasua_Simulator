// IInteractuable.h
// ═══════════════════════════════════════════════════════════════════════════
//  DEPRECATED: Usar IInteractableInterface.h en su lugar.
//  Esta interfaz se mantiene por retrocompatibilidad con Blueprints existentes.
// ═══════════════════════════════════════════════════════════════════════════

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IInteractuable.generated.h"

class AAlsasuaCharacter;

// Declaración de la UInterface (parte requerida por el sistema de reflexión).
UINTERFACE(MinimalAPI, BlueprintType)
class UInteractuable : public UInterface
{
    GENERATED_BODY()
};

/**
 * Interfaz nativa/Blueprint para objetos interactuables.
 * Implementar en C++ (override de Interactuar_Implementation) o en Blueprint.
 */
class ALSASUAMANIFA_API IInteractuable
{
    GENERATED_BODY()

public:
    /**
     * Se llama cuando el jugador interactúa con este objeto.
     * @param Jugador  Personaje que inicia la interacción.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaccion")
    void Interactuar(AAlsasuaCharacter* Jugador);
};
