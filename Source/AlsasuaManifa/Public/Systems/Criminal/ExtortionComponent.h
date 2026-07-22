#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ExtortionComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALSASUAMANIFA_API UExtortionComponent : public UActorComponent {
    GENERATED_BODY()
public:
    UExtortionComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Criminal")
    float RequiredPayment = 1000.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Criminal")
    float DueFrequencySeconds = 300.f; // 5 minutos entre cobros

    UFUNCTION(BlueprintCallable, Category="AAA|Criminal")
    void ProcessPayment(float Amount);

protected:
    virtual void BeginPlay() override;
    void OnPaymentOverdue();

private:
    float LastPaymentTime = 0.f;
    FTimerHandle TimerHandle_Due;
};