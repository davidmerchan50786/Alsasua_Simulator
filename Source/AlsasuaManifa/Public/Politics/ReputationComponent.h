#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ReputationComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnReputationChanged, FName, FactionId, float, NewValue);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALSASUAMANIFA_API UReputationComponent : public UActorComponent
{
    GENERATED_BODY()

public:    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Alsasua|Reputation")
    TMap<FName, float> Reputation;

    UFUNCTION(BlueprintCallable, Category="Alsasua|Reputation")
    void ModifyReputation(FName FactionId, float Delta);

    UPROPERTY(BlueprintAssignable)
    FOnReputationChanged OnReputationChanged;
};
