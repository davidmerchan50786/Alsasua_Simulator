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
    FVector Origin = FVector::ZeroVector;

    UPROPERTY()
    float Radius = 0.f;

    UPROPERTY()
    float Strength = 0.f;

    UPROPERTY()
    float TimeStamp = 0.f;
};

UCLASS()
class ALSASUAMANIFA_API UAlsasuaChainReactionSubsystem : public UWorldSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual bool IsAllowedToTick() const override { return true; }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(AlsasuaChainReactionSubsystem, STATGROUP_Game); }
    virtual bool IsTickable() const override { return !IsTemplate(); }   // no ticar el CDO (CLAUDE.md §9)

    UFUNCTION(BlueprintCallable, Category = "AAA|Chain")
    void EmitChainEvent(FVector Origin, float Radius, float Strength);

private:
    TArray<FChainEvent> ActiveEvents;
    void PropagateEvent(const FChainEvent& Event);
};
