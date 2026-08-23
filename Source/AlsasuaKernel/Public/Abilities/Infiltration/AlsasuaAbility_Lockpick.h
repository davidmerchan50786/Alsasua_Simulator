#pragma once
#include "CoreMinimal.h"
#include "Abilities/AlsasuaGameplayAbility.h"
#include "AlsasuaAbility_Lockpick.generated.h"

UCLASS()
class ALSASUAKERNEL_API UAlsasuaAbility_Lockpick : public UAlsasuaGameplayAbility {
    GENERATED_BODY()
public:
    UAlsasuaAbility_Lockpick();

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

    UPROPERTY(EditAnywhere, Category="Stealth")
    float BaseUnlockTime = 3.0f;
};