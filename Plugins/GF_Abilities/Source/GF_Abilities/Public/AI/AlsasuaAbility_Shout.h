#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "AlsasuaAbility_Shout.generated.h"

/** Habilidad del jugador para emitir consignas de protesta */
UCLASS()
class GF_ABILITIES_API UAlsasuaAbility_Shout : public UGameplayAbility
{
    GENERATED_BODY()

public:
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

    // Tipo de consigna: Motivadora, Agresiva, AntiGC
    UPROPERTY(EditAnywhere, Category = "AAA|Shout")
    FString ShoutType;

    // Radio de efecto del grito
    UPROPERTY(EditAnywhere, Category = "AAA|Shout")
    float Radius = 500.f;
};
