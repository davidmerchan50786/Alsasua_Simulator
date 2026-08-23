#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AlsasuaStealthProfileInterface.generated.h"

/** Perfil de sigilo consultable por los sistemas de deteccion del tronco
 *  (GuardDetectionComponent). Lo implementa, p.ej., UDisguiseComponent de
 *  GF_Social sin que el tronco dependa de ese pilar. */
UINTERFACE(MinimalAPI)
class UAlsasuaStealthProfile : public UInterface
{
	GENERATED_BODY()
};

class ALSASUAKERNEL_API IAlsasuaStealthProfile
{
	GENERATED_BODY()

public:
	/** Multiplicador aplicado al rango de vision del guardia (1 = sin efecto).
	 *  Nombres distintos de los UFUNCTION del implementador para que UHT
	 *  no vea colision entre el evento y el BlueprintPure. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Alsasua|Sigilo")
	float GetVisionMultiplier() const;

	/** Reduccion aplicada al ruido percibido (1 = sin efecto, 0 = silencio total). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Alsasua|Sigilo")
	float GetNoiseDampening() const;
};
