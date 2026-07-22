#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EventBusAlsasua.generated.h"

UCLASS()
class ALSASUACORE_API UEventBusAlsasua : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category = "Alsasua|Core")
    void BroadcastEvent(FName EventName, UObject* Payload = nullptr);
};
