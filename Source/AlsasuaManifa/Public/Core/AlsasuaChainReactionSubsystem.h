#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "AlsasuaChainReactionSubsystem.generated.h"

USTRUCT()
struct FChainEvent
{
    GENERATED_BODY()

    UPROPERTY()
    FVector Origin;

    UPROPERTY()
    float Radius;

    UPROPERTY()
    float Strength;

    UPROPERTY()
    float TimeStamp;
};

UCLASS()
class ALSASUAMANIFA_API UAlsasuaChainReactionSubsystem : public UWorldSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual bool IsAllowedToTick() const { return true; }
    virtual TStatId GetStatId() const { return TStatId(); }

    UFUNCTION(BlueprintCallable, Category = "AAA|Chain")
    void EmitChainEvent(FVector Origin, float Radius, float Strength);

private:
    TArray<FChainEvent> ActiveEvents;
    void PropagateEvent(const FChainEvent& Event);
};
