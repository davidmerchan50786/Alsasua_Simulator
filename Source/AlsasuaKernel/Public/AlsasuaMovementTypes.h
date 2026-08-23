#pragma once

#include "CoreMinimal.h"
#include "AlsasuaMovementTypes.generated.h"

UENUM(BlueprintType)
enum class EMovementGait : uint8 {
    Idle,
    Walking,
    Running,
    Sprinting
};
