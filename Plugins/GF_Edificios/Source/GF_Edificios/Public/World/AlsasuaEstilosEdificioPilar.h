#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Contratos/AlsasuaContratosUI.h"
#include "AlsasuaEstilosEdificioPilar.generated.h"

/**
 * Fase 17 (estilo de barrio + ventanas emissivas) del antiguo DirectorArranque, ahora propiedad del pilar.
 * El director la invoca via IAlsasuaPilarArranque sin conocer este tipo.
 */
UCLASS()
class GF_EDIFICIOS_API UAlsasuaEstilosEdificioPilar : public UWorldSubsystem, public IAlsasuaPilarArranque
{
	GENERATED_BODY()

public:
	virtual int32 EjecutarArranque() override;
	virtual FString EtiquetaArranque() const override { return TEXT("edificios con estilo y ventanas emissivas"); }
	virtual int32 OrdenArranque() const override { return 170; }
};
