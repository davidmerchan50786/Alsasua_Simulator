#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaNoiseComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALSASUAMANIFA_API UAlsasuaNoiseComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UAlsasuaNoiseComponent();

	// Emite un evento de ruido que los guardias pueden oír
	UFUNCTION(BlueprintCallable, Category = "Audio|Noise")
	void EmitNoise(float Intensity, float Radius);
};
