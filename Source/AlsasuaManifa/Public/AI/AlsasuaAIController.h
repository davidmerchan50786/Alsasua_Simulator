#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AlsasuaAIController.generated.h"

UCLASS()
class ALSASUAMANIFA_API AAlsasuaAIController : public AAIController
{
    GENERATED_BODY()

public:
    AAlsasuaAIController();

    UFUNCTION(BlueprintCallable, Category = "AAA|AI")
    void HandleNoiseEvent(FVector NoiseLocation, float Intensity);

    UFUNCTION(BlueprintCallable, Category = "AAA|AI")
    void OnPerceptionUpdated(const TArray<AActor*>& UpdatedActors);

    UPROPERTY(BlueprintReadWrite, Category = "AAA|AI")
    int32 CurrentAIState = 0;

    UPROPERTY(BlueprintReadWrite, Category = "AAA|AI")
    FVector LastNoiseLocation = FVector::ZeroVector;
};

