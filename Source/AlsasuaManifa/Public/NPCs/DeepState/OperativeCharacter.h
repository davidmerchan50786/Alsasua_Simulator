#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "OperativeCharacter.generated.h"

/**
 * NPC operativo de Deep State.
 * Puede estar disfrazado y ejecutar emboscadas coordinadas.
 */
UCLASS()
class ALSASUAMANIFA_API AOperativeCharacter : public ACharacter {
    GENERATED_BODY()
public:
    AOperativeCharacter();

    /** ¿Está actualmente disfrazado? */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeepState")
    bool bIsDisguised = true;

    /** Radio de la emboscada (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeepState|Ambush", meta = (ClampMin = "200"))
    float AmbushRadius = 1500.f;

    /** Daño de la emboscada por segundo. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeepState|Ambush", meta = (ClampMin = "1"))
    float AmbushDamagePerSecond = 25.f;

    /** Fuerza del impulso al revelarse. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeepState|Ambush")
    float RevealImpulseForce = 800.f;

    /**
     * Ejecuta la emboscada: se revela, ataca objetivos cercanos,
     * y alerta a guardias.
     */
    UFUNCTION(BlueprintCallable, Category = "DeepState")
    void ExecuteAmbush();

private:
    bool bAmbushActive = false;
    float AmbushTimer = 0.f;
    float AmbushDuration = 5.f;

    void RevealOperative();
    void DamageNearbyTargets();
    void AlertNearbyGuards();

    virtual void Tick(float DeltaTime) override;
};
