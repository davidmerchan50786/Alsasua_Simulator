#pragma once
#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "AlsasuaGameplayAbility.generated.h"

UCLASS()
class ALSASUAKERNEL_API UAlsasuaGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
public:
	UAlsasuaGameplayAbility();

	// Etiqueta para identificar la habilidad (Input ID)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	int32 AbilityInputID = 0;
};
