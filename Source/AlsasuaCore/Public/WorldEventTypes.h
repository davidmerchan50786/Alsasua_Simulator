#pragma once

#include "CoreMinimal.h"
#include "WorldEventTypes.generated.h"

USTRUCT(BlueprintType)
struct FWorldEventDataV2 {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName EventID;

    UPROPERTY(BlueprintReadOnly)
    FText EventAnnounceMessage;
};
