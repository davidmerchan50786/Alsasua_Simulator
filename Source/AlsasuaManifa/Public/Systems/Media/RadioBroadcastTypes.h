#pragma once
#include "CoreMinimal.h"
#include "RadioBroadcastTypes.generated.h"

USTRUCT(BlueprintType)
struct FRadioNewsClip {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText NewsHeadline;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText NewsBody;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<USoundBase> VoiceClip;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RequiredPopularSupport = 0.f;
};