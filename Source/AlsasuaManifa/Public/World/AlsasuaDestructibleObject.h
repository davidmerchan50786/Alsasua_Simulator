#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AlsasuaDestructibleObject.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnObjectDestroyed, AAlsasuaDestructibleObject*, DestroyedObject);

/** Objeto que reacciona físicamente a la tensión de la multitud */
UCLASS()
class ALSASUAMANIFA_API AAlsasuaDestructibleObject : public AActor
{
    GENERATED_BODY()

public:
    AAlsasuaDestructibleObject();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA|Physics")
    float Integrity = 100.0f;

    UPROPERTY(EditAnywhere, Category = "AAA|Physics")
    bool bBreakOnCrowdTension = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA|Physics")
    class UPrimitiveComponent* GeometryComponent;

    UPROPERTY(BlueprintAssignable, Category = "AAA|Physics")
    FOnObjectDestroyed OnObjectDestroyed;

    /** Integridad que se pierde por segundo mientras la multitud está hostil.
     *  Antes era un 0.5 fijo por Tick, o sea que el aguante de la pieza dependía
     *  de los FPS de la máquina. */
    UPROPERTY(EditAnywhere, Category = "AAA|Physics")
    float DesgastePorSegundo = 30.0f;

    /** Se llamaba ApplySysteimcDamage. Nadie lo llamaba, así que renombrarlo no
     *  rompe ningún Blueprint. */
    UFUNCTION(BlueprintCallable, Category = "AAA|Physics")
    void AplicarDanoSistemico(float DamageAmount);

    UFUNCTION(BlueprintPure, Category = "AAA|Physics")
    bool EstaDestruido() const { return bDestruido; }

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

private:
    void CheckCrowdPressure(float DeltaTime);

    /** Una vez roto, roto: sin esto el Tick seguía pasando por Integrity <= 0
     *  y reemitía OnObjectDestroyed en cada frame. */
    bool bDestruido = false;
};
