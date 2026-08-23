#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Contratos/AlsasuaContratosUI.h"
#include "AlsasuaNochePilar.generated.h"

/**
 * Fase 28 (iluminacion nocturna) del antiguo DirectorArranque, ahora propiedad del pilar.
 * El director la invoca via IAlsasuaPilarArranque sin conocer este tipo.
 */
UCLASS()
class GF_WORLD_API UAlsasuaNochePilar : public UWorldSubsystem, public IAlsasuaPilarArranque
{
	GENERATED_BODY()

public:
	virtual int32 EjecutarArranque() override;
	virtual FString EtiquetaArranque() const override { return TEXT("iluminacion nocturna"); }
	virtual int32 OrdenArranque() const override { return 280; }
};
