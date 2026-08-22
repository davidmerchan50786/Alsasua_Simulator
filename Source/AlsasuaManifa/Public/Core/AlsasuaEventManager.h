#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "AlsasuaEventManager.generated.h"

// Definición de tipos de hitos narrativos
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNarrativeEventTriggered, FName, EventID);

USTRUCT(BlueprintType)
struct FNarrativeMilestone
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName EventIdentifier;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TimeStamp = 0.f;

    UPROPERTY(BlueprintReadOnly)
    bool bHasTriggered = false;
};

UCLASS()
class ALSASUAMANIFA_API UAlsasuaEventManager : public UWorldSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual bool IsAllowedToTick() const override { return true; }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(AlsasuaEventManager, STATGROUP_Game); }
    virtual bool IsTickable() const override { return !IsTemplate(); }   // no ticar el CDO (CLAUDE.md §9)

    // Registra un nuevo evento en el timeline
    UFUNCTION(BlueprintCallable, Category = "AAA|Narrative")
    void AddMilestone(FName ID, float Time);

    // Delegado para informar a otros sistemas (Audio, IA, UI)
    UPROPERTY(BlueprintAssignable, Category = "AAA|Narrative")
    FOnNarrativeEventTriggered OnEventTriggered;

    float GetCurrentMissionTime() const { return MissionTimer; }

private:
    float MissionTimer = 0.0f;
    TArray<FNarrativeMilestone> Timeline;

    void ProcessTimeline();
};
