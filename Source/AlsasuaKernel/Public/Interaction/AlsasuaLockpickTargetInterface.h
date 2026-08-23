#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AlsasuaLockpickTargetInterface.generated.h"

/** Objetivo que la ability Lockpick puede inspeccionar y abrir
 *  (p.ej. AHideoutActor vive en GF_Systems; el tronco solo ve el contrato). */
UINTERFACE(MinimalAPI)
class UAlsasuaLockpickTarget : public UInterface
{
	GENERATED_BODY()
};

class ALSASUAKERNEL_API IAlsasuaLockpickTarget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Alsasua|Infiltracion")
	bool EstaCerrado() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Alsasua|Infiltracion")
	void AlForzado();
};
