#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Contratos/AlsasuaContratosUI.h"
#include "Arranque/AlsasuaPilarArranque.h"
#include "AlsasuaLucesInteriorPilar.generated.h"

/**
 * Fase 19 (luz interior por edificio) del antiguo DirectorArranque, ahora propiedad del pilar.
 * El director la invoca via IAlsasuaPilarArranque sin conocer este tipo.
 */
UCLASS()
class GF_WORLD_API UAlsasuaLucesInteriorPilar : public UWorldSubsystem, public IAlsasuaPilarArranque
{
	GENERATED_BODY()

public:
	virtual int32 EjecutarArranque() override;
	virtual FString EtiquetaArranque() const override { return TEXT("luces interiores de edificios"); }
	virtual int32 OrdenArranque() const override { return 190; }
};
