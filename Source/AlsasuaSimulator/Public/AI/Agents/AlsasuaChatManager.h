#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AlsasuaChatManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNPCResponseReceived, AActor*, NPC, const FString&, ResponseText);

UCLASS()
class ALSASUASIMULATOR_API UAlsasuaChatManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    void RequestNPCResponse(AActor* NPC, const FString& Prompt);

    UPROPERTY(BlueprintAssignable, Category="Alsasua|AI")
    FOnNPCResponseReceived OnResponseReceived;
};
