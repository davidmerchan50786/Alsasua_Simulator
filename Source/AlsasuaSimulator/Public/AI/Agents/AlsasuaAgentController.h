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

    UFUNCTION(BlueprintCallable, Category="Alsasua|AI")
    void HandleNPCResponse(const FString& ResponseText);

protected:
    virtual void BeginPlay() override;

private:
    UFUNCTION()
    void OnLLMResponse(AActor* NPC, const FString& ResponseText);
};
