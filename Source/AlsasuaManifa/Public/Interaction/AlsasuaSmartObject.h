#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AlsasuaSmartObject.generated.h"

UENUM(BlueprintType)
enum class ESmartObjectType : uint8
{
    Sit,        // Bancos en la plaza
    Lean,       // Vallas o paredes
    Guard,      // Puntos de patrilla estática
    Protest     // Puntos de pancarta o megáfono
};

UCLASS()
class ALSASUAMANIFA_API AAlsasuaSmartObject : public AActor
{
    GENERATED_BODY()

public:
    AAlsasuaSmartObject();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartObject")
    ESmartObjectType ObjectType;

    // Posición exacta donde debe colocarse el NPC
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SmartObject")
    class USceneComponent* InteractionSlot;

    // ¿Está este sitio ocupado por un NPC?
    UPROPERTY(BlueprintReadOnly, Category = "SmartObject")
    bool bIsOccupied = false;

    // Animación específica que debe reproducir el NPC aquí
    UPROPERTY(EditAnywhere, Category = "SmartObject")
    UAnimSequence* InteractionAnim;

    bool CanBeUsed() const { return !bIsOccupied; }
    void SetOccupied(bool bState) { bIsOccupied = bState; }
};
