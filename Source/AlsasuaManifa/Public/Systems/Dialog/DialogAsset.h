#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DialogAsset.generated.h"

UCLASS(BlueprintType)
class ALSASUAMANIFA_API UDialogAsset : public UDataAsset {
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialog")
    FText DialogText;
};
