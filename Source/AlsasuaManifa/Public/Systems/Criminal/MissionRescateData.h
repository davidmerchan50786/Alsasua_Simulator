#pragma once
#include "CoreMinimal.h"
#include "Systems/MisionData.h"
#include "MissionRescateData.generated.h"

USTRUCT(BlueprintType)
struct FMisionRescate : public FMisionData {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    class AHostageCharacter* TargetHostage;

    UPROPERTY(BlueprintReadWrite)
    float RansomAmount = 5000.f;
};
