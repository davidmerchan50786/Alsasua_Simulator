#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Contratos/AlsasuaContratosUI.h"
#include "AlsasuaArranqueLOD.generated.h"

/**
 * Fase 15 (Nanite + HLOD) del antiguo DirectorArranque, ahora propiedad del pilar.
 * El director la invoca via IAlsasuaPilarArranque sin conocer este tipo.
 */
UCLASS()
class GF_WORLD_API UAlsasuaArranqueLOD : public UWorldSubsystem, public IAlsasuaPilarArranque
{
	GENERATED_BODY()

public:
	virtual int32 EjecutarArranque() override;
	virtual FString EtiquetaArranque() const override { return TEXT("LOD Nanite + HLOD"); }
	virtual int32 OrdenArranque() const override { return 150; }
};
