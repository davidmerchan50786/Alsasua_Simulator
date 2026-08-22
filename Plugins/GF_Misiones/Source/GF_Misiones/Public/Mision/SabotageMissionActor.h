#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SabotageMissionActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMissionCompleted, float, PopularSupportReward);

UCLASS()
class GF_MISIONES_API ASabotageMissionActor : public AActor
{
    GENERATED_BODY()

public:
    ASabotageMissionActor();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Mision")
    FName TargetNodeId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Mision")
    int32 EnemyCount = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Mision")
    float PopularSupportReward = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Mision")
    TSubclassOf<AActor> EnemyClass;

    UFUNCTION(BlueprintCallable, Category="AAA|Mision")
    void StartMission();

    UFUNCTION(BlueprintCallable, Category="AAA|Mision")
    void OnTargetSabotaged(FName NodeId);

    UFUNCTION(BlueprintImplementableEvent, Category="AAA|Mision")
    void OnMissionSuccess();

    UPROPERTY(BlueprintAssignable, Category="AAA|Mision")
    FOnMissionCompleted OnMissionCompleted;

protected:
    virtual void BeginPlay() override;

private:
    void SpawnEnemies();
};
