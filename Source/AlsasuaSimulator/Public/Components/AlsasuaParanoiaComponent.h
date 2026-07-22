#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaParanoiaComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnParanoiaLevelChanged, float, NewLevel);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALSASUASIMULATOR_API UAlsasuaParanoiaComponent : public UActorComponent {
    GENERATED_BODY()
public:
    UAlsasuaParanoiaComponent();
    UPROPERTY(BlueprintAssignable, Category = "Alsasua|Paranoia")
    FOnParanoiaLevelChanged OnParanoiaChanged;
    UFUNCTION(BlueprintCallable, Category = "Alsasua|Paranoia")
    void AddStress(float Amount);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Paranoia")
    float DecayRate = 0.5f;
protected:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    float CurrentParanoia = 0.0f;
};
