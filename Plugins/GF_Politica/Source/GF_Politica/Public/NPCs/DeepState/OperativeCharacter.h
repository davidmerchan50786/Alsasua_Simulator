#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "OperativeCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAmbushAlert);

UCLASS()
class GF_POLITICA_API AOperativeCharacter : public ACharacter {
    GENERATED_BODY()
public:
    AOperativeCharacter();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeepState")
    bool bIsDisguised = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeepState|Ambush", meta = (ClampMin = "200"))
    float AmbushRadius = 1500.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeepState|Ambush", meta = (ClampMin = "1"))
    float AmbushDamagePerSecond = 25.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeepState|Ambush")
    float RevealImpulseForce = 800.f;

    UFUNCTION(BlueprintCallable, Category = "DeepState")
    void ExecuteAmbush();

    UPROPERTY(BlueprintAssignable, Category = "DeepState")
    FOnAmbushAlert OnAmbushAlert;

private:
    bool bAmbushActive = false;
    float AmbushTimer = 0.f;
    float AmbushDuration = 5.f;

    void RevealOperative();
    void DamageNearbyTargets();
    void AlertNearbyGuards();

    virtual void Tick(float DeltaTime) override;
};
