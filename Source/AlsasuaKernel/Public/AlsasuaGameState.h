#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "AlsasuaGameState.generated.h"

UENUM(BlueprintType)
enum class ETimeOfDay : uint8
{
    Dawn,
    Morning,
    Noon,
    Afternoon,
    Evening,
    Night
};

UCLASS()
class ALSASUAKERNEL_API AAlsasuaGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    AAlsasuaGameState();

    UFUNCTION(BlueprintCallable, Category = "Alsasua|GameState")
    float GetTimeOfDay() const { return TimeOfDay; }

    UFUNCTION(BlueprintCallable, Category = "Alsasua|GameState")
    ETimeOfDay GetCurrentPeriod() const;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|GameState")
    float GetCrowdTension() const { return CrowdTension; }

    UFUNCTION(BlueprintCallable, Category = "Alsasua|GameState")
    void SetCrowdTension(float NewTension);

    UFUNCTION(BlueprintCallable, Category = "Alsasua|GameState")
    void SetTimeOfDay(float NewTime);

protected:
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Alsasua|GameState")
    float TimeOfDay = 12.0f;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Alsasua|GameState")
    float CrowdTension = 0.0f;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
