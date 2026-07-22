#pragma once
#include "CoreMinimal.h"
#include "AIController.h"
#include "AlsasuaAgentController.generated.h"

UCLASS()
class ALSASUASIMULATOR_API AAlsasuaAgentController : public AAIController
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="Alsasua|AI")
    void OnHearPlayerSpeech(const FString& SpeechText);
};
