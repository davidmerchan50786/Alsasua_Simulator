#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AlsasuaDestructibleObject.generated.h"

/** Objeto que reacciona físicamente a la tensión de la multitud */
UCLASS()
class ALSASUAMANIFA_API AAlsasuaDestructibleObject : public AActor
{
    GENERATED_BODY()

public:
    AAlsasuaDestructibleObject();

    // Salud del objeto (vallas, papeleras, etc.)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA|Physics")
    float Integrity = 100.0f;

    // ¿Se rompe por la presión de la masa?
    UPROPERTY(EditAnywhere, Category = "AAA|Physics")
    bool bBreakOnCrowdTension = true;

    // Componente de geometría de Chaos para la fractura
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA|Physics")
    class UPrimitiveComponent* GeometryComponent;

    UFUNCTION(BlueprintCallable, Category = "AAA|Physics")
    void ApplySysteimcDamage(float DamageAmount);

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime);

private:
    void CheckCrowdPressure();
};
