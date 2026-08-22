#pragma once
#include "CoreMinimal.h"
#include "Abilities/AlsasuaGameplayAbility.h"
#include "BoostRecruitmentAbility.generated.h"

UCLASS()
class GF_ABILITIES_API UBoostRecruitmentAbility : public UAlsasuaGameplayAbility {
    GENERATED_BODY()
public:
    UBoostRecruitmentAbility();
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Social")
    float RecruitmentRadius = 1500.f;
};