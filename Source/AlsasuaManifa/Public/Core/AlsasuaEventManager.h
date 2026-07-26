#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
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
    float TimeStamp; // Segundos desde el inicio de la misión

    UPROPERTY(BlueprintReadOnly)
    bool bHasTriggered = false;
};

UCLASS()
class ALSASUAMANIFA_API UAlsasuaEventManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;

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
