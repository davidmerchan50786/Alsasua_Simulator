#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Systems/MisionData.h"
#include "MisionManager.generated.h"

USTRUCT(BlueprintType)
struct FMisionActiva
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FName MissionID;
    UPROPERTY(BlueprintReadOnly) float TiempoInicio = 0.f;
    UPROPERTY(BlueprintReadOnly) bool bCompletada = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMissionStateChanged, const FMisionActiva&, Mission);

UCLASS()
class ALSASUAMANIFA_API UMisionManager : public UObject {
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "AAA|Mission")
    void StartMission(const FMisionData& MissionData);

    UFUNCTION(BlueprintCallable, Category = "AAA|Mission")
    void CompleteMission(FName MissionID);

    UFUNCTION(BlueprintCallable, Category = "AAA|Mission")
    void FailMission(FName MissionID);

    UFUNCTION(BlueprintPure, Category = "AAA|Mission")
    bool IsMissionActive(FName MissionID) const;

    UFUNCTION(BlueprintPure, Category = "AAA|Mission")
    const TArray<FMisionActiva>& GetActiveMissions() const { return ActiveMissions; }

    UPROPERTY(BlueprintAssignable, Category = "AAA|Mission")
    FOnMissionStateChanged OnMissionStateChanged;

private:
    UPROPERTY()
    TArray<FMisionActiva> ActiveMissions;
};
