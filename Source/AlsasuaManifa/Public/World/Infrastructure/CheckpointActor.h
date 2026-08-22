#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Politics/StateOfAlarmSubsystem.h"
#include "CheckpointActor.generated.h"

UCLASS()
class ALSASUAMANIFA_API ACheckpointActor : public AActor
{
    GENERATED_BODY()

public:
    ACheckpointActor();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Checkpoint")
    EStateOfAlarmLevel RequiredLevel = EStateOfAlarmLevel::HighVigilance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Checkpoint")
    TSubclassOf<AActor> GuardClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Checkpoint")
    int32 MaxGuards = 3;

    UFUNCTION(BlueprintCallable, Category="AAA|Checkpoint")
    void SetCheckpointActive(bool bActive);

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void HandleAlarmChanged(EStateOfAlarmLevel NewLevel);

private:
    UPROPERTY()
    TArray<AActor*> SpawnedGuards;

    UPROPERTY(VisibleAnywhere)
    class USceneComponent* RootComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
    class UBoxComponent* TriggerZone;
};
