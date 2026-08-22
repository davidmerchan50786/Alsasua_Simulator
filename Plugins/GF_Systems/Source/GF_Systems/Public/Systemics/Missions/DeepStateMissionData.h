#pragma once
#include "CoreMinimal.h"
#include "Mision/MisionData.h"
#include "UObject/NoExportTypes.h"
#include "DeepStateMissionData.generated.h"

USTRUCT(BlueprintType)
struct FMisionCloacas : public FMissionData {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FName TargetOperationID;

    UPROPERTY(BlueprintReadWrite)
    bool bRequiresStealth = true;

    UPROPERTY(BlueprintReadWrite)
    FText SuccessRadioHeadline_ES;

    UPROPERTY(BlueprintReadWrite)
    FText SuccessRadioHeadline_EU;
};

UCLASS()
class GF_SYSTEMS_API UDeepStateMissionLogic : public UObject {
    GENERATED_BODY()
public:
    static void ExecuteExposeLeak(UWorld* World, const FMisionCloacas& Mission);
};