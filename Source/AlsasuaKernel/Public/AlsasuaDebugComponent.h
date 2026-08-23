#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaDebugComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALSASUAKERNEL_API UAlsasuaDebugComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAlsasuaDebugComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Activa/Desactiva visualización de radios de IA y Sonido
	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bShowAIFieldOfView = true;

	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bShowNoiseRadii = true;

protected:
	void DrawAIDebug();
	void DrawCharacterDebug();
};
