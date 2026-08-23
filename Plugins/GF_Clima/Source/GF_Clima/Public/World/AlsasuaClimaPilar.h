#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Contratos/AlsasuaContratosUI.h"
#include "AlsasuaClimaPilar.generated.h"

class UAlsasuaWeatherSystem;

/**
 * Fase 29 del antiguo DirectorArranque: crea el componente de clima sobre el
 * WorldSettings y publica su estado via IAlsasuaEstadoClima para el puente
 * clima->audio que antes hacia el director cada tick.
 */
UCLASS()
class GF_CLIMA_API UAlsasuaClimaPilar : public UWorldSubsystem,
	public IAlsasuaPilarArranque, public IAlsasuaEstadoClima
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Deinitialize() override;

	virtual int32 EjecutarArranque() override;
	virtual FString EtiquetaArranque() const override { return TEXT("sistema de clima activado"); }
	virtual int32 OrdenArranque() const override { return 290; }

	//~ IAlsasuaEstadoClima
	virtual bool EstaLloviendo() const override;
	virtual bool HayTormenta() const override;
	virtual float HoraDeJuego() const override;
	virtual float VelocidadViento() const override;
	virtual float IntensidadLluvia() const override;

private:
	UPROPERTY()
	TObjectPtr<UAlsasuaWeatherSystem> Componente;
};
