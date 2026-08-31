#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaCrowdSoundComponent.generated.h"

/**
 * Emits sound/shouting events from a crowd location.
 * Attracts nearby NPCs (join probability) and police (investigate).
 * Visual: expanding Niagara rings. Audio: shout sound loop.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GF_NPCS_API UAlsasuaCrowdSoundComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAlsasuaCrowdSoundComponent();

    /** Activate sound emission */
    UFUNCTION(BlueprintCallable, Category = "CrowdSound")
    void Activar(float Intensity = 1.0f);

    /** Deactivate sound */
    UFUNCTION(BlueprintCallable, Category = "CrowdSound")
    void Desactivar();

    /** Current intensity (0-1) */
    UPROPERTY(BlueprintReadOnly, Category = "CrowdSound")
    float Intensity = 0.f;

    /** Sound radius in cm */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CrowdSound")
    float Radio = 5000.f;

    /** How often to emit visual rings (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CrowdSound")
    float FrecuenciaAnillos = 2.0f;

    /** Attracts police to investigate */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CrowdSound")
    bool bAlertarPolicia = true;

    /** Police investigation radius (larger than NPC join radius) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CrowdSound")
    float RadioPolicia = 8000.f;

protected:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    bool bActivo = false;
    float TimerAnillos = 0.f;

    void EmitirAnilloVisual();
};
