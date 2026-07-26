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

    UFUNCTION(BlueprintCallable, Category = "AAA|Physics")
    void ApplySysteimcDamage(float DamageAmount);

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

private:
    void CheckCrowdPressure();
};
