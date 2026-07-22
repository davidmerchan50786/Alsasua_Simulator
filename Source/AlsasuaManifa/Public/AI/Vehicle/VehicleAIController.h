#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "VehicleAIController.generated.h"

UCLASS()
class ALSASUAMANIFA_API AVehicleAIController : public AAIController
{
    GENERATED_BODY()

public:
    AVehicleAIController();

    virtual void OnPossess(APawn* InPawn) override;

    UFUNCTION(BlueprintCallable, Category="AI|Vehicle")
    void StartPursuit(APawn* TargetPawn);

    UFUNCTION(BlueprintCallable, Category="AI|Vehicle")
    void StopPursuit();

protected:
    virtual void Tick(float DeltaTime) override;

private:
    APawn* PursuitTarget = nullptr;
    float PursuitAggression = 1.0f; // 0..1
};