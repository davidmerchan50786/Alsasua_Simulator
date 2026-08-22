#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "AlsasuaMassProxies.generated.h"

USTRUCT(BlueprintType)
struct FMassProtesterProxy
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Position;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRotator Rotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 State; // 0 idle,1 walking,2 chanting

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Energy;

	FMassProtesterProxy()
	: Position(FVector::ZeroVector), Rotation(FRotator::ZeroRotator), State(0), Energy(1.0f) {}
};


