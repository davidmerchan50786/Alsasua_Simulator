#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Systems/MisionData.h"
#include "MisionManager.generated.h"

UCLASS()
class ALSASUAMANIFA_API UMisionManager : public UObject {
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "AAA|Mission")
    void StartMission(const FMisionData& MissionData);
};

