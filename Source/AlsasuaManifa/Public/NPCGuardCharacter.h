#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "NPCGuardCharacter.generated.h"

UCLASS()
class ALSASUAMANIFA_API ANPCGuardCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	ANPCGuardCharacter();
	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	class UAlsasuaAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	float SuspicionLevel = 0.0f;
};
