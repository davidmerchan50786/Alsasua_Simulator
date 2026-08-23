#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Contratos/AlsasuaContratosUI.h"
#include "AlsasuaCtrlFarolasPilar.generated.h"

/**
 * Fase 24b (control automatico de farolas) del antiguo DirectorArranque, ahora propiedad del pilar.
 * El director la invoca via IAlsasuaPilarArranque sin conocer este tipo.
 */
UCLASS()
class GF_CARRETERAS_API UAlsasuaCtrlFarolasPilar : public UWorldSubsystem, public IAlsasuaPilarArranque
{
	GENERATED_BODY()

public:
	virtual int32 EjecutarArranque() override;
	virtual FString EtiquetaArranque() const override { return TEXT("farolas con control automatico"); }
	virtual int32 OrdenArranque() const override { return 250; }
};
