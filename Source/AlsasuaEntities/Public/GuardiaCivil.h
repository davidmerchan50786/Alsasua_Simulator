#pragma once
#include "CoreMinimal.h"
#include "NPCCharacter.h"
#include "Character/Stealth/GuardDetectionComponent.h"
#include "GuardiaCivil.generated.h"

UCLASS()
class ALSASUAENTITIES_API AGuardiaCivil : public ANPCCharacter
{
    GENERATED_BODY()
public:
    AGuardiaCivil();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Alsasua|Patrol")
    float PatrolRadius = 3000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Alsasua|Patrol")
    float PatrolWaitMin = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Alsasua|Patrol")
    float PatrolWaitMax = 6.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Alsasua|Alert")
    float SuspiciousRadius = 1500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Alsasua|Alert")
    float CombatRadius = 600.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Alsasua|Alert")
    EGuardAlertState AlertState = EGuardAlertState::Idle;

    UFUNCTION(BlueprintCallable, Category="Alsasua|Alert")
    void SetAlertState(EGuardAlertState NewState);

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

private:
    FVector SpawnLocation;
    FVector CurrentPatrolTarget;
    float WaitTimer = 0.0f;
    bool bWaiting = false;

    void PickNewPatrolTarget();
    void TickPatrol(float DeltaTime);
    void TickAlertScan(float DeltaTime);
};
