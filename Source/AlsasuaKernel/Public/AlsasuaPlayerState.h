#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AlsasuaPlayerState.generated.h"

UCLASS()
class ALSASUAKERNEL_API AAlsasuaPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    AAlsasuaPlayerState();

    UFUNCTION(BlueprintCallable, Category = "Alsasua|PlayerState")
    float GetReputation() const { return Reputation; }

    UFUNCTION(BlueprintCallable, Category = "Alsasua|PlayerState")
    void AddReputation(float Delta);

    UFUNCTION(BlueprintCallable, Category = "Alsasua|PlayerState")
    int32 GetCompletedMissions() const { return CompletedMissions; }

    UFUNCTION(BlueprintCallable, Category = "Alsasua|PlayerState")
    void IncrementCompletedMissions();

protected:
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Alsasua|PlayerState")
    float Reputation = 50.0f;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Alsasua|PlayerState")
    int32 CompletedMissions = 0;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
