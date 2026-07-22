#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaCore.h"
#include "DetentionMinigameComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDetentionStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDetentionEnded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQTEWindow, float, WindowDuration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQTEResult, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDetentionResult, bool, bEscaped);

UENUM(BlueprintType)
enum class EDetentionState : uint8 { Idle, Arrested, Interrogating, Resisting, Escaped, Surrendered };

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALSASUAMANIFA_API UDetentionMinigameComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UDetentionMinigameComponent();

    // Start/Stop
    UFUNCTION(BlueprintCallable, Category="AAA|Detention")
    void StartMinigame(float InDuration = 30.f, float DifficultyMultiplier = 1.f);

    UFUNCTION(BlueprintCallable, Category="AAA|Detention")
    void StopMinigame(bool bForceFail = false);

    // Input from player (call this from PlayerController input mapping)
    UFUNCTION(BlueprintCallable, Category="AAA|Detention")
    void RegisterInputPress();

    UFUNCTION(BlueprintCallable, Category="AAA|Detention")
    void UseInventoryItemDuringMinigame(FName ItemID);

    // Delegates
    UPROPERTY(BlueprintAssignable, Category="AAA|Detention")
    FOnDetentionStarted OnDetentionStarted;

    UPROPERTY(BlueprintAssignable, Category="AAA|Detention")
    FOnDetentionEnded OnDetentionEnded;

    UPROPERTY(BlueprintAssignable, Category="AAA|Detention")
    FOnQTEWindow OnQTEWindow;

    UPROPERTY(BlueprintAssignable, Category="AAA|Detention")
    FOnQTEResult OnQTEResult;

    UPROPERTY(BlueprintAssignable, Category="AAA|Detention")
    FOnDetentionResult OnDetentionResult;

    UPROPERTY(BlueprintReadOnly, Category="AAA|Detention")
    EDetentionState CurrentState = EDetentionState::Idle;

    UPROPERTY(EditAnywhere, Category="AAA|Detention")
    float Duration = 30.f;

    UPROPERTY(EditAnywhere, Category="AAA|Detention")
    float MashPowerPerPress = 1.f;

    UPROPERTY(EditAnywhere, Category="AAA|Detention")
    FVector2D QTEIntervalRange = FVector2D(3.f, 7.f);

    UPROPERTY(EditAnywhere, Category="AAA|Detention")
    float StressIncreaseRate = 1.f; // per second

    UPROPERTY(EditAnywhere, Category="AAA|Detention")
    float SuccessThreshold = 100.f;

    UPROPERTY(BlueprintReadOnly, Category="AAA|Detention")
    float CurrentResistance = 0.f;

    UPROPERTY(BlueprintReadOnly, Category="AAA|Detention")
    float StressLevel = 0.f;

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

private:
    float Elapsed = 0.f;
    float NextQTETime = 0.f;
    float QTEWindowDuration = 1.5f;
    float Difficulty = 1.f;
    bool bQTEActive = false;

    void StartQTEWindow();
    void ResolveQTE(bool bSuccess);
    void ApplyStress(float Delta);
    void FinishMinigame(bool bEscaped);
};
