#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimeOfDayManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHourChanged, int32, CurrentHour);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNightStarted, bool, bIsNight);

UCLASS()
class ALSASUAMANIFA_API UTimeOfDayManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // 0.0 a 24.0
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Time")
    float CurrentTime = 8.0f; 

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Time")
    float TimeSpeed = 0.1f; // Velocidad del paso del tiempo

    UPROPERTY(BlueprintAssignable, Category="AAA|Time")
    FOnHourChanged OnHourChanged;

    UPROPERTY(BlueprintAssignable, Category="AAA|Time")
    FOnNightStarted OnNightStarted;

    UFUNCTION(BlueprintPure, Category="AAA|Time")
    bool IsNight() const { return CurrentTime < 6.0f || CurrentTime > 21.0f; }

    UFUNCTION(BlueprintPure, Category="AAA|Time")
    FString GetFormattedTime() const;

    void UpdateTime(float DeltaTime);

private:
    int32 LastHour = -1;
    bool bWasNight = false;
};
