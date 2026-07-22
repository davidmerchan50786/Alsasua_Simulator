#pragma once

#include "CoreMinimal.h"
#include "EvidenceData.generated.h"

USTRUCT(BlueprintType)
struct FEvidenceItem {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName EvidenceId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Title;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Reliability = 1.0f; // 0.0 - 1.0
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float ImpactPower = 10.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bIsClassified = true;
};
