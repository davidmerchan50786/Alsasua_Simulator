#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "StateOfAlarmSubsystem.generated.h"

UENUM(BlueprintType)
enum class EStateOfAlarmLevel : uint8 {
    Normal,
    HighVigilance,
    StateOfAlarm,
    Curfew // Toque de queda total
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAlarmLevelChanged, EStateOfAlarmLevel, NewLevel);

UCLASS()
class GF_POLITICA_API UStateOfAlarmSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category="AAA|Politics|Crisis")
    void UpdateState(float StateInfluence, float ProtestIntensity);

    UPROPERTY(BlueprintReadOnly, Category="AAA|Politics|Crisis")
    EStateOfAlarmLevel CurrentLevel = EStateOfAlarmLevel::Normal;

    UPROPERTY(BlueprintAssignable, Category="AAA|Politics|Crisis")
    FOnAlarmLevelChanged OnAlarmLevelChanged;

    UFUNCTION(BlueprintPure, Category="AAA|Politics|Crisis")
    bool IsCurfewActive() const { return CurrentLevel == EStateOfAlarmLevel::Curfew; }
};
