#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HostageCharacter.generated.h"

UENUM(BlueprintType)
enum class EHostageState : uint8 {
    Captive,
    Following,
    Safe,
    Dead
};

UCLASS()
class ALSASUAMANIFA_API AHostageCharacter : public ACharacter {
    GENERATED_BODY()
public:
    AHostageCharacter();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AAA|Hostage")
    EHostageState CurrentState = EHostageState::Captive;

    UFUNCTION(BlueprintCallable, Category="AAA|Hostage")
    void SetHostageState(EHostageState NewState);

    UFUNCTION(BlueprintCallable, Category="AAA|Hostage")
    void FollowPlayer(APawn* PlayerPawn);

protected:
    UPROPERTY()
    APawn* TargetPlayer;
};