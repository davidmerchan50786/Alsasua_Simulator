#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ProcessionDirectorSubsystem.generated.h"

UENUM(BlueprintType)
enum class EProcessionOutcome : uint8 {
    Undetermined,
    PublicJustice,
    StateRepression,
    BloodySunday
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerStatsModified, float, WantedLevelDelta, float, PopularSupportDelta);

UCLASS()
class GF_SYSTEMS_API UProcessionDirectorSubsystem : public UWorldSubsystem {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="AAA|SetPiece")
    void StartProcessionEvent();

    UFUNCTION(BlueprintCallable, Category="AAA|SetPiece")
    void PlayerAction_ExposeCloacas();

    UFUNCTION(BlueprintCallable, Category="AAA|SetPiece")
    void TriggerBombFailure();

    UPROPERTY(BlueprintReadOnly, Category="AAA|SetPiece")
    EProcessionOutcome CurrentOutcome = EProcessionOutcome::Undetermined;

    UPROPERTY(BlueprintAssignable)
    FOnPlayerStatsModified OnPlayerStatsModified;

private:
    void UpdateCityStateByOutcome();
};