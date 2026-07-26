#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EventBusAlsasua.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAlsasuaEvent, FName, EventName, UObject*, Payload);

UCLASS()
class ALSASUACORE_API UEventBusAlsasua : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category = "Alsasua|Core")
    void BroadcastEvent(FName EventName, UObject* Payload = nullptr);

    UPROPERTY(BlueprintAssignable, Category = "Alsasua|Core")
    FOnAlsasuaEvent OnEvent;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Core")
    void ListenEvent(FName EventName, UObject* Listener);

    void BroadcastEventNative(FName EventName, UObject* Payload = nullptr);

private:
    TMap<FName, TArray<TWeakObjectPtr<UObject>>> Listeners;
};
