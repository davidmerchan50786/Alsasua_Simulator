#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FirePropagationComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GF_SYSTEMS_API UFirePropagationComponent : public UActorComponent {
    GENERATED_BODY()
public:
    UFirePropagationComponent();

    UPROPERTY(EditAnywhere, Category="Fire")
    float SpreadRadius = 300.f;

    UPROPERTY(EditAnywhere, Category="Fire")
    float DamagePerSecond = 5.f;

    UFUNCTION(BlueprintCallable, Category="Fire")
    void StartBurning(float Duration);

protected:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    float RemainingBurnTime = 0.f;
    bool bIsBurning = false;
    float SpreadTimer = 0.f;
};