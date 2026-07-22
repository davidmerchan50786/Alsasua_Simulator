#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AlsasuaChatManager.generated.h"

UCLASS()
class ALSASUASIMULATOR_API UAlsasuaChatManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    void RequestNPCResponse(AActor* NPC, const FString& Prompt);
};
