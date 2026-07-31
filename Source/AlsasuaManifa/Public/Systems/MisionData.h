#pragma once
#include "CoreMinimal.h"
#include "MisionData.generated.h"

USTRUCT(BlueprintType)
struct ALSASUAMANIFA_API FMisionObjective {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere) FName ObjectiveID;
    UPROPERTY(BlueprintReadWrite, EditAnywhere) FText ObjectiveText;
    UPROPERTY(BlueprintReadOnly) bool bCompleted = false;
};

USTRUCT(BlueprintType)
struct ALSASUAMANIFA_API FMissionReward {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere) float Money = 0.f;
    UPROPERTY(BlueprintReadWrite, EditAnywhere) float PopularSupport = 0.f;
    UPROPERTY(BlueprintReadWrite, EditAnywhere) float WantedLevelDelta = 0.f;
    UPROPERTY(BlueprintReadWrite, EditAnywhere) FName UnlockNodeID;
};

USTRUCT(BlueprintType)
struct ALSASUAMANIFA_API FMissionData {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere) FName MissionID;
    UPROPERTY(BlueprintReadWrite, EditAnywhere) FText MissionName;
    UPROPERTY(BlueprintReadWrite, EditAnywhere) FText Description;
    UPROPERTY(BlueprintReadWrite, EditAnywhere) TArray<FMisionObjective> Objectives;
    UPROPERTY(BlueprintReadWrite, EditAnywhere) FName RequiredFaction;
    UPROPERTY(BlueprintReadWrite, EditAnywhere) int32 DifficultyLevel = 1;
    UPROPERTY(BlueprintReadWrite, EditAnywhere) FMissionReward Reward;
    UPROPERTY(BlueprintReadWrite, EditAnywhere) TArray<FName> RequiredMissions;
    UPROPERTY(BlueprintReadWrite, EditAnywhere) float TimeLimitSeconds = 0.f;
};
