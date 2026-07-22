#pragma once
#include "CoreMinimal.h"
#include "MisionData.generated.h"

USTRUCT(BlueprintType)
struct FMisionData {
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FName MissionID;
};

