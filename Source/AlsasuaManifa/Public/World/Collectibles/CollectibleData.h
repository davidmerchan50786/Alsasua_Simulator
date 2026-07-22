#pragma once
#include "CoreMinimal.h"
#include "CollectibleData.generated.h"

USTRUCT(BlueprintType)
struct FHistoricalCollectible {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Id;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Title;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText LoreDescription;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) class UTexture2D* HistoricalImage;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Year;
};