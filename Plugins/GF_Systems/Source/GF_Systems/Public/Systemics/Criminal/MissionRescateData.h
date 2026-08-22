#pragma once
#include "CoreMinimal.h"
#include "Mision/MisionData.h"
#include "MissionRescateData.generated.h"

USTRUCT(BlueprintType)
struct FMisionRescate : public FMissionData {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    class AHostageCharacter* TargetHostage = nullptr;

    UPROPERTY(BlueprintReadWrite)
    float RansomAmount = 5000.f;
};
