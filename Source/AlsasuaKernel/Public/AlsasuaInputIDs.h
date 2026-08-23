#pragma once

#include "CoreMinimal.h"
#include "AlsasuaInputIDs.generated.h"

UENUM(BlueprintType)
enum class EAlsasuaAbilityInputID : uint8
{
	None,
	Confirm,
	Cancel,
	Sprint,
	Rally,
	Shout,
	Interaction
};

typedef EAlsasuaAbilityInputID AbilityInputID;
